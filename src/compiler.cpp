/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <string.h>
#include "compiler.hpp"
#include "parser.hpp"

static std::stack<Signature *> signatures;
static Scope *currentScope = NULL;

Scope::Scope(FunctionExpr *function, GroupingExpr *group, Scope *enclosing) {
  this->function = function;
  this->group = group;
  this->enclosing = enclosing;
  hasSuperCalls = false;
  vCount = 1;
}

void Scope::add(Declaration *declaration) {
  Declaration **current = &group->declarations;

  while (*current)
    if (*current != declaration)
      current = &(*current)->next;
    else
      return;

  declaration->next = NULL;
  *current = declaration;
}

Scope *getCurrent() {
  return currentScope;
}

FunctionExpr *getFunction(Scope *scope) {
  for (Scope *current = scope; current; current = current->enclosing)
    if (current->function)
      return current->function;

  return NULL;
}

FunctionExpr *getFunction() {
  return getFunction(currentScope);
}

Type resolveType(Expr *expr) {
  Type type = {(ValueType) -1, NULL};

  if (expr)
    switch (expr->type) {
      case EXPR_GROUPING: {
        Expr *lastExpr = *getLastBodyExpr(&((GroupingExpr *) expr)->body, TOKEN_SEPARATOR);

        type = lastExpr ? resolveType(lastExpr) : UNKNOWN_TYPE;
        break;
      }
      case EXPR_FUNCTION:
        type = OBJ_TYPE(&((FunctionExpr *) expr)->_function);
        break;

      case EXPR_PRIMITIVE:
        type = ((PrimitiveExpr *) expr)->primitiveType;
        break;

      case EXPR_REFERENCE: {
        Expr *dec = resolveReferenceExpr(((ReferenceExpr *) expr)->name, NULL);

        if (dec)
          switch(dec->type) {
            case EXPR_FUNCTION:
              ((ReferenceExpr *) expr)->declaration = dec;
              type = OBJ_TYPE(&((FunctionExpr *) dec)->_function);
              break;

            case EXPR_DECLARATION:
              ((ReferenceExpr *) expr)->declaration = dec;
              type = OBJ_TYPE(((ObjFunction *) ((DeclarationExpr *) dec)->_declaration.type.objType));
              break;
          }
        break;
      }
      case EXPR_ARRAYELEMENT: {
        ArrayElementExpr *arrayElementExpr = (ArrayElementExpr *) expr;

        if (!arrayElementExpr->count && arrayElementExpr->callee)
          type = OBJ_TYPE(newArray(resolveType(arrayElementExpr->callee)));
        break;
      }
    }

  if (type.valueType != -1) {
  }

  return type;
}

void pushScope(FunctionExpr *functionExpr) {
  currentScope = new Scope(functionExpr, functionExpr->body, currentScope);
}

void pushScope(GroupingExpr *groupingExpr) {
  currentScope = new Scope(NULL, groupingExpr, currentScope);
}

void popScope() {
  if (currentScope) {
    Scope *enclosing = currentScope->enclosing;

    delete currentScope;
    currentScope = enclosing;
  }
  else
    currentScope = NULL;
}

DeclarationExpr *newDeclarationExpr(Type type, Token name, Expr* initExpr) {
  DeclarationExpr *expr = new DeclarationExpr(initExpr);

  expr->_declaration.type = type;
  expr->_declaration.name = name;
  expr->_declaration.expr = expr;
  expr->_declaration.function = getFunction();
  return expr;
}

FunctionExpr *newFunctionExpr(Type type, Token name, int arity, GroupingExpr* body, Expr* ui) {
  FunctionExpr *expr = new FunctionExpr(arity, body, ui);

  expr->_declaration.type = type;
  expr->_declaration.name = name;
  expr->_declaration.expr = expr;
  expr->_declaration.function = getFunction();
  expr->_function.expr = expr;
  return expr;
}

