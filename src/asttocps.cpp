/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <cstdarg>
#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "expr.hpp"


int GENSYM = 0;

Expr *newExpr(Expr *newExp) {
  return newExp;
}

char *genSymbol(std::string name) {
    std::string s = name + std::to_string(++GENSYM) + "$_";
    char *str = new char[s.size() + 1];

    strcpy(str, s.c_str());
    return str;
}

GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, Type paramType, std::function<Expr*()> bodyFn) {
  int arity = param ? 1 : 0;
  Token nameToken = buildToken(TOKEN_IDENTIFIER, name);
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), NULL);
  DeclarationExpr **params = RESIZE_ARRAY(DeclarationExpr *, NULL, 0, arity);
  FunctionExpr *wrapperFunc = new FunctionExpr(VOID_TYPE, nameToken, arity, params, group, NULL);
  GroupingExpr *mainGroup = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), wrapperFunc);

  wrapperFunc->_function.expr = wrapperFunc;
  mainGroup->_compiler.pushScope(mainGroup);
  group->_compiler.pushScope(&wrapperFunc->_function, NULL);

  if (param) {
    params[0] = param;
    group->_compiler.addDeclaration(paramType, param->name, NULL, false, NULL);
  }

  wrapperFunc->_declaration = getCurrent()->enclosing->checkDeclaration(OBJ_TYPE(&wrapperFunc->_function), nameToken, &wrapperFunc->_function, NULL);
  group->body = bodyFn();
  group->_compiler.popScope();
  mainGroup->_compiler.popScope();
  return mainGroup;
}

GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, Type paramType, Expr *body) {
  return makeWrapperLambda(name, param, paramType, [body]() -> Expr* {return body;});
}

GroupingExpr *makeWrapperLambda(DeclarationExpr *param, Type paramType, std::function<Expr*()> bodyFn) {
  return makeWrapperLambda(genSymbol("W"), param, paramType, bodyFn);
}

GroupingExpr *makeContinuation(K k, bool boolParam) {
  DeclarationExpr *param = boolParam ? new DeclarationExpr(NULL, buildToken(TOKEN_IDENTIFIER, "arg"), NULL) : NULL;
  ReferenceExpr *arg = boolParam ? new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "arg"), BOOL_TYPE) : NULL;

  return makeWrapperLambda(genSymbol("C"), param, BOOL_TYPE, [k, arg]() -> Expr* {return k(arg);});
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

