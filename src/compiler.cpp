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
static Scope *current = NULL;

Scope::Scope(FunctionExpr *function, GroupingExpr *group, Scope *enclosing) {
  this->function = function;
  this->group = group;
  this->enclosing = enclosing;
  hasSuperCalls = false;
  vCount = 1;
}

Scope *getCurrent() {
  return current;
}

void pushScope(FunctionExpr *functionExpr) {
  current = new Scope(functionExpr, functionExpr->body, current);
}

void pushScope(GroupingExpr *groupingExpr) {
  current = new Scope(NULL, groupingExpr, current);
}

void popScope() {
  if (current) {
    Scope *enclosing = current->enclosing;

    delete current;
    current = enclosing;
  }
  else
    current = NULL;
}

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

void Compiler::pushScope(ObjFunction *function) {
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
    dec->isInternalField = false;
    dec->function = function;
    dec->previous = previous;
    dec->parentFlag = parentFlag;
  }

  return dec;
}

static DeclarationExpr *getDeclarationExpr(Expr *body) {
  Expr *expr = car(body, TOKEN_SEPARATOR);

  return expr->type == EXPR_DECLARATION ? (DeclarationExpr *) expr : NULL;
}

static Expr *getDeclarationRef(Token name, Expr *body) {
  while (body) {
    DeclarationExpr *dec = getDeclarationExpr(body);

    if (dec && identifiersEqual(&name, &dec->name))
      break;

    body = cdr(body, TOKEN_SEPARATOR);
  }

  return body;
}

Expr *getFirstDeclarationRef(Scope *current, Token &name) {
  Expr *expr = NULL;

  while (!expr && current) {
    expr = getDeclarationRef(name, current->group->body);
    current = current->enclosing;
  }

  return expr;
}

Expr *getNextDeclarationRef(Token &name, Expr *previous) {
  return getDeclarationRef(name, cdr(previous, TOKEN_SEPARATOR));
}

DeclarationExpr *getParam(FunctionExpr *expr, int index) {
  for (Expr *body = expr->body->body; body; body = cdr(body, TOKEN_SEPARATOR))
    if (!index--)
      return getDeclarationExpr(body);

  return NULL;
}

Type &Compiler::peekDeclaration() {
  return declarations[declarationCount - 1].type;
}

Expr *resolveReference(Expr *decRef, Token &name, ObjFunction *signature, Parser *parser) {
  char buf[2048] = "";

  while (decRef) {
    DeclarationExpr *dec = getDeclarationExpr(decRef);

    // Remove these patches ASAP
    if (signature && IS_OBJ(dec->decType) && name.getString().find("Instance") == std::string::npos && strcmp(name.getString().c_str(), "post"))
      switch (AS_OBJ_TYPE(dec->decType)) {
      case OBJ_FUNCTION: {
        ObjFunction *callable = AS_FUNCTION_TYPE(dec->decType);
        bool isSignature = signature->compiler->declarationCount == callable->expr->arity;

        for (int index = 0; isSignature && index < callable->expr->arity; index++)
          isSignature = signature->compiler->declarations[index].type.equals(callable->compiler->declarations[index].type);

        if (isSignature)
          return dec;
        else
          if (parser) {
            if (buf[0])
              strcat(buf, ", ");

            strcat(buf, dec->name.getString().c_str());
            strcat(buf, "(");

            for (int index = 0; index < callable->expr->arity; index++) {
              if (index)
                strcat(buf, ", ");

              strcat(buf, callable->compiler->declarations[index].type.toString());
            }

            strcat(buf, ")'");
          }
        break;
      }
      default:
        return dec;
      }
    else
      return dec;

    decRef = getNextDeclarationRef(name, decRef);
  }

  if (parser)
    if (buf[0]) {
      char parms[512] = "";

      for (int index = 0; index < signature->compiler->declarationCount; index++) {
        if (index)
          strcat(parms, ", ");

        strcat(parms, signature->compiler->declarations[index].type.toString());
      }

      parser->error("Call '%.*s(%s)' does not match %s.", name.length, name.start, parms, buf);
    } else
      parser->error("Variable '%.*s' must be defined", name.length, name.start);

  return NULL;
}

Expr *resolveReferenceExpr(Token &name, Parser *parser) {
  return resolveReference(getFirstDeclarationRef(getCurrent(), name), name, getSignature(), parser);
}

std::string getRealName(ObjFunction *function) {
  return function->peerDeclaration ? getRealName(&function->peerDeclaration->_function) + (function->parentFlag ? "$" : "$_") : std::string(function->expr->name.start, function->expr->name.length);
}

bool isInRegularFunction(ObjFunction *function) {
  return function->expr->body->name.type == TOKEN_LEFT_BRACE;
}

bool isExternalField(ObjFunction *function) {
  return function && function->expr && function->isClass() && isInRegularFunction(function) && !function->expr->name.isInternal();
}
/*
void Compiler::checkDeclaration(Type returnType, ReferenceExpr *expr, ObjFunction *signature, Parser *parser) {
  expr->_declaration = checkDeclaration(returnType, expr->name, signature, parser);
}
*/
DeclarationExpr *checkDeclaration(Type returnType, Token &name, ObjFunction *signature, Parser *parser) {
  DeclarationExpr *peerDeclaration = NULL;
  bool parentFlag = false;

  if (resolveReference(getDeclarationRef(name, getCurrent()->group->body), name, signature, NULL)) {
    parser->error("Identical identifier '%.*s' with this name in this scope.", name.length, name.start);
    return NULL;
  }

  if (signature) {
    Expr *expr = resolveReference(getDeclarationRef(name, getCurrent()->group->body), name, NULL, NULL);

    if (!expr) {
      expr = resolveReference(getFirstDeclarationRef(getCurrent()->enclosing, name), name, NULL, NULL);
      signature->expr->_function.parentFlag = !!expr;
    }

    signature->expr->_function.peerDeclaration = (FunctionExpr *) expr;
  }

  return NULL;
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

Expr **addExpr(Expr **body, Expr *exp, Token op) {
  if (!*body)
    *body = exp;
  else {
    Expr **lastExpr = getLastBodyExpr(body, op.type);

    *lastExpr = new BinaryExpr(*lastExpr, op, exp);
  }

  return body;
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
