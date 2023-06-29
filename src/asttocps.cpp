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

Expr *AssignExpr::toCps(K k) {
  return varExp->toCps([this, k](Expr *varExp) {
    if (this->value)
      return this->value->toCps([this, k, varExp](Expr *value) {
        return k(compareExpr(this->varExp, varExp) && compareExpr(this->value, value) ? this : newExpr(new AssignExpr((ReferenceExpr *) varExp, this->op, value)));
      });
    else
      return k(compareExpr(this->varExp, varExp) ? this : newExpr(new AssignExpr((ReferenceExpr *) varExp, this->op, NULL)));
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
  if (op.type == TOKEN_SEPARATOR)
    return left->toCps([this, k](Expr *left) {
      Expr *right = this->right->toCps(k);

      return compareExpr(this->left, left) && compareExpr(this->right, right) ? this : newExpr(new BinaryExpr(left, this->op, right));
    });
    /*
    function cps_prog(exp, k) {
      return cps(body[0], function(first){
        return {type: "prog", prog: [first, cps(body[1], k)]};
      });
    }*/

  return left->toCps([this, k](Expr *left) {
    return this->right->toCps([this, k, left](Expr *right) {
      return k(compareExpr(this->left, left) && compareExpr(this->right, right) ? this : newExpr(new BinaryExpr(left, this->op, right)));
    });
  });
}

Expr *CallExpr::toCps(K k) {
  const char *cont = genSymbol("R");
  const char *var = genSymbol("ret_");
  DeclarationExpr **expList = RESIZE_ARRAY(DeclarationExpr *, NULL, 0, 1);

  expList[0] = new DeclarationExpr(NULL/*this->handlerFunction->type*/, buildToken(TOKEN_IDENTIFIER, var, strlen(var), -1), NULL);

  Expr *handlerFunction = new FunctionExpr(NULL, buildToken(TOKEN_IDENTIFIER, cont, strlen(cont), -1), 1, expList,
                                           new GroupingExpr(buildToken(TOKEN_LEFT_BRACKET, "{", 1, -1), k(expList[0])), NULL);

  return callee->toCps([this, handlerFunction, k](Expr *callee) {
    bool same = compareExpr(this->callee, callee);

    if (this->params)
      this->params->toCps([this, &same, handlerFunction, callee, k](Expr *params) {
        same &= compareExpr(this->params, params);
        addExpr(&params, handlerFunction, buildToken(TOKEN_COMMA, ",", -1, 1));
        return same ? this : newExpr(new CallExpr(this->newFlag, callee, this->paren, params, this->handler));
      });
    else {
      addExpr(&params, handlerFunction, buildToken(TOKEN_COMMA, ",", -1, 1));
      return same ? this : newExpr(new CallExpr(this->newFlag, callee, this->paren, NULL, this->handler));
    }
  });
/*
function cps_call(exp, k) {
  return cps(exp.func, function(func){
    return (function loop(args, i){
      if (i == exp.args.length) return {
        type : "call",
        func : func,
        args : args
      };
      return cps(exp.args[i], function(value){
        args[i + 1] = value;
        return loop(args, i + 1);
      });
    })([ make_continuation(k) ], 0);
  });
}*/
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
  if (!_function.isClass())
    return this;

  const char *cont = genSymbol("K");
  Expr *currentBody = body;
  Expr *newBodyExpr = body->toCps([this, cont](Expr *body) {return body;});

  body = (GroupingExpr *) newBodyExpr;// && newBodyExpr->type == EXPR_GROUPING ? (GroupingExpr *) newBodyExpr : NULL;
  params  = RESIZE_ARRAY(DeclarationExpr *, params, arity, arity + 1);
  params[arity] = new DeclarationExpr(typeExpr, buildToken(TOKEN_IDENTIFIER, cont, strlen(cont), -1), NULL);
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
//  return parenthesize2(".", expr.object, expr.name.lexeme);
  object->toCps(k);
  return this;
}

Expr *GroupingExpr::toCps(K k) {
  if (body)
    return body->toCps([this, k](Expr *body) {
      return compareExpr(this->body, body) ? this : k(newExpr(new GroupingExpr(this->name, body)));
    });
  else
    return k(this);
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
  return this;
}

Expr *ReturnExpr::toCps(K k) {
//    Expr **expList = new Expr *{body};

//    return  ? body : (Expr *) new CallExpr(new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cont, strlen(cont), -1), -1, false), buildToken(TOKEN_LEFT_PAREN, "(", 1, -1), 1, expList, false, NULL, NULL, NULL);
  return value->toCps([this, k](Expr *value) {
    return compareExpr(this->value, value) ? this : k(newExpr(new ReturnExpr(this->keyword, value)));
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
  return this;
}