bool endsWith1(std::string const &str, std::string const &suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

const char *getHandlerType(Type type) {
  switch (type.valueType) {
    case VAL_VOID: return "VoidHandler_";
    case VAL_BOOL: return "BoolHandler_";
    case VAL_INT: return "IntHandler_";
    case VAL_FLOAT: return "FloatHandler_";
    case VAL_OBJ: return type.objType->type == OBJ_STRING ? "StringHandler_" : NULL;
    default: return NULL;
  }
}

Expr *AssignExpr::toCps(K k) {
  return varExp->toCps([this, k](Expr *varExp) {
    return this->value
      ? this->value->toCps([this, k, varExp](Expr *value) {
          return k(compareExpr(this->varExp, varExp) && compareExpr(this->value, value) ? this : newExpr(new AssignExpr(varExp, this->op, value)));
        })
      : k(compareExpr(this->varExp, varExp) ? this : newExpr(new AssignExpr(varExp, this->op, NULL)));
  });
}

Expr *CastExpr::toCps(K k) {
  return expr->toCps([this, k](Expr *expr) {
    return k(compareExpr(this->expr, expr) ? this : newExpr(new CastExpr(typeExpr, expr)));
  });
}

Expr *UIAttributeExpr::toCps(K k) {
  return k(this); // assume no cps for now
}

Expr *UIDirectiveExpr::toCps(K k) {
  return k(this); // assume no cps for now
}

Expr *BinaryExpr::toCps(K k) {
  auto genBinary = [this](Expr *left, Expr *right) {
    return !left ? right : compareExpr(this->left, left) && compareExpr(this->right, right) ? this : newExpr(new BinaryExpr(left, this->op, right));
  };

  if (op.type == TOKEN_SEPARATOR) {
    Expr *exp = left->toCps([this, genBinary, k](Expr *left) {
      while (this->right) {
        Expr *next = car(this->right, TOKEN_SEPARATOR);
        FunctionExpr *func = next->type == EXPR_FUNCTION ? (FunctionExpr *) next : NULL;

        if (func && getCurrent() != func->body->_compiler.enclosing) {
          addExpr(&func->body->_compiler.enclosing->funcs, func, buildToken(TOKEN_SEPARATOR, ";"));
          this->right = removeExpr(this->right, TOKEN_SEPARATOR);
        }
        else
          break;
      }

      return this->right ? genBinary(left, this->right->toCps(k)) : getCompareExpr(this->left, left);
    });

    if (exp->type != EXPR_BINARY) {
      if (getCurrent()->funcs) {
        exp = new BinaryExpr(exp, op, getCurrent()->funcs->toCps([](Expr *expr) {
          return expr;
        }));
        getCurrent()->funcs = NULL;
      }

      delete this;
    }

    return exp;
  }
  else
    return left->toCps([this, genBinary, k](Expr *left) {
      return this->right->toCps([this, genBinary, k, left](Expr *right) {
        return k(genBinary(left, right));
      });
    });
}

Expr *CallExpr::toCps(K k) {
  bool userClassCall = !newFlag && handler;

  auto genCall = [this, userClassCall, k](bool same, Expr *callee, Expr *&params) {
    if (userClassCall) {
      if (handler->type != EXPR_GROUPING)
        handler = NULL;

      GroupingExpr *group = (GroupingExpr *) handler;
      FunctionExpr *func = (FunctionExpr *) group->body;

      group->_compiler.pushScope(group);
      func->body->_compiler.pushScope(&func->_function, NULL);

      Expr *param = func->arity ? new ReferenceExpr(func->params[0]->name, UNKNOWN_TYPE) : NULL;
      Expr *newBody = newExpr(k(param));

      func->body->body = func->body->body ? new BinaryExpr(newBody, buildToken(TOKEN_SEPARATOR, ";"), func->body->body) : newBody;
      func->body->_compiler.popScope();
      group->_compiler.popScope();
    }

    if (this->handler) {
      addExpr(same && !userClassCall ? &this->params : &params, this->handler, buildToken(TOKEN_COMMA, ","));
      this->handler = NULL;
    }

    Expr *call = same && !userClassCall ? this : newExpr(new CallExpr(this->newFlag, callee, this->paren, params, NULL));

    return userClassCall ? call : k(call);
  };

  return callee->toCps([this, genCall](Expr *callee) {
    return this->params
      ? this->params->toCps([this, genCall, callee](Expr *params) {
          return genCall(compareExpr(this->callee, callee) && compareExpr(this->params, params), callee, params);
        })
      : genCall(compareExpr(this->callee, callee), callee, this->params);
  });
}

Expr *ArrayElementExpr::toCps(K k) {
  return this;
}

Expr *DeclarationExpr::toCps(K k) {
  return !initExpr ? k(this) : initExpr->toCps([this, k](Expr *initExpr) {
    return k(compareExpr(this->initExpr, initExpr) ? this : newExpr([this, initExpr]() {
      DeclarationExpr *expr = new DeclarationExpr(this->typeExpr, this->name, initExpr);

      expr->_declaration = this->_declaration;
      return expr;
    }()));
  });
}

Expr *FunctionExpr::toCps(K k) {
  if (_function.isClass() && !endsWith1(name.getString(), "_")) {
    body->_compiler.pushScope();
    const char *type = getHandlerType(returnType);
    Token name = buildToken(TOKEN_IDENTIFIER, "HandlerFn_");
    ReferenceExpr *paramTypeExpr = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, type), UNKNOWN_TYPE);

    returnType = VOID_TYPE;
    params = RESIZE_ARRAY(DeclarationExpr *, params, arity, arity + 1);
    params[arity] = new DeclarationExpr(paramTypeExpr, name, NULL);

    if (getCurrent()) {
      params[arity++]->resolve(*((Parser *) NULL));

      if (!IS_UNKNOWN(((ReferenceExpr *) paramTypeExpr)->returnType))
        body->_compiler.addDeclaration(((ReferenceExpr *) paramTypeExpr)->returnType, name, NULL, false, NULL);
    }
    else
      arity++;
  }

  body->_compiler.popScope();

  Expr *currentBody = body;
  Expr *newBodyExpr = body->toCps([this](Expr *body) {return body;});
  body = (GroupingExpr *) newBodyExpr;// && newBodyExpr->type == EXPR_GROUPING ? (GroupingExpr *) newBodyExpr : NULL;

  Expr *lastExpr = *getLastBodyExpr(&body->body, TOKEN_SEPARATOR);

