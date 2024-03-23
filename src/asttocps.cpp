/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <string.h>
#include "compiler.hpp"
#include "parser.hpp"

bool hasSideEffects(Expr *expr) {
  if (expr)
    switch (expr->type) {
      case EXPR_REFERENCE:
        return false;

      default:
        return true;
    }

  return false;
}

Expr *idExpr(Expr *newExp) {
  return newExp;
}

GroupingExpr *makeBooleanContinuation(K k, bool boolParam) {
  DeclarationExpr *param = boolParam ? newDeclarationExpr(BOOL_TYPE, buildToken(TOKEN_IDENTIFIER, "arg"), NULL) : NULL;
  ReferenceExpr *arg = boolParam ? new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "arg"), NULL) : NULL;

  return makeWrapperLambda(genSymbol("c"), param, [k, arg]() -> Expr* {return k(arg);});
}

bool compareExpr(Expr *origExpr, Expr *newExpr) {
  bool equal = origExpr == newExpr;

  if (!equal)
;//    delete origExpr;

  return equal;
}

Expr *getCompareExpr(Expr *origExpr, Expr *newExpr) {
  compareExpr(origExpr, newExpr);
  return newExpr;
}

bool isCpsContext() {
  return getFunction() && getFunction()->body->hasSuperCalls;
}

Expr *declarationToCps(Expr **statementRef, std::function<Expr *()> genGroup) {
  if (statementRef) {
    Expr *left = car(*statementRef, TOKEN_SEPARATOR);

    if (left->type == EXPR_FUNCTION) {
      left->toCps(idExpr);
      statementRef = removeExpr(statementRef, TOKEN_SEPARATOR);
    }
    else
      statementRef = cdrAddress(*statementRef, TOKEN_SEPARATOR);

    Expr *right = declarationToCps(statementRef, genGroup);

    return left->type == EXPR_FUNCTION ? right ? new BinaryExpr(left, buildToken(TOKEN_SEPARATOR, ";"), right) : left : right;
  }
  else
    return genGroup();
}

static const char *getHandlerType(Type type) {
  switch (type.valueType) {
    case VAL_VOID: return "voidHandler_";
    case VAL_BOOL: return "boolHandler_";
    case VAL_INT: return "intHandler_";
    case VAL_FLOAT: return "floatHandler_";
    case VAL_OBJ:
      switch (type.objType->type) {
        case OBJ_STRING: return "stringHandler_";
        case OBJ_ANY: return "anyHandler_";
        default: return NULL;
      }
    default: return NULL;
  }
}

Expr *IteratorExpr::toCps(K k) {
  Expr *iteratorExpr = this->value->toCps([this, k](Expr *value) {
    return compareExpr(this->value, value) ? this : k(idExpr(new IteratorExpr(this->name, this->op, value)));
  });

  return this == iteratorExpr ? k(this) : iteratorExpr;
}

