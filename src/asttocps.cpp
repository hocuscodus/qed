/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <string.h>
#include "compiler.hpp"
#include "parser.hpp"


int GENSYM = 0;

Expr *idExpr(Expr *newExp) {
  return newExp;
}

char *genSymbol(std::string name) {
    std::string s = name + std::to_string(++GENSYM) + "$_";
    char *str = new char[s.size() + 1];

    strcpy(str, s.c_str());
    return str;
}

GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, std::function<Expr*()> bodyFn) {
  int arity = param ? 1 : 0;
  Token nameToken = buildToken(TOKEN_IDENTIFIER, name);
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), param, NULL);
  FunctionExpr *wrapperFunc = newFunctionExpr(VOID_TYPE, nameToken, arity, group, NULL);
  GroupingExpr *mainGroup = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), wrapperFunc, NULL);

  pushScope(mainGroup);
  checkDeclaration(wrapperFunc->_declaration, nameToken, wrapperFunc, NULL);
  pushScope(wrapperFunc);

  if (param)
    checkDeclaration(param->_declaration, param->_declaration.name, NULL, NULL);

  addExpr(&group->body, bodyFn(), buildToken(TOKEN_SEPARATOR, ";"));
  popScope();
  popScope();
  return mainGroup;
}

GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, Expr *body) {
  return makeWrapperLambda(name, param, [body]() -> Expr* {return body;});
}

GroupingExpr *makeWrapperLambda(DeclarationExpr *param, std::function<Expr*()> bodyFn) {
  return makeWrapperLambda(genSymbol("W"), param, bodyFn);
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
  return getCurrent() && getCurrent()->group->hasSuperCalls;
}

Expr *declarationToCps(Declaration *declaration, std::function<Expr *()> genGroup);

FunctionExpr *functionToCps(FunctionExpr *function) {
  if (function->_declaration.name.isUserClass()) {
    function->_declaration.type = VOID_TYPE;
    function->arity++;
  }

  if (function->_declaration.name.isClass()) {
    pushScope(function);
    function->body->body = declarationToCps(function->body->declarations, [function]() {
      Expr *body = getStatement(function->body, function->arity);

      return body ? body->toCps([](Expr *body) {return body;}) : NULL;
    });
    popScope();
  }

  return function;
}

