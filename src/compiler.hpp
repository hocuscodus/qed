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
#include "scanner.hpp"

typedef struct {
  Type type;
  Token name;
  bool isField;
  int realIndex;
} Local;

class Parser;
struct VariableExpr;

struct Compiler {
  Parser *parser;
  std::string prefix;
  Compiler *enclosing;
  ObjFunction *function = NULL;
  int fieldCount;
  int realLocalStart;
  int realLocalCount;
  int localStart;
  int localCount;
  Local locals[UINT8_COUNT];
  ObjFunction *compile(Parser &parser);

  void beginScope(ObjFunction *function);
  void beginScope();
  void endScope();

  Local *addLocal(ValueType type);
  Local *addLocal(Type type);
  void setLocalObjType(ObjFunction *function);
  Type removeLocal();
  Local *peekLocal(int index);
  void setLocalName(Token *name);
  int resolveLocal(Token *name);
  int resolveUpvalue(Token *name);
  int addUpvalue(uint8_t index, Type type, bool isLocal);
  void resolveVariableExpr(VariableExpr *expr);
  void checkDeclaration(Token *name);
  bool inLocalBlock();

  static inline Compiler *getCurrent() {
    return current;
  }

  inline int getLocalCount() {
    return localStart + localCount;
  }

  inline Local &getLocal(int index) {
    return locals[index - localStart];
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