Expr *AssignExpr::toCps(K k) {
  Expr *cpsExpr = varExp->toCps([this, k](Expr *varExp) {
    return this->value
      ? this->value->toCps([this, k, varExp](Expr *value) {
          return compareExpr(this->varExp, varExp) && compareExpr(this->value, value) ? this : k(idExpr(new AssignExpr(varExp, this->op, value)));
        })
      : compareExpr(this->varExp, varExp) ? this : k(idExpr(new AssignExpr(varExp, this->op, NULL)));
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *CastExpr::toCps(K k) {
  Expr *cpsExpr = expr->toCps([this, k](Expr *expr) {
    return compareExpr(this->expr, expr) ? this : k(idExpr(new CastExpr(type, expr)));
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *UIAttributeExpr::toCps(K k) {
  return k(this); // assume no cps for now
}

Expr *UIDirectiveExpr::toCps(K k) {
  return k(this); // assume no cps for now
}

Expr *BinaryExpr::toCps(K k) {
  if (op.type == TOKEN_SEPARATOR) {
    if (!hasSideEffects(left))
      return this->right ? this->right->toCps(k) : NULL;//k(getCompareExpr(this->left, left));
    else {
      Expr *exp = left->toCps([this, k](Expr *left) {
        if (hasSideEffects(left)) {
          Expr *right = this->right ? this->right->toCps(k) : NULL;//k(getCompareExpr(this->left, left));

          return left && right ? new BinaryExpr(left, op, right) : left ? left : right;
        }
        else
          return this->right ? this->right->toCps(k) : NULL;
      });

      return exp;
    }
  }
  else {
    auto genBinary = [this](Expr *left, Expr *right) {
      return compareExpr(this->left, left) && compareExpr(this->right, right) ? this : idExpr(new BinaryExpr(left, this->op, right));
    };
    Expr *cpsExpr = left->toCps([this, genBinary, k](Expr *left) {
      return this->right->toCps([this, genBinary, k, left](Expr *right) {
        Expr *expr = genBinary(left, right);

        return this == expr ? this : k(expr);
      });
    });

    return this == cpsExpr ? k(this) : cpsExpr;
  }
}

Expr *CallExpr::toCps(K k) {
  FunctionExpr *function = (FunctionExpr *) _declaration->expr;
  bool userClassCall = !newFlag && function && function->_declaration.name.isUserClass();
  auto genCall = [this, function, userClassCall, k](bool same, Expr *callee, Expr *&args) {
    if (function->_declaration.name.isUserClass()) {
      Type returnType = function->_declaration.type;

      if (IS_VOID(returnType)) {
        DeclarationExpr *lastParam = getParam(function, function->arity - 1);

        if (lastParam && lastParam->_declaration.name.equal("_HandlerFn_")) {
          FunctionExpr *handlerExpr = AS_FUNCTION_TYPE(lastParam->_declaration.type)->expr;

          if (handlerExpr->arity)
            returnType = getParam(handlerExpr, handlerExpr->arity - 1)->_declaration.type;
        }
      }

      bool parmFlag = !IS_VOID(returnType);
      DeclarationExpr *param = parmFlag ? newDeclarationExpr(returnType, buildToken(TOKEN_IDENTIFIER, "_ret"), NULL) : NULL;
      ReferenceExpr *arg = parmFlag ? new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "_ret"), NULL) : NULL;
      std::function<Expr*()> bodyFn = [this]() {return this->handler;};

      if (!newFlag)
        bodyFn = [k, arg]() -> Expr* {return k(arg);};

      this->handler = makeWrapperLambda("Lambda_", param, bodyFn);
    }

    if (this->handler) {
      addExpr(same && !userClassCall ? &this->args : &args, this->handler, buildToken(TOKEN_COMMA, ","));
      this->handler = NULL;
    }

    return same && !userClassCall ? this : idExpr(new CallExpr(this->newFlag, callee, this->paren, args, NULL));
  };

  Expr *cpsExpr = callee->toCps([this, genCall](Expr *callee) {
    return this->args
      ? this->args->toCps([this, genCall, callee](Expr *args) {
          return genCall(compareExpr(this->callee, callee) && compareExpr(this->args, args), callee, args);
        })
      : genCall(compareExpr(this->callee, callee), callee, this->args);
  });

  return this != cpsExpr && userClassCall ? cpsExpr : k(cpsExpr);
}

Expr *ArrayElementExpr::toCps(K k) {
  return k(this);
}

Expr *DeclarationExpr::toCps(K k) {
  if (isCpsContext() && initExpr) {
    auto genAssign = [this, k](Expr *initExpr) {
//      AssignExpr *assignExpr = new AssignExpr(new ReferenceExpr(_declaration.name, this), buildToken(TOKEN_EQUAL, "="), initExpr);
      DeclarationExpr *declarationExpr = new DeclarationExpr(initExpr, 2);

      declarationExpr->_declaration = _declaration;
      declarationExpr->_declaration.expr = declarationExpr;
      this->initExpr = NULL;
      return k(declarationExpr);
    };

    if (initExpr->hasSuperCalls)
      return initExpr->toCps([genAssign](Expr *initExpr) {
        return genAssign(initExpr);
      });
    else {
      initExpr->toCps(idExpr);
      return genAssign(initExpr);
    }
  }
  else
    return k(initExpr ? this : NULL);
}

Expr *FunctionExpr::toCps(K k) {
  if (_declaration.name.isUserClass()) {
    const char *type = getHandlerType(_declaration.type);
    Token typeName = buildToken(TOKEN_IDENTIFIER, type);
    Declaration *paramType = getFirstDeclarationRef(getCurrent(), typeName, 2);

    if (!paramType)
      paramType = NULL;

    if (getCurrent()) {
      Token name = buildToken(TOKEN_IDENTIFIER, "_HandlerFn_");
      DeclarationExpr *dec = newDeclarationExpr(OBJ_TYPE(&((FunctionExpr *) paramType->expr)->_function), name, NULL);

//      checkDeclaration(dec->_declaration, dec->_declaration.name, NULL, NULL);
      addExpr(&params, dec, buildToken(TOKEN_COMMA, ","));
      _declaration.type = VOID_TYPE;
      arity++;
    }
    else
      arity++;
  }

//  if (_declaration.name.isClass()) {
    pushScope(this);
    body->body = declarationToCps(initAddress(body->body), [this]() {
      return body->body ? body->body->toCps(idExpr) : NULL;
    });
    popScope();
//  }

  return k(this);//compareExpr(body, bodyExpr) ? this : newExpr(newFunctionExpr(typeExpr, name, arity + 1, newParams, newBody)));
}

Expr *GetExpr::toCps(K k) {
  Expr *cpsExpr = object->toCps([this, k](Expr *object) {
    return compareExpr(this->object, object) ? this : k(idExpr(new GetExpr(object, this->name)));
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *GroupingExpr::toCps(K k) {
  if (body) {
    pushScope(this);

    body = declarationToCps(initAddress(body), [this, k]() {
      return body ? body->toCps(hasSuperCalls ? k : idExpr) : NULL;
    });

    if (hasSuperCalls)
      this->body = body;

    popScope();
  }

  return hasSuperCalls ? this : k(this);
}

Expr *ArrayExpr::toCps(K k) {
  Expr *body = this->body ? this->body->toCps(idExpr) : NULL;

  return k(compareExpr(this->body, body) ? this : (ArrayExpr *) idExpr(new ArrayExpr(body)));
}

Expr *LiteralExpr::toCps(K k) {
  return k(this);
}

Expr *LogicalExpr::toCps(K k) {
  Expr *cpsExpr = left->toCps([this, k](Expr *left) -> Expr* {
    const char *cvar = genSymbol("i");
    bool rightEqual = true;
    Expr *right = this->right->toCps([this, &rightEqual, cvar](Expr *right) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), NULL);

      rightEqual = compareExpr(this->right, right);
      return new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), right, NULL);
    });

    if (rightEqual) {
      // delete right
      return compareExpr(this->left, left) ? this : k(idExpr(new LogicalExpr(left, this->op, this->right)));
    } else {
      GroupingExpr *cast = makeBooleanContinuation(k, true);
      Type contType = {VAL_OBJ, &((FunctionExpr *) cast->body)->_function.obj};
      // delete this->right
      DeclarationExpr *param = newDeclarationExpr(contType, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      Expr *left2 = getCompareExpr(this->left, left);
      Expr *notExpr = this->op.type == TOKEN_OR_OR ? new UnaryExpr(buildToken(TOKEN_BANG, "!"), left2) : left2;
      ReferenceExpr *callee2 = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      Expr *elseExpr = idExpr(new CallExpr(false, callee2, buildToken(TOKEN_CALL, "("), new LiteralExpr(VAL_BOOL, {.boolean = false}), NULL));
      std::function<Expr*()> bodyFn = [notExpr, right, elseExpr]() -> Expr* {
        return new IfExpr(notExpr, right, elseExpr);
      };

      Expr *wrapperLambda = makeWrapperLambda(param, bodyFn);

      return idExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_CALL, "("), cast, NULL));
    }
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

// (function Lambda_() {if (condition) {body; post_(Lambda_)} else {k}})();
Expr *WhileExpr::toCps(K k) {
  if (hasSuperCalls) {
    char *newSymbol = genSymbol("while");
    auto genBody = [this, k, &newSymbol](Expr *condition) {
      Expr *elseExpr = k(NULL);
      Expr *arg = NULL;
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, newSymbol), NULL);

      if (!this->condition->hasSuperCalls && !body->hasSuperCalls) {
        arg = callee;
        callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), NULL);
      }

      CallExpr *call = new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), arg, NULL);

      call->resolve(*((Parser *) NULL));
      body = addToGroup(&body, call);
      Expr *newBody = this->body->toCps(idExpr);
      return new IfExpr(condition, newBody, elseExpr);
    };
    Expr *wrapperLambda = makeWrapperLambda(newSymbol, NULL, [this, genBody]() {
      return condition->hasSuperCalls
        ? condition->toCps([genBody](Expr *condition) {return genBody(condition);})
        : genBody(condition);
    });

    return idExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_CALL, "("), NULL, NULL));
  }
  else
    return k(this);
}

