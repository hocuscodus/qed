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
#include "parser.hpp"
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
  parser.expr->astPrint();
  printf("\n");
#endif
//  Reifier reifier(parser);
  Expr *cpsExpr = parser.expr->toCps([](Expr *expr) {
    return expr;
  });

  parser.expr->resolve(parser);

  if (parser.hadError)
    return NULL;

//  if (!reifier.reify())
//    return NULL;
#ifdef DEBUG_PRINT_CODE
  printf("Adapted parse: ");
  parser.expr->astPrint();
  printf("\n          ");
  for (int i = 0; i < declarationCount; i++) {
    Token *token = &declarations[i].name;

    if (token != NULL)
      printf("[ %.*s ]", token->length, token->start);
    else
      printf("[ N/A ]");
  }
  printf("\n");
#endif
  parser.expr->toCode(parser, function);

  if (parser.hadError)
    function->chunk.reset();

  current = enclosing;

  return parser.hadError ? NULL : function;
}

Token token = buildToken(TOKEN_IDENTIFIER, "", 0, -1);
Declaration *Compiler::beginScope(ObjFunction *function) {
  parser = current ? current->parser : parser;
  this->enclosing = current;
  current = this;
  function->compiler = this;
  fieldCount = 1;
  declarationStart = 0;
  declarationCount = 0;
  vCount = 1;
  this->function = function;

  if (enclosing) {
    ObjString *enclosingNameObj = enclosing->function->name;
    std::string enclosingName = enclosingNameObj ? std::string("_") + enclosingNameObj->chars : "";

    prefix = enclosing->prefix + enclosingName;
    enclosing->function->add(function);
  }
  else
    prefix = "qni";

  return addDeclaration({VAL_OBJ, &function->obj}, token, NULL, false);
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
/*
void Compiler::pushType(ValueType type) {
  pushType({type, NULL});
}

void Compiler::pushType(Type type) {
  typeStack.push(type);
}

Type Compiler::popType() {
  if (typeStack.empty())
    printf("toto");

  Type type = typeStack.top();

  typeStack.pop();
  return type;
}
*/
Declaration *Compiler::addDeclaration(Type type, Token &name, Declaration *previous, bool parentFlag) {
  if (declarationCount == UINT8_COUNT) {
    parser->error("Too many declarations in function.");
    return NULL;
  }

  Declaration *dec = &declarations[declarationCount++];

  if (dec) {
    dec->type = type;
    dec->name = name;
    dec->isField = function->isClass();
    dec->function = function;
    dec->previous = previous;
    dec->parentFlag = parentFlag;
  }

  return dec;
}

Type &Compiler::peekDeclaration() {
  return declarations[declarationCount - 1].type;
}

int Compiler::resolveReference(Token *name) {
  int found = -1;
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
            isSignature = signature->compiler->declarations[index + 1].type.equals(callable->compiler->declarations[index + 1].type);

          found = isSignature ? i : -2 - i - declarationStart; // -2 = found name, no good signature yet
          break;
        }
        case OBJ_FUNCTION_PTR: {
          ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;
          bool isSignature = signature->arity == callable->arity;

          for (int index = 0; isSignature && index < signature->arity; index++)
            isSignature = signature->compiler->declarations[index].type.equals(callable->parms[index + 1]);

          found = isSignature ? i : -2 - i - declarationStart; // -2 = found name, no good signature yet
          break;
        }
        default:
          parser->error("Variable '%.*s' is not a call.", name->length, name->start);
          return -2 - i - declarationStart;
        }
      else
        found = declarationStart + i;
    }
  }

  return found;
}

int Compiler::resolveReference2(Token *name) {
  int found = resolveReference(name);

  if (found <= -2) {
    ObjCallable *signature = getSignature();
    char parms[512] = "";
    char buf[2048];
    Declaration *dec = &declarations[-2 - found - declarationStart];
    Type type = dec->type;
    ObjCallable *callable = AS_FUNCTION_TYPE(type);

    sprintf(buf, "'%.*s(", name->length, name->start);

    for (int index = 0; index < callable->arity; index++) {
      if (index)
        strcat(buf, ", ");

      strcat(buf, declarations[index + 1].type.toString());
    }

    strcat(buf, ")'");

    for (int index = 0; index < signature->arity; index++) {
      if (index)
        strcat(parms, ", ");

      strcat(parms, signature->compiler->declarations[index].type.toString());
    }

    parser->error("Call '%.*s(%s)' does not match %s.", name->length, name->start, parms, buf);
  }

  return found;
}

int Compiler::resolveUpvalue(Token *name) {
  if (enclosing != NULL) {
    int decIndex = -1;

    Compiler *current = enclosing;

    while (true) {
      decIndex = current->resolveReference2(name);

      if (decIndex == -1 && current->inBlock())
        current = current->enclosing;
      else
        break;
    }

    if (decIndex == -1) {
      int upvalue = current->resolveUpvalue(name);

      if (upvalue != -1)
        return addUpvalue((uint8_t) upvalue, enclosing->function->upvalues[upvalue].declaration, false);
    }
    else
      if (decIndex >= 0) {
        current->getDeclaration(decIndex).isField = true;
        return addUpvalue((uint8_t) decIndex, &current->getDeclaration(decIndex), true);
      }
  }

  return -1;
}

int Compiler::addUpvalue(uint8_t index, Declaration *declaration, bool isDeclaration) {
  return function->addUpvalue(index, isDeclaration, declaration, *parser);
}

Type Compiler::resolveReferenceExpr(ReferenceExpr *expr) {
  Compiler *current = this;

  while (true) {
    expr->index = current->resolveReference2(&expr->name);

    if (expr->index == -1 && current->inBlock())
      current = current->enclosing;
    else {
      if (expr->index >= 0)
;//        expr->revIndex = expr->index - getDeclarationCount();
      break;
    }
  }

  if (expr->index == -1)
    if ((expr->index = current->resolveUpvalue(&expr->name)) != -1) {
      expr->upvalueFlag = true;
      expr->_declaration = function->upvalues[expr->index].declaration;
    }
    else {
      parser->error("Variable '%.*s' must be defined", expr->name.length, expr->name.start);
      expr->_declaration = NULL;
    }
  else
    if (expr->index >= 0)
      expr->_declaration = &current->getDeclaration(expr->index);
    else {
      expr->index = -1;
      expr->_declaration = NULL;
    }

  return expr->_declaration ? expr->_declaration->type : (Type) {VAL_VOID};
}

void Compiler::checkDeclaration(Type returnType, ReferenceExpr *expr, ObjCallable *signature) {
  expr->_declaration = checkDeclaration(returnType, expr->name, signature);
}

Declaration *Compiler::checkDeclaration(Type returnType, Token &name, ObjCallable *signature) {
  Declaration *peerDeclaration = NULL;
  bool parentFlag = false;

  pushSignature(signature);

  if (resolveReference(&name) >= 0)
    parser->error("Identical identifier '%.*s' with this name in this scope.", name.length, name.start);
  else {
    pushSignature(NULL);

    for (Compiler *compiler = this; !peerDeclaration && compiler; compiler = compiler->enclosing) {
      int index = compiler->resolveReference(&name);

      peerDeclaration = index >= 0 ? &compiler->getDeclaration(index) : NULL;
      parentFlag = peerDeclaration && compiler != this;
    }

    popSignature();
  }

  popSignature();
  return addDeclaration(returnType, name, peerDeclaration, parentFlag);
}

bool Compiler::inBlock() {
  return enclosing && enclosing->function == function;
}
