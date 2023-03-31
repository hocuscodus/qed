/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  for (int i = 0; i < declarationCount; i++) {
    Token *token = &declarations[i].name;

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
    for (int index = 0; index < declarationCount; index++) {
      const char *name = declarations[index].name.start;

      printf("<%s %s> ", declarations[index].type.toString(), name ? name : "");
    }
    printf("\n");
  }
#endif
  current = enclosing;

  return parser.hadError ? NULL : function;
}

Token token = buildToken(TOKEN_IDENTIFIER, "", 0, -1);
Declaration *Compiler::beginScope(ObjFunction *function) {
  parser = current ? current->parser : parser;
  this->enclosing = current;
  current = this;
  function->compiler = this;
  fieldCount = 0;
  declarationStart = 0;
  declarationCount = 0;
  this->function = function;

  if (enclosing) {
    ObjString *enclosingNameObj = enclosing->function->name;
    std::string enclosingName = enclosingNameObj ? std::string("_") + enclosingNameObj->chars : "";

    prefix = enclosing->prefix + enclosingName;
  }
  else
    prefix = "qni";

  return addDeclaration({VAL_OBJ, &function->obj}, token);
}

void Compiler::beginScope() {
  parser = current->parser;
  this->enclosing = current;
  current = this;
  fieldCount = 0;
  declarationStart = enclosing->declarationStart + enclosing->declarationCount;
  declarationCount = 0;
  function = enclosing->function;
  prefix = enclosing->prefix;
}

void Compiler::endScope() {
  current = enclosing;
}

void Compiler::pushType(ValueType type) {
  pushType({type, NULL});
}

void Compiler::pushType(Type type) {
  typeStack.push(type);
}

Type Compiler::popType() {
  Type type = typeStack.top();

  typeStack.pop();
  return type;
}

Declaration *Compiler::addDeclaration(Type type, Token &name) {
  if (declarationCount == UINT8_COUNT) {
    parser->error("Too many declarations in function.");
    return NULL;
  }

  Declaration *dec = &declarations[declarationCount++];

  if (dec) {
    dec->type = type;
    dec->name = name;
    dec->isField = function->isClass();
  }

  return dec;
}

Type &Compiler::peekDeclaration() {
  return declarations[declarationCount - 1].type;
}

int Compiler::resolveReference(Token *name) {
  int found = -1;
  char buffer[2048] = "";
  ObjCallable *signature = getSignature();

  for (int i = declarationCount - 1; found < 0 && i >= 0; i--) {
    Declaration *dec = &declarations[i];

    if (identifiersEqual(name, &dec->name)) {
      Type type = dec->type;

//      if (signature && type.valueType == VAL_OBJ)
      // Remove these patches ASAP
//      if (signature && type.valueType == VAL_OBJ && name->getString().find("Instance") == std::string::npos)
      if (signature && IS_OBJ(type) && name->getString().find("Instance") == std::string::npos && strcmp(name->getString().c_str(), "post"))
        switch (AS_OBJ_TYPE(type)) {
        case OBJ_FUNCTION: {
          ObjCallable *callable = AS_FUNCTION_TYPE(type);
          bool isSignature = signature->arity == callable->arity;

          for (int index = 0; isSignature && index < signature->arity; index++)
            isSignature = signature->compiler->declarations[index].type.equals(callable->compiler->declarations[index + 1].type);

          found = isSignature ? i : -2; // -2 = found name, no good signature yet

          if (found == -2) {
            char buf[512];

            sprintf(buf, "'%.*s(", name->length, name->start);

            for (int index = 0; index < callable->arity; index++) {
              if (index)
                strcat(buf, ", ");

              strcat(buf, declarations[index + 1].type.toString());
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
            isSignature = signature->compiler->declarations[index].type.equals(callable->parms[index + 1]);

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
        found = declarationStart + i;
    }
  }

  if (found == -2) {
    char parms[512] = "";

    for (int index = 0; index < signature->arity; index++) {
      if (index)
        strcat(parms, ", ");

      strcat(parms, signature->compiler->declarations[index].type.toString());
    }

    parser->error("Call '%.*s(%s)' does not match %s.", name->length, name->start, parms, buffer);
  }

  return found;
}

int Compiler::resolveUpvalue(Token *name) {
  if (enclosing != NULL) {
    int decIndex = -1;

    Compiler *current = enclosing;

    while (true) {
      decIndex = current->resolveReference(name);

      if (decIndex == -1 && current->inBlock())
        current = current->enclosing;
      else
        break;
    }

    switch (decIndex) {
    case -2:
      break;

    case -1: {
      int upvalue = current->resolveUpvalue(name);

      if (upvalue != -1)
        return addUpvalue((uint8_t) upvalue, enclosing->function->upvalues[upvalue].type, false);
      break;
    }
    default:
      current->getDeclaration(decIndex).isField = true;
      return addUpvalue((uint8_t) decIndex, current->getDeclaration(decIndex).type, true);
    }
  }

  return -1;
}

int Compiler::addUpvalue(uint8_t index, Type type, bool isDeclaration) {
  return function->addUpvalue(index, isDeclaration, type, *parser);
}

void Compiler::resolveReferenceExpr(ReferenceExpr *expr) {
  Compiler *current = this;

  while (true) {
    expr->index = current->resolveReference(&expr->name);

    if (expr->index == -1 && current->inBlock())
      current = current->enclosing;
    else
      break;
  }

  switch (expr->index) {
  case -2:
    expr->index = -1;
    pushType(VAL_VOID);
    break;

  case -1:
    if ((expr->index = current->resolveUpvalue(&expr->name)) != -1) {
      expr->upvalueFlag = true;
      pushType(function->upvalues[expr->index].type);
    }
    else {
      parser->error("Variable '%.*s' must be defined", expr->name.length, expr->name.start);
      pushType(VAL_VOID);
    }
    break;

  default:
    pushType(current->getDeclaration(expr->index).type);
    break;
  }
}

void Compiler::checkDeclaration(Token *name) {
  for (int i = declarationCount - 1; i >= 0; i--) {
    Declaration *dec = &declarations[i];

    if (identifiersEqual(name, &dec->name))
;//      parser->error("Already a variable '%.*s' with this name in this scope.", name->length, name->start);
  }
}

bool Compiler::inBlock() {
  return enclosing && enclosing->function == function;
}
