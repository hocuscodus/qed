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

static std::stack<ObjFunction *> signatures;

void pushSignature(ObjFunction *signature) {
  signatures.push(signature);
}

void popSignature() {
  signatures.pop();
}

static ObjFunction *getSignature() {
  return signatures.empty() ? NULL : signatures.top();
}

Compiler *Compiler::current = NULL;
extern std::stringstream &line();

Compiler::Compiler() {
  enclosing = NULL;
  groupingExpr = NULL;
  function = NULL;
  hasSuperCalls = false;
  vCount = 0;
  declarationCount = 0;
}

ObjFunction *Compiler::compile(FunctionExpr *expr, Parser *parser) {
#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "Original parse: ");
  expr->astPrint();
  fprintf(stderr, "\n");
#endif

  Expr *cpsExpr = expr->toCps([](Expr *expr) {
    return expr;
  });
#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "CPS parse: ");
  cpsExpr->astPrint();
  fprintf(stderr, "\n");
#endif
  cpsExpr->resolve(*parser);

  if (parser->hadError)
    return NULL;

#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "Adapted parse: ");
  cpsExpr->astPrint();
  fprintf(stderr, "\n          ");
  for (int i = 0; i < declarationCount; i++) {
    Token *token = &declarations[i].name;

    if (token != NULL)
      fprintf(stderr, "[ %.*s ]", token->length, token->start);
    else
      fprintf(stderr, "[ N/A ]");
  }
  fprintf(stderr, "\n");
#endif
  line() << "\"use strict\";\n";
  line() << "const canvas = document.getElementById(\"canvas\");\n";
  line() << "let postCount = 1;\n";
  line() << "let attributeStacks = [];\n";
  line() << "const ctx = canvas.getContext(\"2d\");\n";
  line() << "function toColor(color) {return \"#\" + color.toString(16).padStart(6, '0');}\n";
  line() << "let getAttribute = function(index) {\n";
  line() << "  return attributeStacks[index][attributeStacks[index].length - 1];\n";
  line() << "}\n";

  cpsExpr->toCode(*parser, function);

  line() << "pushAttribute(4, 20);\n";
  line() << "pushAttribute(10, 0);\n";
  line() << "pushAttribute(11, 1.0);\n";
  line() << "let layout_ = null;\n";
  line() << "function _refresh() {\n";
  line() << "//  if (ui_ != undefined && --postCount == 0) {\n";
  line() << "    ui_ = new UI_();\n";
  line() << "    layout_ = new ui_.Layout_();\n";
  line() << "    ctx.globalAlpha = 1.0;\n";
  line() << "    ctx.clearRect(0, 0, canvas.width, canvas.height);\n";
  line() << "    layout_.paint(0, 0, layout_.size >> 16, layout_.size & 65535);\n";
  line() << "//  }\n";
  line() << "}\n";
  line() << "_refresh();\n";
  line() << "canvas.addEventListener(\"mousedown\", function(ev) {\n";
  line() << "  postCount++;\n";
  line() << "  var rect = canvas.getBoundingClientRect();\n";
  line() << "  layout_.onEvent(0, ev.clientX - rect.left, ev.clientY - rect.top, layout_.size >> 16, layout_.size & 65535);\n";
  line() << "  _refresh();\n";
  line() << "  });\n";
  line() << "canvas.addEventListener(\"mouseup\", function(ev) {\n";
  line() << "  postCount++;\n";
  line() << "  var rect = canvas.getBoundingClientRect();\n";
  line() << "  layout_.onEvent(1, ev.clientX - rect.left, ev.clientY - rect.top, layout_.size >> 16, layout_.size & 65535);\n";
  line() << "  _refresh();\n";
  line() << "});\n";
  line() << "canvas.onselectstart = function () { return false; }\n";

  if (parser->hadError)
    function->chunk.reset();

  return parser->hadError ? NULL : function;
}

void Compiler::pushScope() {
  this->enclosing = current;
  current = this;
}

void Compiler::pushScope(ObjFunction *function, Parser *parser) {
  this->groupingExpr = function->expr->body;
  this->enclosing = current;
  pushScope();
  this->function = function;
  function->compiler = this;
  vCount = 1;

  if (enclosing)
    enclosing->function->add(function);
}