bool identifiersEqual(Token *a, Token *b) {
  return a->length != 0 && a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

Type getDeclarationType(Expr *expr) {
  switch(expr->type) {
    case EXPR_DECLARATION: return ((DeclarationExpr *) expr)->_declaration.type;
    case EXPR_FUNCTION: return OBJ_TYPE(&((FunctionExpr *) expr)->_function);
    default: return UNKNOWN_TYPE;
  }
}

Declaration *getDeclaration(Expr *expr) {
  switch(expr ? expr->type : -1) {
    case EXPR_DECLARATION: return &((DeclarationExpr *) expr)->_declaration;
    case EXPR_FUNCTION: return &((FunctionExpr *) expr)->_declaration;
    default: return NULL;
  }
}

void pushSignature(Signature *signature) {
  signatures.push(signature);
}

void popSignature() {
  signatures.pop();
}

static Signature *getSignature() {
  return signatures.empty() ? NULL : signatures.top();
}

extern std::stringstream &line();

static Expr *getDeclarationExpr(Expr *body) {
  Expr *expr = car(body, TOKEN_SEPARATOR);

  return expr->type == EXPR_DECLARATION || expr->type == EXPR_FUNCTION ? expr : NULL;
}

static Declaration *getDeclarationRef(Token name, Declaration *dec) {
  while (dec) {
    if (identifiersEqual(&name, &dec->name))
      break;

    dec = dec->next;
  }

  return dec;
}

Declaration *getFirstDeclarationRef(Scope *current, Token &name) {
  Declaration *dec = NULL;

  while (!dec && current) {
    dec = getDeclarationRef(name, current->group->declarations);
    current = current->enclosing;
  }

  return dec;
}

Expr *getStatement(GroupingExpr *expr, int index) {
  if (index >= 0)
    for (Expr *body = expr ? expr->body : NULL; body; body = cdr(body, TOKEN_SEPARATOR))
      if (!index--)
        return body;

  return NULL;
}

DeclarationExpr *getParam(FunctionExpr *expr, int index) {
  Expr *statement = getStatement(expr->body, index);

  return statement ? (DeclarationExpr *) getDeclarationExpr(statement) : NULL;
}

Expr *resolveReference(Declaration *decRef, Token &name, Signature *signature, Parser *parser) {
  char buf[2048] = "";

  while (decRef) {
    Expr *dec = decRef->expr;
    Type type = getDeclarationType(dec);

    // Remove these patches ASAP
    if (signature && IS_OBJ(type) && name.getString().find("Instance") == std::string::npos && strcmp(name.getString().c_str(), "post"))
      switch (AS_OBJ_TYPE(type)) {
      case OBJ_FUNCTION: {
        ObjFunction *callable = AS_FUNCTION_TYPE(type);
        bool isSignature = signature->size() == callable->expr->arity;

        for (int index = 0; isSignature && index < callable->expr->arity; index++)
          isSignature = signature->at(index).equals(getParam(callable->expr, index)->_declaration.type);

        if (isSignature)
          return dec;
        else
          if (parser) {
            if (buf[0])
              strcat(buf, ", ");

            strcat(buf, getDeclaration(dec)->name.getString().c_str());
            strcat(buf, "(");

            for (int index = 0; index < callable->expr->arity; index++) {
              if (index)
                strcat(buf, ", ");

              strcat(buf, getParam(callable->expr, index)->_declaration.type.toString());
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

    decRef = getDeclarationRef(name, decRef->next);
  }

  if (parser)
    if (buf[0]) {
      char parms[512] = "";

      for (int index = 0; index < signature->size(); index++) {
        if (index)
          strcat(parms, ", ");

        strcat(parms, signature->at(index).toString());
      }

      parser->error("Call '%.*s(%s)' does not match %s.", name.length, name.start, parms, buf);
    } else
      parser->error("Variable '%.*s' must be defined", name.length, name.start);

  return NULL;
}

Expr *resolveReferenceExpr(Token &name, Parser *parser) {
  Declaration *decRef = getFirstDeclarationRef(getCurrent(), name);

  return resolveReference(decRef, name, getSignature(), parser);
}

bool isInRegularFunction(FunctionExpr *function) {
  function->body->name.type == TOKEN_LEFT_BRACE;
}

bool isClass(FunctionExpr *function) {
  return function->_declaration.name.isClass();
}

bool isExternalField(FunctionExpr *function, DeclarationExpr *expr) {
  return function && isClass(function) && isInRegularFunction(function) && !expr->_declaration.name.isInternal();
}

bool isField(FunctionExpr *function, DeclarationExpr *expr) {
  return isExternalField(function, expr) || expr->_isInternalField;
}

bool isInRegularFunction(ObjFunction *function) {
  return function->expr->body->name.type == TOKEN_LEFT_BRACE;
}

bool isExternalField(ObjFunction *function) {
  return function && function->expr && function->isClass() && isInRegularFunction(function) && !function->expr->_declaration.name.isInternal();
}

Expr *checkDeclaration(Declaration &declaration, Token &name, FunctionExpr *function, Parser *parser) {
  Signature signature;

  if (function)
    for (int index = 0; index < function->arity; index++)
      signature.push_back(getParam(function, index)->_declaration.type);

  Expr *expr = resolveReference(getDeclarationRef(name, getCurrent()->group->declarations), name, function ? &signature : NULL, NULL);

  if (expr) {
    parser->error("Identical identifier '%.*s' with this name in this scope.", name.length, name.start);
    return NULL;
  }

  if (function)
    expr = resolveReference(getDeclarationRef(name, getCurrent()->group->declarations), name, NULL, NULL);

  if (!expr) {
    expr = resolveReference(getFirstDeclarationRef(getCurrent()->enclosing, name), name, NULL, NULL);
    declaration.parentFlag = !!expr;
  }

  declaration.peer = getDeclaration(expr);
  getCurrent()->add(&declaration);
  return NULL;
}

Expr **cdrAddress(Expr *body, TokenType tokenType) {
  return body && isGroup(body, tokenType) ? &((BinaryExpr *) body)->right : NULL;
}

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
