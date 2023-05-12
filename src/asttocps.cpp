/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <cstdarg>
#include <stdio.h>
#include "expr.hpp"


Expr *AssignExpr::toCps(K k) {
  Expr *expr = varExp->toCps([this, k](Expr *left) {
    return this->value->toCps([this, k, left](Expr *right) {
      return this->varExp == left && this->value == right ? this : k(new AssignExpr((ReferenceExpr *) left, this->op, right, this->opCode));
    });
  });

  return expr;
}

Expr *UIAttributeExpr::toCps(K k) {
  return this; // assume no cps for now
}

Expr *UIDirectiveExpr::toCps(K k) {
  return this; // assume no cps for now
}

Expr *BinaryExpr::toCps(K k) {
  Expr *expr = left->toCps([this, k](Expr *left) {
    return this->right->toCps([this, k, left](Expr *right) {
      return this->left == left && this->right == right ? this : k(new BinaryExpr(left, this->op, right, this->opCode));
    });
  });

  return expr;
}

int GENSYM = 0;

std::string genSymbol(std::string name) {
    name = "β_" + name;
    name += ++GENSYM;

    return name;
}

Expr *makeContinuation(K k) {
  std::string cont = genSymbol("R");

  return NULL;//new FunctionExpr(type, name, count, [cont], k(new DeclarationExpr(cont)), NULL);
}

Expr *loop(CallExpr *callExpr, Expr *func, Expr **arguments, int index, K k) {
  if (index == callExpr->count) {
    arguments[index] = makeContinuation(k);
    return new CallExpr(func, callExpr->paren, callExpr->count, arguments, callExpr->newFlag, NULL, NULL, NULL);
  }

  return arguments[index]->toCps([callExpr, func, arguments, index, k](Expr *value) {
    arguments[index] = value;
    return loop(callExpr, func, arguments, index + 1, k);
  });
}

Expr *CallExpr::toCps(K k) {
  return callee->toCps([this, k](Expr *func){
    return (loop(this, func, {}, 0, k));
  });
}

Expr *ArrayElementExpr::toCps(K k) {
  return this;
}

Expr *DeclarationExpr::toCps(K k) {
  return this;
}

Expr *FunctionExpr::toCps(K k) {
  return this;
}

Expr *GetExpr::toCps(K k) {
//  return parenthesize2(".", expr.object, expr.name.lexeme);
  printf("(. ");
  object->toCps(k);
  printf(" %.*s)", name.length, name.start);
  return this;
}

Expr *GroupingExpr::toCps(K k) {
  printf("(group'%.*s' ", name.length, name.start);
  for (int index = 0; index < count; index++) {
    printf(", ");
    expressions[index]->toCps(k);
  }
  if (ui)
    ui->toCps(k);
  printf(")");
  return this;
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
}

Expr *OpcodeExpr::toCps(K k) {
  printf("(Op %d ", op);
  if (right)
    right->toCps(k);
  printf(")");
  return this;
}

Expr *ReturnExpr::toCps(K k) {
  printf("return ");
  if (value != NULL)
    value->toCps(k);
  printf(";");
  return this;
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
  expr->toCps(k);
  printf(";");
  return this;
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
}

Expr *ThisExpr::toCps(K k) {
  printf("this");
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
  return this;
}

Expr *SwapExpr::toCps(K k) {
  return this;
}

Expr *NativeExpr::toCps(K k) {
  return this;
}