Expr *ReturnExpr::toCps(K k) {
  auto genReturn = [](ReturnExpr *expr) -> Expr * {
    if (expr->isUserClass) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), NULL);
      Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "_HandlerFn_"), NULL);
      Expr *value = expr->value;

      if (value) {
        CallExpr *call = new CallExpr(false, param, buildToken(TOKEN_LEFT_PAREN, "("), value, NULL);

        param = makeWrapperLambda("lambda_", NULL, [call]() {return call;});
      }

      callee->resolve(*((Parser *) NULL));
      value = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), param, NULL);
      value = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), value, NULL, NULL);
      addExpr(&((GroupingExpr *) value)->body, new ReturnExpr(expr->keyword, false, NULL), buildToken(TOKEN_SEPARATOR, ";"));
      return value;
    }
    else
      return expr;
  };
  Expr *cpsExpr = !value ? this : value->toCps([this, genReturn, k](Expr *value) {
    return compareExpr(this->value, value) ? this : k(idExpr(genReturn(new ReturnExpr(keyword, false, value))));
  });

  return this == cpsExpr ? k(genReturn(this)) : cpsExpr;
}

Expr *SetExpr::toCps(K k) {
  Expr *cpsExpr = object->toCps([this, k](Expr *object) {
    return this->value->toCps([this, k, object](Expr *value) {
      return compareExpr(this->object, object) && compareExpr(this->value, value) ? this : k(idExpr(new SetExpr(object, this->name, this->op, value)));
    });
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *IfExpr::toCps(K k) {
  Expr *cpsExpr = condition->toCps([this, k](Expr *condition) -> Expr* {
    const char *cvar = genSymbol("i");
    bool bothEqual = true;

    Expr *thenBranch = this->thenBranch->toCps([this, &bothEqual, cvar](Expr *thenBranch) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), NULL);

      bothEqual &= compareExpr(this->thenBranch, thenBranch);
      return *addExpr(&thenBranch, new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), NULL, NULL), buildToken(TOKEN_SEPARATOR, ";"));
    });
    ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), NULL);
    CallExpr *elseExpr = new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), NULL, NULL);
    Expr *elseBranch = this->elseBranch ? this->elseBranch->toCps([this, &bothEqual, elseExpr](Expr *elseBranch) {
      bothEqual &= compareExpr(this->elseBranch, elseBranch);
      return *addExpr(&elseBranch, elseExpr, buildToken(TOKEN_SEPARATOR, ";"));
    }) : elseExpr;

    if (bothEqual) {
      // delete cvar, thenBranch and elseBranch
      return compareExpr(this->condition, condition) ? this : k(idExpr(new IfExpr(condition, this->thenBranch, this->elseBranch)));
    } else {
      // delete this->thenBranch, this->elseBranch
      GroupingExpr *cast = makeBooleanContinuation(k, false);
      Type contType = {VAL_OBJ, &((FunctionExpr *) cast->body)->_function.obj};
      DeclarationExpr *param = newDeclarationExpr(contType, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      std::function<Expr*()> bodyFn = [condition, thenBranch, elseBranch]() -> Expr* {
        return new IfExpr(condition, thenBranch, elseBranch);
      };

      Expr *wrapperLambda = makeWrapperLambda(param, bodyFn);

      return idExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_CALL, "("), cast, NULL));
    }
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *TernaryExpr::toCps(K k) {
  Expr *cpsExpr = left->toCps([this, k](Expr *left) -> Expr* {
    Expr *testExpr;
    const char *cvar = genSymbol("i");
    bool bothEqual = true;
    K newK = [this, &testExpr, &bothEqual, cvar](Expr *ifResult) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), NULL);

      bothEqual &= compareExpr(testExpr, ifResult);
      return new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), ifResult, NULL);
    };

    Expr *middle = (testExpr = this->middle)->toCps(newK);
    Expr *right = (testExpr = this->right)->toCps(newK);

    if (bothEqual) {
      // delete middle and right
      return compareExpr(this->left, left) ? this : k(idExpr(new TernaryExpr(this->op, left, this->middle, this->right)));
    } else {
      // delete this->middle, this->right
      GroupingExpr *cast = makeBooleanContinuation(k, true);
      Type contType = {VAL_OBJ, &((FunctionExpr *) cast->body)->_function.obj};
      DeclarationExpr *param = newDeclarationExpr(contType, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      std::function<Expr*()> bodyFn = [left, middle, right]() -> Expr* {
        return new IfExpr(left, middle, right);
      };

      Expr *wrapperLambda = makeWrapperLambda(param, bodyFn);

      return idExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_CALL, "("), cast, NULL));
    }
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *ThisExpr::toCps(K k) {
  return k(this);
}

