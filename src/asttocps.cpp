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
        return k(compareExpr(this->varExp, varExp) && compareExpr(this->value, value) ? this : new AssignExpr((ReferenceExpr *) varExp, this->op, value));
      });
    else
      return k(compareExpr(this->varExp, varExp) ? this : new AssignExpr((ReferenceExpr *) varExp, this->op, NULL));
  });
}

Expr *CastExpr::toCps(K k) {
  return expr->toCps([this, k](Expr *expr) {
    return compareExpr(this->expr, expr) ? this : k(new CastExpr(typeExpr, expr));
  });
}

Expr *UIAttributeExpr::toCps(K k) {
  return k(this); // assume no cps for now
}

Expr *UIDirectiveExpr::toCps(K k) {
  return k(this); // assume no cps for now
}

Expr *BinaryExpr::toCps(K k) {
  return left->toCps([this, k](Expr *left) {
    return this->right->toCps([this, k, left](Expr *right) {
      return k(compareExpr(this->left, left) && compareExpr(this->right, right) ? this : new BinaryExpr(left, this->op, right));
    });
  });
}

Expr *CallExpr::toCps(K k) {
  Expr **arguments = RESIZE_ARRAY(Expr *, NULL, 0, count + 1);
  bool same = true;

  return callee->toCps([&same, arguments, this, k](Expr *callee) {
    same = compareExpr(this->callee, callee);

    std::function<Expr *(int)> loop = [&loop, &same, arguments, callee, this, k](int index) -> Expr * {
      if (index < this->count)
        return this->arguments[index]->toCps([&loop, &same, arguments, callee, this, k, index](Expr *expr) -> Expr * {
          same &= compareExpr(arguments[index], expr);
          arguments[index] = expr;
          return loop(index + 1);
        });
      else
        if (same)
          return this;
        else {
          const char *cont = genSymbol("R");
          const char *var = genSymbol("ret_");
          DeclarationExpr **expList = RESIZE_ARRAY(DeclarationExpr *, NULL, 0, 1);
          Expr **bodyExpr = RESIZE_ARRAY(Expr *, NULL, 0, 1);

          expList[0] = new DeclarationExpr(NULL/*this->handlerFunction->type*/, buildToken(TOKEN_IDENTIFIER, var, strlen(var), -1), NULL);
          bodyExpr[0] = k(expList[0]);
          arguments[index] = new FunctionExpr(NULL, buildToken(TOKEN_IDENTIFIER, cont, strlen(cont), -1), 1,// expList,
                                              new GroupingExpr(buildToken(TOKEN_LEFT_BRACKET, "{", 1, -1), 1, bodyExpr, NULL));
          return new CallExpr(callee, this->paren, this->count + 1, arguments, this->newFlag, NULL, NULL, NULL);
        }
    };

    return loop(0);
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
  return !initExpr ? this : initExpr->toCps([this, k](Expr *initExpr) {
    return k(compareExpr(this->initExpr, initExpr) ? this : new DeclarationExpr(this->typeExpr, this->name, initExpr));
  });
}

Expr *FunctionExpr::toCps(K k) {
  if (!_function.isClass())
    return this;

  const char *cont = genSymbol("K");
  Expr *bodyExpr = body->toCps([this, cont](Expr *body) {return body;});
  GroupingExpr *newBody = bodyExpr->type == EXPR_GROUPING ? (GroupingExpr *) bodyExpr : NULL;
  Expr **newParams  = RESIZE_ARRAY(Expr *, NULL, 0, arity + 1);

  memcpy(newParams, body->expressions, arity * sizeof(Expr *));
  newParams[arity] = new DeclarationExpr(typeExpr, buildToken(TOKEN_IDENTIFIER, cont, strlen(cont), -1), NULL);
  return k(compareExpr(body, bodyExpr) ? this : new FunctionExpr(typeExpr, name, arity + 1, newBody));
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
  std::function<Expr *(Expr *, int, int)> loop = [&loop, this, k](Expr *topExpr, int start, int index) -> Expr * {
    while (index < this->count) { // if
      Expr *cpsExpr = this->expressions[index]->toCps(index + 1 < this->count
        ? [start, &loop, this, k, topExpr, index](Expr *expr) -> Expr * {
            return this->expressions[index] != expr ? loop(expr, index + 1, index + 1) : expr;
          }
        : k);

      if (!compareExpr(this->expressions[index], cpsExpr)) {
        int count = index - start;
        int offset = topExpr ? 1 : 0;
        Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, offset + count + 1);

        if (offset)
          expList[0] = topExpr;

        memcpy(&expList[offset], &this->expressions[start], count * sizeof(Expr *));
        expList[offset + count] = cpsExpr;
        return new GroupingExpr(this->name, offset + count + 1, expList, NULL);
      }
      else
        index++;
    }

    if (index != this->count)
      return NULL;

    if (!topExpr)
      return this;
    else {
      int count = this->count - start;
      Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 1 + count);

      expList[0] = topExpr;
      memcpy(&expList[1], &this->expressions[start], count * sizeof(Expr *));
      return new GroupingExpr(this->name, 1 + count, expList, NULL);
    }
  };

  return loop(NULL, 0, 0);
/*
function cps_prog(exp, k) {
  if (exp.prog.length == 0) return k(FALSE);

  return (function loop(body){
    if (body.length == 1) return cps(body[0], k);
    return cps(body[0], function(first){
      return {
        type: "prog",
        prog: [ first, loop(body.slice(1)) ]
      };
    });
  })(exp.prog);
}*/
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
    return compareExpr(this->value, value) ? this : k(new ReturnExpr(this->keyword, value));
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
    return compareExpr(this->right, right) ? this : k(new UnaryExpr(this->op, right));
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
