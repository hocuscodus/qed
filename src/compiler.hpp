/*
 * The QED Programming Language
 * Copyright (C) 2022  Hocus Codus Software inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http: *www.gnu.org/licenses/>.
 */
#ifndef qed_compiler_h
#define qed_compiler_h

#include <iostream>
#include "parser.hpp"

typedef struct {
  Type type;
  Token name;
  int depth;
  bool isField;
  int realIndex;
} Local;

struct Compiler {
  Parser &parser;
  std::string prefix;
  Compiler *enclosing;
  ObjFunction *function = NULL;
  Local locals[UINT8_COUNT];
  int localStart;
  int localCount;
  int scopeDepth;

  Compiler(Parser &parser);
  Compiler(ObjFunction *function);
  Compiler();

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
  void resolveVariableExpr(VariableExpr *expr);
  bool inLocalBlock();

  static inline Compiler *getCurrent() {
    return current;
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


