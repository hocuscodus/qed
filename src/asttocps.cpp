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
    return compareExpr(this->expr, expr) ? this : k(newExpr(new CastExpr(typeExpr, expr)));
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
  if (!newFlag && handler) {
    FunctionExpr *func = (FunctionExpr *) handler;

    func->body->body = new BinaryExpr(k(func->params[0]), buildToken(TOKEN_SEPARATOR, ";", 1, -1), func->body->body);
  }

  auto genCall = [this, k](bool same, Expr *callee, Expr *&params) {
    bool userClass = this->newFlag || this->handler;

    if (this->handler) {
      addExpr(same ? &this->params : &params, this->handler, buildToken(TOKEN_COMMA, ",", -1, 1));
      this->handler = NULL;
    }

    Expr *call = same ? this : newExpr(new CallExpr(this->newFlag, callee, this->paren, params, NULL));

    return userClass ? call : k(call);
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

  const char *type = getHandlerType(typeExpr ? ((ReferenceExpr *) typeExpr)->returnType : VOID_TYPE);
  Token name = buildToken(TOKEN_IDENTIFIER, "HandlerFn_", strlen("HandlerFn_"), -1);
  ReferenceExpr *paramTypeExpr = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, type, strlen(type), -1), UNKNOWN_TYPE);

  if (_function.type.valueType != VAL_VOID) {
    if (typeExpr)
      delete typeExpr;

    typeExpr = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "void", 4, -1), VOID_TYPE);
  }

  params = RESIZE_ARRAY(DeclarationExpr *, params, arity, arity + 1);
  params[arity] = new DeclarationExpr(paramTypeExpr, name, NULL);

  if (getCurrent()) {
     params[arity++]->resolve(*((Parser *) NULL));

    if (!IS_UNKNOWN(((ReferenceExpr *) paramTypeExpr)->returnType))
      body->_compiler.addDeclaration(((ReferenceExpr *) paramTypeExpr)->returnType, name, NULL, false, NULL);
  }
  else
    arity++;

  Expr *currentBody = body;
  Expr *newBodyExpr = body->toCps([this](Expr *body) {return body;});
  body = (GroupingExpr *) newBodyExpr;// && newBodyExpr->type == EXPR_GROUPING ? (GroupingExpr *) newBodyExpr : NULL;

  Expr *lastExpr = *getLastBodyExpr(&body->body, TOKEN_SEPARATOR);

  if (!lastExpr || lastExpr->type != EXPR_RETURN)
    addExpr(&body->body, new ReturnExpr(buildToken(TOKEN_IDENTIFIER, "return", 6, -1), NULL, NULL), buildToken(TOKEN_SEPARATOR, ";", 1, -1));

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
  return cpsExpr->type == EXPR_GROUPING ? cpsExpr : newExpr(new GroupingExpr(this->name, cpsExpr));
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
}*/
}

Expr *WhileExpr::toCps(K k) {
  return k(this);
}

Expr *ReturnExpr::toCps(K k) {
  auto retCall = [this, k](Expr *value) {
    bool same = compareExpr(this->value, value);
    ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_", 5, -1), UNKNOWN_TYPE);
    Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "HandlerFn_", 10, -1), UNKNOWN_TYPE);

    if (value) {
      Token name = buildToken(TOKEN_IDENTIFIER, "Lambda", 6, -1);
      CallExpr *call = new CallExpr(false, param, buildToken(TOKEN_LEFT_PAREN, "(", 1, -1), value, NULL);
      ReturnExpr *ret = new ReturnExpr(buildToken(TOKEN_IDENTIFIER, "return", 6, -1), NULL, NULL);
      BinaryExpr *code = new BinaryExpr(call, buildToken(TOKEN_SEPARATOR, ";", 1, -1), ret);
      GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{", 1, -1), code);
      ReferenceExpr *voidType = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "void", 4, -1), VOID_TYPE);
      FunctionExpr *wrapperFunc = new FunctionExpr(voidType, name, 0, NULL, group, NULL);
      GroupingExpr *mainGroup = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "(", 1, -1), wrapperFunc);

      mainGroup->_compiler.pushScope(mainGroup);
      group->_compiler.pushScope(&wrapperFunc->_function, NULL);
      wrapperFunc->_declaration = getCurrent()->enclosing->checkDeclaration(OBJ_TYPE(&wrapperFunc->_function), name, &wrapperFunc->_function, NULL);
      group->_compiler.popScope();
      mainGroup->_compiler.popScope();
      param = mainGroup;
      value = NULL;
      this->value = NULL;
    }

    postExpr = newExpr(new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "(", 1, -1), param, NULL));
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
  return this;
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
    return compareExpr(this->right, right) ? this : k(newExpr(new UnaryExpr(this->op, right)));
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
