/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_compiler_h
#define qed_compiler_h

#include <iostream>
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
  Declaration declarations[UINT8_COUNT];
  ObjFunction *compile(Parser &parser);

  void beginScope(ObjFunction *function);
  void beginScope();
  void endScope();

  Declaration *addDeclaration(ValueType type);
  Declaration *addDeclaration(Type type);
  Declaration *addDeclaration(Type type, Token &name);
  Type removeDeclaration();
  Declaration *peekDeclaration(int index);
  void setDeclarationName(Token *name);
  int resolveReference(Token *name);
  int resolveUpvalue(Token *name);
  int addUpvalue(uint8_t index, Type type, bool isDeclaration);
  void resolveReferenceExpr(ReferenceExpr *expr);
  void checkDeclaration(Token *name);
  bool inBlock();

  static inline Compiler *getCurrent() {
    return current;
  }

  inline int getDeclarationCount() {
    return declarationStart + declarationCount;
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