Expr *UnaryExpr::toCps(K k) {
  Expr *cpsExpr = right->toCps([this, k](Expr *right) {
    return compareExpr(this->right, right) ? this : k(idExpr(new UnaryExpr(this->op, right)));
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *PrimitiveExpr::toCps(K k) {
  return k(this);
}

Expr *ReferenceExpr::toCps(K k) {
//  Declaration *declaration = this->declaration ? getDeclaration(this->declaration) : NULL;

//  if (declaration && declaration->function && isExternalField(declaration->function, declaration) && declaration->function != function->expr)
  Declaration *dec = declaration ? getDeclaration(declaration) : NULL;

  if (dec && dec->function && dec->function != getTopFunction() && isExternalField(dec->function, dec))
    dec->function->_function.hasInternalFields = true;

  return k(this);
}

Expr *SwapExpr::toCps(K k) {
  Expr *cpsExpr = expr->toCps([this, k](Expr *_expr) -> Expr* {
    SwapExpr *swapExpr = compareExpr(this->expr, _expr) ? this : (SwapExpr *) idExpr(new SwapExpr(NULL));

    if (swapExpr == this)
      return this;
    else {
      swapExpr->expr = _expr;
      return k(swapExpr);
    }
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *NativeExpr::toCps(K k) {
  return k(this);
}
