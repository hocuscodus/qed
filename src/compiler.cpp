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
#include <stack>
#include "compiler.hpp"
#include "resolver.hpp"
#include "codegen.hpp"
#include "astprinter.hpp"
#include "object.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif

static std::stack<ObjCallable *> signatures;

void pushSignature(ObjCallable *signature) {
  signatures.push(signature);
}

void popSignature() {
  signatures.pop();
}

static ObjCallable *getSignature() {
  return signatures.empty() ? NULL : signatures.top();
}

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
  else {
    disassembleChunk(&function->chunk, function->name != NULL ? function->name->chars : "<script>");
    for (int index = 0; index < localCount; index++) {
      const char *name = locals[index].name.start;

      printf("<%s %s> ", locals[index].type.toString(), name ? name : "");
    }
    printf("\n");
  }
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

  int fieldIndex = 0;
  int localIndex = 0;

  for (int index = 0; index < popLevels; index++) {
    Local *local = &locals[localCount + index];

    local->realIndex = local->isField ? fieldIndex++ : localIndex++;

//    if (IS_FUNCTION(local->type))
    for (int i = 0; i < function->upvalueCount; i++) {
      Upvalue *upvalue = &function->upvalues[i];

      if (upvalue->isLocal) {
        if (locals[upvalue->index].isField)
          upvalue->index = upvalue->index;
        else
          upvalue->index = upvalue->index;
        upvalue->index = upvalue->index;//locals[upvalue->index].realIndex;
      }
    }
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
  local->isField = false;
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
  int found = -1;
  char buffer[2048] = "";
  ObjCallable *signature = getSignature();

  for (int i = localCount - 1; found < 0 && i >= 0; i--) {
    Local *local = &locals[i];

    if (identifiersEqual(name, &local->name)) {// && local->depth != -1)
      Type type = local->type;

//      if (signature && type.valueType == VAL_OBJ)
      // Remove these patches ASAP
//      if (signature && type.valueType == VAL_OBJ && name->getString().find("Instance") == std::string::npos)
      if (signature && IS_OBJ(type) && name->getString().find("Instance") == std::string::npos && strcmp(name->getString().c_str(), "post"))
        switch (AS_OBJ_TYPE(type)) {
        case OBJ_FUNCTION: {
          ObjCallable *callable = AS_FUNCTION_TYPE(type);
          bool isSignature = signature->arity == callable->arity;

          for (int index = 0; isSignature && index < signature->arity; index++)
            isSignature = signature->fields[index].type.equals(callable->fields[index + 1].type);

          found = isSignature ? i : -2; // -2 = found name, no good signature yet

          if (found == -2) {
            char buf[512];

            sprintf(buf, "'%.*s(", name->length, name->start);

            for (int index = 0; index < callable->arity; index++) {
              if (index)
                strcat(buf, ", ");

              strcat(buf, callable->fields[index + 1].type.toString());
            }

            strcat(buf, ")'");

            if (buffer[0])
              strcat(buffer, ", ");

            strcat(buffer, buf);
          }
          break;
        }
        case OBJ_FUNCTION_PTR: {
          ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;
          bool isSignature = signature->arity == callable->arity;

          for (int index = 0; isSignature && index < signature->arity; index++)
            isSignature = signature->fields[index].type.equals(callable->parms[index + 1]);

          found = isSignature ? i : -2; // -2 = found name, no good signature yet

          if (found == -2) {
            char buf[512];

            sprintf(buf, "'%.*s(", name->length, name->start);

            for (int index = 0; index < callable->arity; index++) {
              if (index)
                strcat(buf, ", ");

              strcat(buf, callable->parms[index + 1].toString());
            }

            strcat(buf, ")'");

            if (buffer[0])
              strcat(buffer, ", ");

            strcat(buffer, buf);
          }
          break;
        }
        default:
          parser.error("Variable '%.*s' is not a call.", name->length, name->start);
          return -2;
        }
      else
        found = i;
    }
  }

  if (found == -2) {
    char parms[512] = "";

    for (int index = 0; index < signature->arity; index++) {
      if (index)
        strcat(parms, ", ");

      strcat(parms, signature->fields[index].type.toString());
    }

    parser.error("Call '%.*s(%s)' does not match %s.", name->length, name->start, parms, buffer);
  }

  return found;
}

int Compiler::resolveUpvalue(Token *name) {
  if (enclosing != NULL) {
    int local = enclosing->resolveLocal(name);

    switch (local) {
    case -2:
      break;

    case -1: {
      int upvalue = enclosing->resolveUpvalue(name);

      if (upvalue != -1)
        return addUpvalue((uint8_t) upvalue, enclosing->function->upvalues[upvalue].type, false);
      break;
    }
    default:
      enclosing->locals[local].isField = true;
      return addUpvalue((uint8_t) local, enclosing->locals[local].type, true);
    }
  }

  return -1;
}

int Compiler::addUpvalue(uint8_t index, Type type, bool isLocal) {
  return function->addUpvalue(index, isLocal, type, parser);
}