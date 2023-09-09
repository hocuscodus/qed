/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_compiler_h
#define qed_compiler_h

#include <iostream>
#include <functional>
#include "expr.hpp"

struct Scope {
  FunctionExpr *function;
  GroupingExpr *group;
  Scope *enclosing;
  bool hasSuperCalls;
  int vCount;

  Scope(FunctionExpr *function, GroupingExpr *group, Scope *enclosing);
};

class Parser;

void pushScope(FunctionExpr *functionExpr);
void pushScope(GroupingExpr *groupingExpr);
void popScope();
DeclarationExpr *getParam(FunctionExpr *expr, int index);
Expr *resolveReferenceExpr(Token &name, Parser *parser);
std::string getRealName(ObjFunction *function);
bool isInRegularFunction(ObjFunction *function);
bool isExternalField(ObjFunction *function);
DeclarationExpr *checkDeclaration(Type returnType, Token &name, ObjFunction *signature, Parser *parser);

ObjFunction *compile(FunctionExpr *expr, Parser *parser);

struct Compiler {
  Compiler *enclosing;
  GroupingExpr *groupingExpr;
  ObjFunction *function = NULL;
  bool hasSuperCalls;
  int vCount;
  int declarationCount;
  Declaration declarations[UINT8_COUNT];
  Expr *funcs = NULL;

  Compiler();

  ObjFunction *compile(FunctionExpr *expr, Parser *parser);
  void pushScope();
  void pushScope(ObjFunction *function);
  void pushScope(GroupingExpr *groupingExpr);
  void popScope();

  Declaration *addDeclaration(Type type, Token &name, Declaration *previous, bool parentFlag, Parser *parser);
  Type &peekDeclaration();
//  int resolveReference(Token *name, Parser *parser);
//  int resolveReference2(Token *name, Parser *parser);
//  int resolveUpvalue(Token *name, Parser *parser);
//  int addUpvalue(uint8_t index, Declaration *declaration, bool isDeclaration, Parser *parser);
//  Type resolveReferenceExpr(ReferenceExpr *expr, Parser *parser);
//  void checkDeclaration(Type returnType, ReferenceExpr *expr, ObjFunction *signature, Parser *parser);
//  Declaration *checkDeclaration(Type returnType, Token &name, ObjFunction *signature, Parser *parser);
  bool inBlock();
  bool isInRegularFunction();

  static inline Compiler *getCurrent() {
    return current;
  }

  inline int getDeclarationCount() {
    return declarationCount;
  }

  inline Declaration &getDeclaration(int index) {
    return declarations[index];
  }
private:
  static Compiler *current;
};

bool identifiersEqual(Token *a, Token *b);
void pushSignature(ObjFunction *signature);
void popSignature();

Scope *getCurrent();

static inline FunctionExpr *getFunction() {
  return NULL;
}

struct Expr;

Expr **cdrAddress(Expr *body, TokenType tokenType);
Expr **addExpr(Expr **body, Expr *exp, Token op);
Expr *removeExpr(Expr *body, TokenType tokenType);
Expr **getLastBodyExpr(Expr **body, TokenType tokenType);
bool isGroup(Expr *exp, TokenType tokenType);
Expr *car(Expr *exp, TokenType tokenType);
Expr *cdr(Expr *exp, TokenType tokenType);
int getSize(Expr *exp, TokenType tokenType);

#endif


