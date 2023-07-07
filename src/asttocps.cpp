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

const char *genSymbol(std::string name) {
    std::string s = "$_" + name + std::to_string(++GENSYM);
    char *str = new char[s.size() + 1];

    strcpy(str, s.c_str());
    return str;
}

bool compareExpr(Expr *origExpr, Expr *newExpr) {
  bool equal = origExpr == newExpr;

  if (!equal)
;//    delete origExpr;

  return equal;
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
          return k(compareExpr(this->varExp, varExp) && compareExpr(this->value, value) ? this : newExpr(new AssignExpr((ReferenceExpr *) varExp, this->op, value)));
        })
      : k(compareExpr(this->varExp, varExp) ? this : newExpr(new AssignExpr((ReferenceExpr *) varExp, this->op, NULL)));
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
    return compareExpr(this->left, left) && compareExpr(this->right, right) ? this : newExpr(new BinaryExpr(left, this->op, right));
  };

  return left->toCps([this, genBinary, k](Expr *left) {
    return this->op.type == TOKEN_SEPARATOR 
             ? genBinary(left, this->right->toCps(k))
             : this->right->toCps([this, genBinary, k, left](Expr *right) {
                 return k(genBinary(left, right));
               });
  });
}

Expr *CallExpr::toCps(K k) {
  bool userClassCall = !newFlag && handler;

  if (userClassCall) {
    if (handler->type != EXPR_GROUPING)
      handler = NULL;

    FunctionExpr *func = (FunctionExpr *) ((GroupingExpr *) handler)->body;

    func->body->body = newExpr(new BinaryExpr(k(func->params[0]), buildToken(TOKEN_SEPARATOR, ";"), func->body->body));
  }

  auto genCall = [this, userClassCall, k](bool same, Expr *callee, Expr *&params) {
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
    return k(compareExpr(this->initExpr, initExpr) ? this : newExpr(new DeclarationExpr(this->typeExpr, this->name, initExpr)));
  });
}

Expr *FunctionExpr::toCps(K k) {
  if (!_function.isClass() || endsWith1(name.getString(), "_"))
    return k(this);

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
  object->toCps(k);
  return this;
}

Expr *GroupingExpr::toCps(K k) {
  _compiler.pushScope();

  Expr *cpsExpr = body 
    ? body->toCps([this, k](Expr *body) {
        return k(compareExpr(this->body, body) ? this : body);
      })
    : k(this);

  _compiler.popScope();

  GroupingExpr *group = cpsExpr == body ? this : (GroupingExpr *) newExpr(new GroupingExpr(this->name, cpsExpr));

  if (group != this)
    group->_compiler = _compiler;

  return group;
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
  printf("(%.*s ", op.length, op.start);
  left->toCps(k);
  printf(" ");
  right->toCps(k);
  printf(")");
  return this;
/*
function cps_if(exp, k) {
  return cps(exp.cond, function(cond){
    return {
      type: "if",
      cond: cond,
      then: cps(exp.then, k),
      else: cps(exp.else || FALSE, k),
    };
  });
}
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

// (function Lambda() {if (condition) {body; post_(Lambda)} else {k}})();
Expr *WhileExpr::toCps(K k) {
  Expr *condition = this->condition->toCps([this, k](Expr *condition) {
    bool sameCondition = compareExpr(this->condition, condition);
    Expr *body = this->body->toCps([this, sameCondition](Expr *body) {
      if (!compareExpr(this->body, body) || !sameCondition) {
        ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "Lambda11"), VOID_TYPE);
        CallExpr *call = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);

        addExpr(&body, call, buildToken(TOKEN_SEPARATOR, ";"));
      }

      return body;
    });

    return this->body == body ? this->condition : new IfExpr(condition, body, k(new LiteralExpr(VAL_INT, {.integer = 1111})));
  });

  if (this->condition == condition)
    return k(this);
  else {
    // (function Lambda() {if (condition) {body; post_(Lambda)} else {k}})();
    Token name = buildToken(TOKEN_IDENTIFIER, "Lambda11");
    GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), condition);
    FunctionExpr *wrapperFunc = new FunctionExpr(VOID_TYPE, name, 0, NULL, group, NULL);
    GroupingExpr *mainGroup = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), wrapperFunc);

    wrapperFunc->_function.expr = wrapperFunc;
    mainGroup->_compiler.pushScope(mainGroup);
    group->_compiler.pushScope(&wrapperFunc->_function, NULL);
    wrapperFunc->_declaration = getCurrent()->enclosing->checkDeclaration(OBJ_TYPE(&wrapperFunc->_function), name, &wrapperFunc->_function, NULL);
    group->_compiler.popScope();
    mainGroup->_compiler.popScope();
    return new CallExpr(false, mainGroup, buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);
  }
}

Expr *ReturnExpr::toCps(K k) {
  auto retCall = [this, k](Expr *value) {
    bool same = compareExpr(this->value, value);
    ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), UNKNOWN_TYPE);
    Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "HandlerFn_"), UNKNOWN_TYPE);

    if (value) {
      Token name = buildToken(TOKEN_IDENTIFIER, "Lambda");
      CallExpr *call = new CallExpr(false, param, buildToken(TOKEN_LEFT_PAREN, "("), value, NULL);
      ReturnExpr *ret = new ReturnExpr(buildToken(TOKEN_IDENTIFIER, "return"), NULL, NULL);
      BinaryExpr *code = new BinaryExpr(call, buildToken(TOKEN_SEPARATOR, ";"), ret);
      GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), code);
      FunctionExpr *wrapperFunc = new FunctionExpr(VOID_TYPE, name, 0, NULL, group, NULL);
      GroupingExpr *mainGroup = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), wrapperFunc);

      wrapperFunc->_function.expr = wrapperFunc;
      mainGroup->_compiler.pushScope(mainGroup);
      group->_compiler.pushScope(&wrapperFunc->_function, NULL);
      wrapperFunc->_declaration = getCurrent()->enclosing->checkDeclaration(OBJ_TYPE(&wrapperFunc->_function), name, &wrapperFunc->_function, NULL);
      group->_compiler.popScope();
      mainGroup->_compiler.popScope();
      param = mainGroup;
      value = NULL;
      this->value = NULL;
    }

    postExpr = newExpr(new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), param, NULL));
    postExpr->resolve(*((Parser *) NULL));
    return k(same ? this : newExpr(new ReturnExpr(this->keyword, postExpr, value)));
  };

  return !value ? retCall(NULL) : value->toCps([this, retCall, k](Expr *value) {
    return retCall(value);
  });
}

Expr *SetExpr::toCps(K k) {
  printf("(%.*s (. ", op.length, op.start);
  object->toCps(k);
  printf(" %.*s) ", name.length, name.start);
  value->toCps(k);
  printf(")");
  return this;
}

Expr *IfExpr::toCps(K k) {
//  parenthesize2("super", 1, method);
  return k(this);
}

Expr *TernaryExpr::toCps(K k) {
  printf("(%.*s ", op.length, op.start);
  left->toCps(k);
  printf(" ");
  middle->toCps(k);
  printf(" ");
  if (right)
    right->toCps(k);
  else
    printf("NULL");
  printf(")");
  return this;
/*
function cps_if(exp, k) {
  return cps(exp.cond, function(cond){
    return {
      type: "if",
      cond: cond,
      then: cps(exp.then, k),
      else: cps(exp.else || FALSE, k),
    };
  });
}*/
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
  return _expr->toCps(k);
}

Expr *NativeExpr::toCps(K k) {
  return k(this);
}
