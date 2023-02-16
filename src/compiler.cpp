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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.hpp"
#include "resolver.hpp"
#include "codegen.hpp"
#include "astprinter.hpp"
#include "object.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif

Compiler::Compiler(Parser &parser, Compiler *enclosing) : parser(parser) {
  ObjString *enclosingNameObj = enclosing ? enclosing->function->name : NULL;
  std::string enclosingName = enclosingNameObj ? std::string("_") + enclosingNameObj->chars : "";

  prefix = enclosing ? enclosing->prefix + enclosingName : "qni";
  localCount = 0;
  scopeDepth = 0;
  function = newFunction();
  this->enclosing = enclosing;
  addLocal({VAL_OBJ, &function->obj});
}

ObjFunction *Compiler::compile() {
  if (!parser.parse())
    return NULL;
#ifdef DEBUG_PRINT_CODE
  printf("Original parse: ");
  ASTPrinter().print(parser.expr);
#endif
  Resolver resolver(parser, parser.expr);

  if (!resolver.resolve(this))
    return NULL;
#ifdef DEBUG_PRINT_CODE
  printf("Adapted parse: ");
  ASTPrinter().print(parser.expr);
  printf("          ");
  for (int i = 0; i < localCount; i++) {
    Token *token = &locals[i].name;

    if (token != NULL)
      printf("[ %.*s;%d ]", token->length, token->start, locals[i].depth);
    else
      printf("[ N/A;%d ]", locals[i].depth);
  }
  printf("\n");
#endif
  CodeGenerator generator(parser, function);

  generator.emitCode(parser.expr);
  generator.endCompiler();

  if (parser.hadError)
    function->chunk.reset();
#ifdef DEBUG_PRINT_CODE
  else
    disassembleChunk(&function->chunk, function->name != NULL ? function->name->chars : "<script>");
#endif

  return parser.hadError ? NULL : function;
}

void Compiler::beginScope() {
  scopeDepth++;
}

int Compiler::endScope() {
  int popLevels = 0;

  scopeDepth--;

  while (localCount > 0 && locals[localCount - 1].depth > scopeDepth) {
    popLevels++;
    localCount--;
  }

  return popLevels;
}

Local *Compiler::addLocal(ValueType type) {
  return addLocal({type, NULL});
}

Local *Compiler::addLocal(Type type) {
  if (localCount == UINT8_COUNT) {
    parser.error("Too many local variables in function.");
    return NULL;
  }

  Local *local = &locals[localCount++];

  local->type = type;
  local->name.start = "";
  local->name.length = 0;
  local->depth = scopeDepth;
  local->isCaptured = false;
  return local;
}

Type Compiler::removeLocal() {
  return locals[--localCount].type;
}

Local *Compiler::peekLocal(int index) {
  return &locals[localCount - 1 - index];
}

void Compiler::setLocalName(Token *name) {
  locals[localCount - 1].name = *name;
}

void Compiler::setLocalObjType(ObjFunction *function) {
  locals[localCount - 1].type.objType = &function->obj;/*
  Type parms[function->arity];

  for (int index = 0; index < function->arity; index++)
    parms[index] = function->fields[index].type;

  locals[localCount - 1].type.objType = &newFunctionPtr(function->type, function->arity, parms)->obj;
*/
}

int Compiler::resolveLocal(Token *name) {
  for (int i = localCount - 1; i >= 0; i--) {
    Local *local = &locals[i];

    if (identifiersEqual(name, &local->name)) // && local->depth != -1)
      return i;
  }

  return -1;
}

int Compiler::resolveUpvalue(Token *name) {
  if (enclosing != NULL) {
    int local = enclosing->resolveLocal(name);

    if (local != -1) {
      enclosing->locals[local].isCaptured = true;
      return addUpvalue((uint8_t) local, enclosing->locals[local].type, true);
    }

    int upvalue = enclosing->resolveUpvalue(name);

    if (upvalue != -1)
      return addUpvalue((uint8_t) upvalue, enclosing->function->upvalues[upvalue].type, false);
  }

  return -1;
}

int Compiler::addUpvalue(uint8_t index, Type type, bool isLocal) {
  return function->addUpvalue(index, isLocal, type, parser);
}