Expr *declarationToCps(Declaration *declaration, std::function<Expr *()> genGroup) {
  if (declaration) {
    if (declaration->expr->type == EXPR_FUNCTION)
      functionToCps((FunctionExpr *) declaration->expr);

    Expr *right = declarationToCps(declaration->next, genGroup);

    return right ? new BinaryExpr(declaration->expr, buildToken(TOKEN_SEPARATOR, ";"), right) : declaration->expr;
  }
  else
    return genGroup();
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
    Expr *exp = left->toCps([this, k](Expr *left) {
      Expr *right = this->right ? this->right->toCps(k) : NULL;//k(getCompareExpr(this->left, left));

      return left && right ? new BinaryExpr(left, op, right) : left ? left : right;
    });

    return exp;
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
  pushSignature(NULL);
  Type type = resolveType(callee);
  popSignature();

  ObjFunction *callable = AS_FUNCTION_TYPE(type);
  FunctionExpr *function = callable ? callable->expr : NULL;
  bool userClassCall = !newFlag && function && function->_declaration.name.isUserClass();
  auto genCall = [this, function, userClassCall, k](bool same, Expr *callee, Expr *&params) {
    if (function->_declaration.name.isUserClass()) {
      Type returnType = function->_declaration.type;

      if (IS_VOID(returnType)) {
        DeclarationExpr *lastParam = getParam(function, function->arity - 1);

        if (lastParam && lastParam->_declaration.name.equal("handlerFn_")) {
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
      addExpr(same && !userClassCall ? &this->params : &params, this->handler, buildToken(TOKEN_COMMA, ","));
      this->handler = NULL;
    }

    return same && !userClassCall ? this : idExpr(new CallExpr(this->newFlag, callee, this->paren, params, NULL));
  };

  Expr *cpsExpr = callee->toCps([this, genCall](Expr *callee) {
    return this->params
      ? this->params->toCps([this, genCall, callee](Expr *params) {
          return genCall(compareExpr(this->callee, callee) && compareExpr(this->params, params), callee, params);
        })
      : genCall(compareExpr(this->callee, callee), callee, this->params);
  });

  return this != cpsExpr && userClassCall ? cpsExpr : k(cpsExpr);
}

Expr *ArrayElementExpr::toCps(K k) {
  return k(this);
}

Expr *DeclarationExpr::toCps(K k) {
  if (initExpr) {
    auto genAssign = [this, k](Expr *initExpr) {
      AssignExpr *assignExpr = new AssignExpr(new ReferenceExpr(_declaration.name, this), buildToken(TOKEN_EQUAL, "="), initExpr);

      this->initExpr = NULL;
      return k(assignExpr);
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
    return k(this);
}

Expr *FunctionExpr::toCps(K k) {
  return k(isCpsContext() ? NULL : functionToCps(this));//compareExpr(body, bodyExpr) ? this : newExpr(newFunctionExpr(typeExpr, name, arity + 1, newParams, newBody, NULL)));
/*
function cps_lambda(exp, k) {
  var cont = gensym("K");
  var body = cps(exp.body, function(body){
    return { type: "call",
            func: { type: "var", value: cont },
            args: [ body ] };
  });
  return k({ type: "lambda",
            name: exp.name,
            vars: [ cont ].concat(exp.vars),
            body: body });
}*/
}

Expr *GetExpr::toCps(K k) {
  Expr *cpsExpr = object->toCps([this, k](Expr *object) {
    return compareExpr(this->object, object) ? this : k(idExpr(new GetExpr(object, this->name, this->index)));
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *GroupingExpr::toCps(K k) {
  if (body) {
    pushScope(this);

    Expr *body = this->body->toCps(hasSuperCalls ? k : idExpr);

    if (hasSuperCalls)
      this->body = body;

    popScope();
  }

  return hasSuperCalls ? this : k(this);
}

Expr *ArrayExpr::toCps(K k) {
  Expr *body = this->body 
    ? this->body->toCps([this, k](Expr *body) {
        return body;
      })
    : NULL;

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

  return this == cpsExpr ? k(this) : cpsExpr;/*
  return left->toCps([this, k](Expr *left) -> Expr* {
    Expr *right = this->right->toCps([this, k](Expr *right) {
      return compareExpr(this->right, right) ? right : k(right);
    });

    if (compareExpr(this->left, left) && right == this->right)
      return k(this);
    else {
      Expr *notExpr = this->op.type == TOKEN_OR_OR ? new UnaryExpr(buildToken(TOKEN_BANG, "!"), left) : left;

      return new IfExpr(notExpr, right, k(new LiteralExpr(VAL_BOOL, {.boolean = false})));
    }
  });*/
/*
function cps_if(exp, k) {
    return cps(exp.cond, function(cond){
        var cvar = gensym("I");
        function k2(ifresult) {
            return {
                type: "call",
                func: { type: "var", callee: cvar },
                args: [ ifresult ]
            };
        };
        return {
            type: "call",
            func: {
                type: "lambda",
                vars: [ cvar ],
                body: {type: "if", cond: cond, then: cps(exp.then, k2), else: cps(exp.else || FALSE, k2)}
            },
            args: [ make_continuation(k) ]
        };
    });
}*/
/*
function cps_if(exp, k) {
    return cps(exp.cond, function(cond){
        var cvar = gensym("I");
        var cast = make_continuation(k);
        k = function(ifresult) {
            return {
                type: "call",
                func: { type: "var", value: cvar },
                args: [ ifresult ]
            };
        };
        return {
            type: "call",
            func: {
                type: "lambda",
                vars: [ cvar ],
                body: {
                    type: "if",
                    cond: cond,
                    then: cps(exp.then, k),
                    else: cps(exp.else || FALSE, k)
                }
            },
            args: [ cast ]
        };
    });
}*/
}

// (function Lambda_() {if (condition) {body; post_(Lambda_)} else {k}})();
Expr *WhileExpr::toCps(K k) {
  if (hasSuperCalls) {
    char *newSymbol = genSymbol("while");
    auto genBody = [this, k, &newSymbol](Expr *condition) {
      Expr *body = this->body;
      Expr *elseExpr = k(NULL);

      if (body->hasSuperCalls) {
        ReferenceExpr *arg = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, newSymbol), NULL);
        ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), NULL);
        CallExpr *call = new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), arg, NULL);

        this->body = addToGroup(&body, call);
        body = this->body->toCps([](Expr *body) {
          return body;
        });
      }

      return new IfExpr(condition, body, elseExpr);
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
  Expr *cpsExpr = !value ? this : value->toCps([this, k](Expr *value) {
    return compareExpr(this->value, value) ? this : k(idExpr(new ReturnExpr(this->keyword, this->isUserClass, value)));
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *SetExpr::toCps(K k) {
  Expr *cpsExpr = object->toCps([this, k](Expr *object) {
    return this->value->toCps([this, k, object](Expr *value) {
      return compareExpr(this->object, object) && compareExpr(this->value, value) ? this : k(idExpr(new SetExpr(object, this->name, this->op, value, this->index)));
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
/*
  return condition->toCps([this, k](Expr *cond) {
    const char *cvar = genSymbol("I");
    GroupingExpr *cast = makeContinuation(k);
    K newK = [this, cvar](Expr *ifResult) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), NULL);

      return new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), ifResult, NULL);
    };
    DeclarationExpr *param = newDeclarationExpr(paramType, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
    Expr *ifExpr = new IfExpr(cond, this->thenBranch->toCps(newK), this->elseBranch->toCps(newK));

    return new CallExpr(false, makeWrapperLambda(param, ifExpr), buildToken(TOKEN_CALL, "("), cast, NULL);
  });*/
//  return k(this);
/*
function cps_if(exp, k) {
    return cps(exp.cond, function(cond){
        var cvar = gensym("I");
        function k2(ifresult) {
            return {
                type: "call",
                func: { type: "var", callee: cvar },
                args: [ ifresult ]
            };
        };
        return {
            type: "call",
            func: {
                type: "lambda",
                vars: [ cvar ],
                body: {type: "if", cond: cond, then: cps(exp.then, k2), else: cps(exp.else || FALSE, k2)}
            },
            args: [ make_continuation(k) ]
        };
    });
}*/
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
  if (declaration && !name.equal("handlerFn_") && !getDeclarationRef(name/*buildToken(TOKEN_IDENTIFIER, getDeclaration(declaration)->getRealName().c_str())*/, getCurrent()->group->declarations))
    getDeclaration(declaration)->isInternalField = true;

  return k(this);
}

Expr *SwapExpr::toCps(K k) {
  Expr *cpsExpr = _expr->toCps([this, k](Expr *_expr) -> Expr* {
    SwapExpr *swapExpr = compareExpr(this->_expr, _expr) ? this : (SwapExpr *) idExpr(new SwapExpr());

    if (swapExpr == this)
      return this;
    else {
      swapExpr->_expr = _expr;
      return k(swapExpr);
    }
  });

  return this == cpsExpr ? k(this) : cpsExpr;
}

Expr *NativeExpr::toCps(K k) {
  return k(this);
}