//  if (!lastExpr || lastExpr->type != EXPR_RETURN)
//    addExpr(&body->body, new ReturnExpr(buildToken(TOKEN_IDENTIFIER, "return"), NULL, NULL), buildToken(TOKEN_SEPARATOR, ";"));

  return k(this);//compareExpr(body, bodyExpr) ? this : newExpr(new FunctionExpr(typeExpr, name, arity + 1, newParams, newBody, NULL)));
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
  return object->toCps([this, k](Expr *object) {
    return k(compareExpr(this->object, object) ? this : newExpr(new GetExpr(object, this->name, this->index)));
  });
}

Expr *GroupingExpr::toCps(K k) {
  _compiler.pushScope();

  Expr *body = this->body 
    ? this->body->toCps([this, k](Expr *body) {
        return body;
      })
    : NULL;

  _compiler.popScope();

  GroupingExpr *group = compareExpr(this->body, body) ? this : (GroupingExpr *) newExpr(new GroupingExpr(this->name, body));

  if (group != this)
    group->_compiler = _compiler;

  return k(group);
}

Expr *ArrayExpr::toCps(K k) {
  printf("[");
  for (int index = 0; index < count; index++) {
    if (index) printf(", ");
    expressions[index]->toCps(k);
  }
  printf("]");
  return this;
}

Expr *ListExpr::toCps(K k) {
  printf("(list ");
  for (int index = 0; index < count; index++) {
    printf(", ");
    expressions[index]->toCps(k);
  }
  printf(")");
  return this;
}

Expr *LiteralExpr::toCps(K k) {
  return k(this);
}

