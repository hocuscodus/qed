/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_compiler_h
#define qed_compiler_h

#include <vector>
#include "expr.hpp"

struct Scope {
  FunctionExpr *function;
  GroupingExpr *group;
  Declaration **current;
  Scope *enclosing;
  bool hasSuperCalls;
  int vCount;

  Scope(FunctionExpr *function, GroupingExpr *group, Scope *enclosing);
  void add(Declaration *declaration);
};

class Parser;

typedef std::vector<Type> Signature;

void pushScope(FunctionExpr *functionExpr);
void pushScope(GroupingExpr *groupingExpr);
void popScope();
DeclarationExpr *newDeclarationExpr(Type type, Token name, Expr* initExpr);
FunctionExpr *newFunctionExpr(Type type, Token name, int arity, GroupingExpr* body, Expr* ui);
Expr *getStatement(GroupingExpr *expr, int index);
DeclarationExpr *getParam(FunctionExpr *expr, int index);
Expr *resolveReferenceExpr(Token &name, Parser *parser);
std::string getRealName(ObjFunction *function);
bool isInRegularFunction(FunctionExpr *function);
bool isClass(FunctionExpr *function);
bool isExternalField(FunctionExpr *function, DeclarationExpr *expr);
bool isField(FunctionExpr *function, DeclarationExpr *expr);
bool isInRegularFunction(ObjFunction *function);
bool isExternalField(ObjFunction *function);
Expr *checkDeclaration(Declaration &declaration, Token &name, FunctionExpr *function, Parser *parser);

ObjFunction *compile(FunctionExpr *expr, Parser *parser);

bool identifiersEqual(Token *a, Token *b);
Type getDeclarationType(Expr *expr);
Declaration *getDeclaration(Expr *expr);
void pushSignature(Signature *signature);
void popSignature();

Scope *getCurrent();
FunctionExpr *getFunction(Scope *current);
FunctionExpr *getFunction();

Expr **cdrAddress(Expr *body, TokenType tokenType);
Expr **addExpr(Expr **body, Expr *exp, Token op);
Expr *removeExpr(Expr *body, TokenType tokenType);
Expr **getLastBodyExpr(Expr **body, TokenType tokenType);
bool isGroup(Expr *exp, TokenType tokenType);
Expr *car(Expr *exp, TokenType tokenType);
Expr *cdr(Expr *exp, TokenType tokenType);
int getSize(Expr *exp, TokenType tokenType);

#endif
