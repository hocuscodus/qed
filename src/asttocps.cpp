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

std::string genSymbol(std::string name) {
    name = "β_" + name;
    name += ++GENSYM;

    return name;
}

Expr *AssignExpr::toCps(K k) {
  return varExp->toCps([this, k](Expr *left) {
    return this->value->toCps([this, k, left](Expr *right) {
      return this->varExp == left && this->value == right ? this : k(new AssignExpr((ReferenceExpr *) left, this->op, right, this->opCode));
    });
  });
}

Expr *UIAttributeExpr::toCps(K k) {
  return this; // assume no cps for now
}

Expr *UIDirectiveExpr::toCps(K k) {
  return this; // assume no cps for now
}

Expr *BinaryExpr::toCps(K k) {
  return left->toCps([this, k](Expr *left) {
    return this->right->toCps([this, k, left](Expr *right) {
      return this->left == left && this->right == right ? this : k(new BinaryExpr(left, this->op, right, this->opCode));
    });
  });
}

Expr *CallExpr::toCps(K k) {
  Expr **arguments = new Expr *[count + 1];
  bool same = true;

  return callee->toCps([&same, arguments, this, k](Expr *callee) {
    same = this->callee == callee;

    std::function<Expr *(int)> loop = [&loop, &same, arguments, callee, this, k](int index) -> Expr * {
      if (index < this->count)
        return this->arguments[index]->toCps([&loop, &same, arguments, callee, this, k, index](Expr *expr) -> Expr * {
          same &= arguments[index] == expr;
          arguments[index] = expr;
          return loop(index + 1);
        });
      else
        if (same)
          return this;
        else {
          std::string cont = genSymbol("R");

//          arguments[index] = new FunctionExpr(this->type, this->name, count, new Expr* {cont}, k(new DeclarationExpr(cont)), NULL);
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
  return initExpr->toCps([this, k](Expr *initExpr) {
    return this->initExpr == initExpr ? this : k(new DeclarationExpr(this->typeExpr, this->name, initExpr));
  });
}

Expr *FunctionExpr::toCps(K k) {
  std::string cont = genSymbol("K");
  Expr *bodyExpr = body->toCps([this, cont, k](Expr *body) {
    Expr **exprList = new Expr *{body};

    return this->body == body ? this : (Expr *) new CallExpr(new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, cont.c_str(), cont.size(), -1), -1, false), buildToken(TOKEN_LEFT_PAREN, "(", 1, -1), 1, exprList, false, NULL, NULL, NULL);
  });
  GroupingExpr *newBody = bodyExpr->type == EXPR_GROUPING ? (GroupingExpr *) bodyExpr : NULL;

  return bodyExpr == body ? this : k(new FunctionExpr(typeExpr, name, count + 1, params/*.concat(cont)*/, newBody, NULL));
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
  printf("(. ");
  object->toCps(k);
  printf(" %.*s)", name.length, name.start);
  return this;
}

Expr *GroupingExpr::toCps(K k) {;
  bool same = true;

  std::function<Expr *(int)> loop = [&same, &loop, this, k](int index) -> Expr * {
    while (index < this->count) { // if
      Expr *cpsExpr = this->expressions[index]->toCps([&same, &loop, this, k, index](Expr *expr) -> Expr * { // return this->...
        if (expr != this->expressions[index]) {
          Expr **expList = NULL;

          expList = RESIZE_ARRAY(Expr *, expList, 0, index + 1);
          memcpy(expList, this->expressions, index);
          same = false;

          if (index < this->count) {
            this->expressions[0] = expr;
            memmove(&this->expressions[1], &this->expressions[index], this->count -= index);
            expList[index] = loop(1);
          }
          else
            expList[index] = k(expr);

          return new GroupingExpr(this->name, index + 1, expList, 0, NULL);
        }
        else
          return expr;//loop(index + 1);
      });
/// more
      if (cpsExpr != this->expressions[index])
        break;
      else
        index++;
    }

    if (index == this->count)
      if (same)
        return k(this);
      else {
        Expr **expList = NULL;

        expList = RESIZE_ARRAY(Expr *, expList, 0, this->count);
        memcpy(expList, this->expressions, this->count);

        return k(new GroupingExpr(this->name, this->count, expList, 0, NULL));
      }
  };

  return loop(0);
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

Expr *OpcodeExpr::toCps(K k) {
  return this;
}

Expr *ReturnExpr::toCps(K k) {
  return value->toCps([this, k](Expr *value) {
    return this->value == value ? this : k(new ReturnExpr(this->keyword, value));
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

Expr *StatementExpr::toCps(K k) {
  return expr->toCps([this, k](Expr *expr) {
    return this->expr == expr ? this : k(new StatementExpr(expr));
  });
}

Expr *SuperExpr::toCps(K k) {
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
  return this;
}

Expr *TypeExpr::toCps(K k) {
  return this;
}

Expr *UnaryExpr::toCps(K k) {
  return right->toCps([this, k](Expr *right) {
    return this->right == right ? this : k(new UnaryExpr(this->op, right));
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
