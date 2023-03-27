/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stack>
#include "compiler.hpp"
#include "resolver.hpp"
#include "reifier.hpp"
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

Compiler *Compiler::current = NULL;

ObjFunction *Compiler::compile(Parser &parser) {
  this->parser = &parser;
  beginScope(newFunction({VAL_VOID}, NULL, 0));

#ifdef DEBUG_PRINT_CODE
  printf("Original parse: ");
  ASTPrinter().print(parser.expr);
#endif
  Resolver resolver(parser, parser.expr);
  Reifier reifier(parser);

  if (!resolver.resolve(this))
    return NULL;

  if (!reifier.reify())
    return NULL;
#ifdef DEBUG_PRINT_CODE
  printf("Adapted parse: ");
  ASTPrinter().print(parser.expr);
  printf("          ");
  for (int i = 0; i < localCount; i++) {
    Token *token = &locals[i].name;

    if (token != NULL)
      printf("[ %.*s ]", token->length, token->start);
    else
      printf("[ N/A ]");
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
  current = enclosing;

  return parser.hadError ? NULL : function;
}

void Compiler::beginScope(ObjFunction *function) {
  parser = current ? current->parser : parser;
  this->enclosing = current;
  current = this;
  fieldCount = 0;
  realLocalStart = 0;
  realLocalCount = 0;
  localStart = 0;
  localCount = 0;
  this->function = function;

  if (enclosing) {
    ObjString *enclosingNameObj = enclosing->function->name;
    std::string enclosingName = enclosingNameObj ? std::string("_") + enclosingNameObj->chars : "";

    prefix = enclosing->prefix + enclosingName;
  }
  else
    prefix = "qni";
  addLocal({VAL_OBJ, &function->obj});
}

void Compiler::beginScope() {
  parser = current->parser;
  this->enclosing = current;
  current = this;
  fieldCount = 0;
  realLocalStart = 0;
  realLocalCount = 0;
  localStart = enclosing->localStart + enclosing->localCount;
  localCount = 0;
  function = enclosing->function;
  prefix = enclosing->prefix;
}

void Compiler::endScope() {
  current = enclosing;
}

Local *Compiler::addLocal(ValueType type) {
  return addLocal({type, NULL});
}

Local *Compiler::addLocal(Type type) {
  if (localCount == UINT8_COUNT) {
    parser->error("Too many local variables in function.");
    return NULL;
  }

  Local *local = &locals[localCount++];

  local->type = type;
  local->name.start = "";
  local->name.length = 0;
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

    if (identifiersEqual(name, &local->name)) {
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
          parser->error("Variable '%.*s' is not a call.", name->length, name->start);
          return -2;
        }
      else
        found = localStart + i;
    }
  }

  if (found == -2) {
    char parms[512] = "";

    for (int index = 0; index < signature->arity; index++) {
      if (index)
        strcat(parms, ", ");

      strcat(parms, signature->fields[index].type.toString());
    }

    parser->error("Call '%.*s(%s)' does not match %s.", name->length, name->start, parms, buffer);
  }

  return found;
}

int Compiler::resolveUpvalue(Token *name) {
  if (enclosing != NULL) {
    int local = -1;

    Compiler *current = enclosing;

    while (true) {
      local = current->resolveLocal(name);

      if (local == -1 && current->inLocalBlock())
        current = current->enclosing;
      else
        break;
    }

    switch (local) {
    case -2:
      break;

    case -1: {
      int upvalue = current->resolveUpvalue(name);

      if (upvalue != -1)
        return addUpvalue((uint8_t) upvalue, enclosing->function->upvalues[upvalue].type, false);
      break;
    }
    default:
      current->getLocal(local).isField = true;
      return addUpvalue((uint8_t) local, current->getLocal(local).type, true);
    }
  }

  return -1;
}

int Compiler::addUpvalue(uint8_t index, Type type, bool isLocal) {
  return function->addUpvalue(index, isLocal, type, *parser);
}

void Compiler::resolveVariableExpr(VariableExpr *expr) {
  Compiler *current = this;

  while (true) {
    expr->index = current->resolveLocal(&expr->name);

    if (expr->index == -1 && current->inLocalBlock())
      current = current->enclosing;
    else
      break;
  }

  switch (expr->index) {
  case -2:
    expr->index = -1;
    addLocal(VAL_VOID);
    break;

  case -1:
    if ((expr->index = current->resolveUpvalue(&expr->name)) != -1) {
      expr->upvalueFlag = true;
      addLocal(function->upvalues[expr->index].type);
    }
    else {
      parser->error("Variable '%.*s' must be defined", expr->name.length, expr->name.start);
      addLocal(VAL_VOID);
    }
    break;

  default:
    addLocal(current->getLocal(expr->index).type);
    break;
  }
}

void Compiler::checkDeclaration(Token *name) {
  for (int i = localCount - 1; i >= 0; i--) {
    Local *local = &locals[i];

    if (identifiersEqual(name, &local->name))
;//      parser->error("Already a variable '%.*s' with this name in this scope.", name->length, name->start);
  }
}

bool Compiler::inLocalBlock() {
  return enclosing && enclosing->function == function;
}