void Compiler::pushScope(GroupingExpr *groupingExpr) {
  this->groupingExpr = groupingExpr;
  this->enclosing = current;
  pushScope();
  function = enclosing->function;
}

void Compiler::popScope() {
  current = enclosing;
}

Declaration *Compiler::addDeclaration(Type type, Token &name, Declaration *previous, bool parentFlag, Parser *parser) {
  if (declarationCount == UINT8_COUNT) {
    parser->error("Too many declarations in function.");
    return NULL;
  }

  Declaration *dec = &declarations[declarationCount++];

  if (dec) {
    dec->type = type;
    dec->name = name;
    dec->isExternalField = function && function->expr && function->isClass() && isInRegularFunction() && !name.isInternal();
    dec->isInternalField = false;
    dec->function = function;
    dec->previous = previous;
    dec->parentFlag = parentFlag;
  }

  return dec;
}

Type &Compiler::peekDeclaration() {
  return declarations[declarationCount - 1].type;
}

int Compiler::resolveReference(Token *name, Parser *parser) {
  int found = -1;
  ObjFunction *signature = getSignature();

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
          ObjFunction *callable = AS_FUNCTION_TYPE(type);
          bool isSignature = signature->compiler->declarationCount == callable->expr->arity;

          for (int index = 0; isSignature && index < callable->expr->arity; index++)
            isSignature = signature->compiler->declarations[index].type.equals(callable->compiler->declarations[index].type);

          found = isSignature ? i : -2 - i; // -2 = found name, no good signature yet
          break;
        }
        default:
          parser->error("Variable '%.*s' is not a call.", name->length, name->start);
          return -2 - i;
        }
      else
        found = i;
    }
  }

  return found;
}

int Compiler::resolveReference2(Token *name, Parser *parser) {
  int found = resolveReference(name, parser);

  if (found <= -2) {
    ObjFunction *signature = getSignature();
    char parms[512] = "";
    char buf[2048];
    Declaration *dec = &declarations[-2 - found];
    Type type = dec->type;
    ObjFunction *callable = AS_FUNCTION_TYPE(type);

    sprintf(buf, "'%.*s(", name->length, name->start);

    for (int index = 0; index < callable->expr->arity; index++) {
      if (index)
        strcat(buf, ", ");

      strcat(buf, callable->compiler->declarations[index].type.toString());
    }

    strcat(buf, ")'");

    for (int index = 0; index < signature->compiler->declarationCount; index++) {
      if (index)
        strcat(parms, ", ");

      strcat(parms, signature->compiler->declarations[index].type.toString());
    }

    parser->error("Call '%.*s(%s)' does not match %s.", name->length, name->start, parms, buf);
  }

  return found;
}

int Compiler::resolveUpvalue(Token *name, Parser *parser) {
  if (enclosing != NULL) {
    int decIndex = -1;
    Compiler *current = enclosing;

    while (true) {
      decIndex = current->resolveReference2(name, parser);

      if (decIndex == -1 && current->inBlock())
        current = current->enclosing;
      else
        break;
    }

    if (decIndex == -1) {
      int upvalue = current->resolveUpvalue(name, parser);

      if (upvalue != -1)
        return addUpvalue((uint8_t) upvalue, enclosing->function->upvalues[upvalue].declaration, false, parser);
    }
    else
      if (decIndex >= 0) {
        current->getDeclaration(decIndex).isInternalField = true;
        return addUpvalue((uint8_t) decIndex, &current->getDeclaration(decIndex), true, parser);
      }
  }

  return -1;
}

int Compiler::addUpvalue(uint8_t index, Declaration *declaration, bool isDeclaration, Parser *parser) {
  return function->addUpvalue(index, isDeclaration, declaration, *parser);
}

Type Compiler::resolveReferenceExpr(ReferenceExpr *expr, Parser *parser) {
  Compiler *current = this;
  int index = -1;

  while (true) {
    index = current->resolveReference2(&expr->name, parser);

    if (index == -1 && current->inBlock())
      current = current->enclosing;
    else {
//      if (index >= 0)
;//        expr->revIndex = expr->index - getDeclarationCount();
      break;
    }
  }

  if (index == -1)
    if ((index = current->resolveUpvalue(&expr->name, parser)) != -1)
      expr->_declaration = function->upvalues[index].declaration;
    else {
      parser->error("Variable '%.*s' must be defined", expr->name.length, expr->name.start);
      expr->_declaration = NULL;
    }
  else
    if (index >= 0)
      expr->_declaration = &current->getDeclaration(index);
    else {
      index = -1;
      expr->_declaration = NULL;
    }

  return expr->_declaration ? expr->_declaration->type : UNKNOWN_TYPE;
}

