/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_compiler_h
#define qed_compiler_h

#include <iostream>
#include "parser.hpp"

typedef struct {
  Type type;
  Token name;
  int depth;
  bool isCaptured;
} Local;

struct Compiler {
  std::string prefix;
  Parser &parser;
  Compiler *enclosing;
  ObjFunction *function = NULL;
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;

  Compiler(Parser &parser, Compiler *enclosing);

  ObjFunction *compile();

  void beginScope();
  int endScope();

  Local *addLocal(ValueType type);
  Local *addLocal(Type type);
  void setLocalObjType(ObjFunction *function);
  Type removeLocal();
  Local *peekLocal(int index);
  void setLocalName(Token *name);
  int resolveLocal(Token *name);
  int resolveUpvalue(Token *name);
  int addUpvalue(uint8_t index, Type type, bool isLocal);
};

bool identifiersEqual(Token *a, Token *b);

#endif