Expr *LogicalExpr::toCps(K k) {
  return left->toCps([this, k](Expr *left) -> Expr* {
    const char *cvar = genSymbol("I");
    bool rightEqual = true;
    Expr *right = this->right->toCps([this, &rightEqual, cvar](Expr *right) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), UNKNOWN_TYPE);

      rightEqual = compareExpr(this->right, right);
      return new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), right, NULL);
    });

    if (rightEqual) {
      // delete right
      return k(compareExpr(this->left, left) ? this : newExpr(new LogicalExpr(left, this->op, this->right)));
    } else {
      // delete this->right
      DeclarationExpr *param = new DeclarationExpr(NULL, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      Expr *left2 = getCompareExpr(this->left, left);
      Expr *notExpr = this->op.type == TOKEN_OR_OR ? new UnaryExpr(buildToken(TOKEN_BANG, "!"), left2) : left2;
      ReferenceExpr *callee2 = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), UNKNOWN_TYPE);
      Expr *elseExpr = newExpr(new CallExpr(false, callee2, buildToken(TOKEN_LEFT_PAREN, "("), new LiteralExpr(VAL_BOOL, {.boolean = false}), NULL));
      std::function<Expr*()> bodyFn = [notExpr, right, elseExpr]() -> Expr* {
        Expr *expr = new IfExpr(notExpr, right, elseExpr);

        expr->resolve(*((Parser *) NULL));
        return expr;
      };

      GroupingExpr *cast = makeContinuation(k, true);
      Type contType = {VAL_OBJ, &((FunctionExpr *) cast->body)->_function.obj};
      Expr *wrapperLambda = makeWrapperLambda(param, contType, bodyFn);

      return newExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_LEFT_PAREN, "("), cast, NULL));
    }
  });/*
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
  char *newSymbol = NULL;
  Expr *condition = this->condition->toCps([this, &newSymbol, k](Expr *condition) {
    bool sameCondition = compareExpr(this->condition, condition);
    Expr *body = this->body->toCps([this, &newSymbol, sameCondition](Expr *body) {
      if (!compareExpr(this->body, body) || !sameCondition) {
        newSymbol = genSymbol("While");
        ReferenceExpr *arg = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, newSymbol), UNKNOWN_TYPE);
        ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), UNKNOWN_TYPE);
        CallExpr *call = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), arg, NULL);

        addExpr(&body, call, buildToken(TOKEN_SEPARATOR, ";"));
      }

      return body;
    });

    return this->body == body ? this->condition : new IfExpr(condition, body, k(NULL));
  });

  return this->condition == condition
           ? k(this)
           : newExpr(new CallExpr(false, makeWrapperLambda(newSymbol, NULL, UNKNOWN_TYPE, condition), buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL));
}

Expr *ReturnExpr::toCps(K k) {
  auto retCall = [this, k](Expr *value) {
    bool same = compareExpr(this->value, value);

    if (getCurrent()->function->isUserClass()) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), UNKNOWN_TYPE);
      Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "HandlerFn_"), UNKNOWN_TYPE);
      CallExpr *call = new CallExpr(false, param, buildToken(TOKEN_LEFT_PAREN, "("), value, NULL);
      ReturnExpr *ret = new ReturnExpr(buildToken(TOKEN_IDENTIFIER, "return"), NULL, NULL);
      BinaryExpr *code = new BinaryExpr(call, buildToken(TOKEN_SEPARATOR, ";"), ret);

      param = makeWrapperLambda("Lambda_", NULL, UNKNOWN_TYPE, code);
      value = NULL;
      this->value = NULL;
      postExpr = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), param, NULL);
      postExpr->resolve(*((Parser *) NULL));
    }

    return k(same ? this : newExpr(new ReturnExpr(this->keyword, postExpr, value)));
  };

  return !value ? retCall(NULL) : value->toCps([this, retCall, k](Expr *value) {
    return retCall(value);
  });
}

Expr *SetExpr::toCps(K k) {
  return object->toCps([this, k](Expr *object) {
    return this->value->toCps([this, k, object](Expr *value) {
      return k(compareExpr(this->object, object) && compareExpr(this->value, value) ? this : newExpr(new SetExpr(object, this->name, this->op, value, this->index)));
    });
  });
}

Expr *IfExpr::toCps(K k) {
  return condition->toCps([this, k](Expr *condition) -> Expr* {
    const char *cvar = genSymbol("I");
    bool thenEqual = true;
    bool elseEqual = true;

    Expr *thenBranch = this->thenBranch->toCps([this, &thenEqual, cvar](Expr *thenBranch) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), UNKNOWN_TYPE);

      thenEqual = compareExpr(this->thenBranch, thenBranch);
      return new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);
    });
    Expr *elseBranch = this->elseBranch ? this->elseBranch->toCps([this, &elseEqual, cvar](Expr *elseBranch) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), UNKNOWN_TYPE);

      elseEqual = compareExpr(this->elseBranch, elseBranch);
      return new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);
    }) : NULL;

    if (thenEqual && elseEqual) {
      // delete cvar, thenBranch and elseBranch
      return k(compareExpr(this->condition, condition) ? this : newExpr(new IfExpr(condition, this->thenBranch, this->elseBranch)));
    } else {
      // delete this->thenBranch, this->elseBranch
      DeclarationExpr *param = new DeclarationExpr(NULL, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      std::function<Expr*()> bodyFn = [this, condition, thenEqual, elseEqual, thenBranch, elseBranch]() -> Expr* {
        Expr *expr = new IfExpr(condition, thenEqual ? this->thenBranch : thenBranch, elseEqual ? this->elseBranch : elseBranch);

        expr->resolve(*((Parser *) NULL));
        return expr;
      };

      GroupingExpr *cast = makeContinuation(k, false);
      Type contType = {VAL_OBJ, &((FunctionExpr *) cast->body)->_function.obj};
      Expr *wrapperLambda = makeWrapperLambda(param, contType, bodyFn);

      return newExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_LEFT_PAREN, "("), cast, NULL));
    }
  });
/*
  return condition->toCps([this, k](Expr *cond) {
    const char *cvar = genSymbol("I");
    GroupingExpr *cast = makeContinuation(k);
    K newK = [this, cvar](Expr *ifResult) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), VOID_TYPE);

      return new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), ifResult, NULL);
    };
    DeclarationExpr *param = new DeclarationExpr(NULL, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
    Expr *ifExpr = new IfExpr(cond, this->thenBranch->toCps(newK), this->elseBranch->toCps(newK));

    return new CallExpr(false, makeWrapperLambda(param, paramType, ifExpr), buildToken(TOKEN_LEFT_PAREN, "("), cast, NULL);
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
  return left->toCps([this, k](Expr *left) -> Expr* {
    Expr *testExpr;
    const char *cvar = genSymbol("I");
    bool bothEqual = true;
    K newK = [this, &testExpr, &bothEqual, cvar](Expr *ifResult) {
      ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cvar), UNKNOWN_TYPE);

      bothEqual &= compareExpr(testExpr, ifResult);
      return new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), ifResult, NULL);
    };

    Expr *middle = (testExpr = this->middle)->toCps(newK);
    Expr *right = (testExpr = this->right)->toCps(newK);

    if (bothEqual) {
      // delete middle and right
      return k(compareExpr(this->left, left) ? this : newExpr(new TernaryExpr(this->op, left, this->middle, this->right)));
    } else {
      // delete this->middle, this->right
      DeclarationExpr *param = new DeclarationExpr(NULL, buildToken(TOKEN_IDENTIFIER, cvar), NULL);
      std::function<Expr*()> bodyFn = [left, middle, right]() -> Expr* {
        Expr *expr = new IfExpr(left, middle, right);

        expr->resolve(*((Parser *) NULL));
        return expr;
      };

      GroupingExpr *cast = makeContinuation(k, true);
      Type contType = {VAL_OBJ, &((FunctionExpr *) cast->body)->_function.obj};
      Expr *wrapperLambda = makeWrapperLambda(param, contType, bodyFn);

      return newExpr(new CallExpr(false, wrapperLambda, buildToken(TOKEN_LEFT_PAREN, "("), cast, NULL));
    }
  });
}

Expr *ThisExpr::toCps(K k) {
  return k(this);
}

Expr *TypeExpr::toCps(K k) {
  return k(this);
}

Expr *UnaryExpr::toCps(K k) {
  return right->toCps([this, k](Expr *right) {
    return k(compareExpr(this->right, right) ? this : newExpr(new UnaryExpr(this->op, right)));
  });
}

Expr *ReferenceExpr::toCps(K k) {
  return k(this);
}

Expr *SwapExpr::toCps(K k) {
  return _expr->toCps([this, k](Expr *_expr) {
    SwapExpr *swapExpr = compareExpr(this->_expr, _expr) ? this : (SwapExpr *) newExpr(new SwapExpr());

    if (swapExpr != this)
      swapExpr->_expr = _expr;

    return k(swapExpr);
  });
}

Expr *NativeExpr::toCps(K k) {
  return k(this);
}
