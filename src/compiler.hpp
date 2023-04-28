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
#include "object.hpp"

class Parser;
struct ReferenceExpr;

struct Compiler {
  Parser *parser;
  std::string prefix;
  Compiler *enclosing;
  ObjFunction *function = NULL;
  int fieldCount;
  int declarationStart;
  int declarationCount;
  int vCount;
  std::stack<Type> typeStack;
  Declaration declarations[UINT8_COUNT];
  ObjFunction *compile(Parser &parser);

  Declaration *beginScope(ObjFunction *function);
  void beginScope();
  void endScope();

  void pushType(ValueType type);
  void pushType(Type type);
  Type popType();
  Declaration *addDeclaration(Type type, Token &name);
  Type &peekDeclaration();
  int resolveReference(Token *name);
  int resolveUpvalue(Token *name);
  int addUpvalue(uint8_t index, Declaration *declaration, bool isDeclaration);
  void resolveReferenceExpr(ReferenceExpr *expr);
  Compiler *peekReferenceExpr(ReferenceExpr *expr);
  void checkDeclaration(ReferenceExpr *expr);
  bool inBlock();

  static inline Compiler *getCurrent() {
    return current;
  }

  inline int getDeclarationCount() {
    return declarationStart + declarationCount + typeStack.size();
  }

  inline Declaration &getDeclaration(int index) {
    return declarations[index - declarationStart];
  }
private:
  static Compiler *current;
};

struct ObjCallable;

bool identifiersEqual(Token *a, Token *b);
void pushSignature(ObjCallable *signature);
void popSignature();


static inline Compiler *getCurrent() {
  return Compiler::getCurrent();
}

#endif