void Compiler::checkDeclaration(Type returnType, ReferenceExpr *expr, ObjFunction *signature, Parser *parser) {
  expr->_declaration = checkDeclaration(returnType, expr->name, signature, parser);
}

Declaration *Compiler::checkDeclaration(Type returnType, Token &name, ObjFunction *signature, Parser *parser) {
  Declaration *peerDeclaration = NULL;
  bool parentFlag = false;

  pushSignature(signature);

  if (resolveReference(&name, parser) >= 0)
    parser->error("Identical identifier '%.*s' with this name in this scope.", name.length, name.start);
  else {
    pushSignature(NULL);

    for (Compiler *compiler = this; !peerDeclaration && compiler; compiler = compiler->enclosing) {
      int index = compiler->resolveReference(&name, parser);

      peerDeclaration = index >= 0 ? &compiler->getDeclaration(index) : NULL;
      parentFlag = peerDeclaration && compiler != this;
    }

    popSignature();
  }

  popSignature();
  return addDeclaration(returnType, name, peerDeclaration, parentFlag, parser);
}

bool Compiler::inBlock() {
  return enclosing && enclosing->function == function;
}

bool Compiler::isInRegularFunction() {
  return function->expr->body->name.type == TOKEN_LEFT_BRACE;
}

Expr **cdrAddress(Expr *body, TokenType tokenType) {
  return body && isGroup(body, tokenType) ? &((BinaryExpr *) body)->right : NULL;
}
/*
Expr *cutExpr(Expr **body, Expr *exp, TokenType tokenType) {
  bool groupFlag = isGroup(*body, tokenType);
  Expr **bodyExp = groupFlag && ((BinaryExpr *)*body)->left != exp ? &((BinaryExpr *)*body)->right : NULL;

  if (bodyExp)
    return cutExpr(bodyExp, exp, tokenType);
  else {
    Expr *rest = NULL;

    if (groupFlag) {
      Expr **right = cdrAddress(*body, tokenType);
      Expr *params = *right;

      for (*right = NULL; params; params = removeExpr(params, tokenType)) {
        Expr *unit = car(params, tokenType);

        addExpr(unit->type != EXPR_FUNCTION ? &rest : right, unit, buildToken(tokenType, tokenType == TOKEN_SEPARATOR ? ";" : ","));
      }
    }

    return rest;
  }
}*/

Expr **getLastBodyExpr(Expr **body, TokenType tokenType) {
  Expr **exp = cdrAddress(*body, tokenType);

  return exp ? getLastBodyExpr(exp, tokenType) : body;
}

bool isGroup(Expr *exp, TokenType tokenType) {
  return exp->type == EXPR_BINARY && ((BinaryExpr *) exp)->op.type == tokenType;
}

void addExpr(Expr **body, Expr *exp, Token op) {
  if (!*body)
    *body = exp;
  else {
    Expr **lastExpr = getLastBodyExpr(body, op.type);

    *lastExpr = new BinaryExpr(*lastExpr, op, exp);
  }
}

Expr *removeExpr(Expr *body, TokenType tokenType) {
  BinaryExpr *group = isGroup(body, tokenType) ? (BinaryExpr *) body : NULL;
  Expr *right = group ? group->right : NULL;

  if (group)
    delete group;

  return right;
}

Expr *car(Expr *exp, TokenType tokenType) {
  return isGroup(exp, tokenType) ? ((BinaryExpr *) exp)->left : exp;
}

Expr *cdr(Expr *exp, TokenType tokenType) {
  Expr **ptr = cdrAddress(exp, tokenType);

  return ptr ? *ptr : NULL;
}

int getSize(Expr *exp, TokenType tokenType) {
  int size = 0;

  for (Expr *element = exp; element; element = cdr(element, tokenType))
    size++;

  return size;
}
