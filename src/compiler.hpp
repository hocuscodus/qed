/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_compiler_h
#define qed_compiler_h

#include <iostream>
#include <stack>
#include <functional>
#include "object.hpp"

class Parser;
struct ReferenceExpr;
struct GroupingExpr;

struct Compiler {
  Parser *parser;
  Compiler *enclosing;
  GroupingExpr *groupingExpr;
  ObjFunction *function = NULL;
  int vCount;
  int declarationCount;
  Declaration declarations[UINT8_COUNT];

  ObjFunction *compile(GroupingExpr *expr);
  void pushScope();
  Declaration *beginScope(ObjFunction *function);
  void beginScope();
  void endScope();

  Declaration *addDeclaration(Type type, Token &name, Declaration *previous, bool parentFlag);
  Type &peekDeclaration();
  int resolveReference(Token *name);
  int resolveReference2(Token *name);
  int resolveUpvalue(Token *name);
  int addUpvalue(uint8_t index, Declaration *declaration, bool isDeclaration);
  Type resolveReferenceExpr(ReferenceExpr *expr);
  void checkDeclaration(Type returnType, ReferenceExpr *expr, ObjFunction *signature);
  Declaration *checkDeclaration(Type returnType, Token &name, ObjFunction *signature);
  bool inBlock();

  static inline Compiler *getCurrent() {
    return current;
  }

  inline int getDeclarationCount() {
    return declarationCount;
  }

  inline Declaration &getDeclaration(int index) {
    return declarations[index];
  }
private:
  static Compiler *current;
};

bool identifiersEqual(Token *a, Token *b);
void pushSignature(ObjFunction *signature);
void popSignature();

static inline Compiler *getCurrent() {
  return Compiler::getCurrent();
}

struct Expr;

typedef std::function<Expr *(Expr *)> K;

#endif


