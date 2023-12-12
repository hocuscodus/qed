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
  Scope *enclosing;
  int vCount;

  Scope(FunctionExpr *function, GroupingExpr *group, Scope *enclosing);
  void add(Declaration *declaration);
};

class Parser;

typedef std::vector<Type> Signature;

Type resolveType(Expr *expr);
void pushScope(FunctionExpr *functionExpr);
void pushScope(GroupingExpr *groupingExpr);
void popScope();
DeclarationExpr *newDeclarationExpr(Type type, Token name, Expr* initExpr);
FunctionExpr *newFunctionExpr(Type type, Token name, int arity, GroupingExpr* body, Expr* ui);
Expr *getDeclarationExpr(Expr *body);
Signature *getSignature();
Declaration *getDeclarationRef(Token name, Declaration *dec);
Declaration *getFirstDeclarationRef(Scope *current, Token &name);
Expr *getStatement(GroupingExpr *expr, int index);
DeclarationExpr *getParam(FunctionExpr *expr, int index);
Expr *resolveReference(Declaration *decRef, Token &name, Signature *signature, Parser *parser);
Expr *resolveReferenceExpr(Token &name, Parser *parser);
std::string getRealName(ObjFunction *function);
bool isInRegularFunction(FunctionExpr *function);
bool isClass(FunctionExpr *function);
bool isExternalField(FunctionExpr *function, DeclarationExpr *expr);
bool isField(FunctionExpr *function, DeclarationExpr *expr);
bool isInRegularFunction(ObjFunction *function);
bool isExternalField(ObjFunction *function);
void checkDeclaration(Declaration &declaration, Token &name, FunctionExpr *function, Parser *parser);

std::string compile(FunctionExpr *expr, Parser *parser);

bool identifiersEqual(Token *a, Token *b);
Type getDeclarationType(Expr *expr);
Declaration *getDeclaration(Expr *expr);
void pushSignature(Signature *signature);
void popSignature();

Scope *getCurrent();
FunctionExpr *getFunction(Scope *current);
FunctionExpr *getFunction();

Expr **cdrAddress(Expr *body, TokenType tokenType);
Expr *addToGroup(Expr **body, Expr *exp);
Expr **addExpr(Expr **body, Expr *exp, Token op);
Expr *removeExpr(Expr *body, TokenType tokenType);
Expr **getLastBodyExpr(Expr **body, TokenType tokenType);
bool isGroup(Expr *exp, TokenType tokenType);
Expr *car(Expr *exp, TokenType tokenType);
Expr *cdr(Expr *exp, TokenType tokenType);
int getSize(Expr *exp, TokenType tokenType);

char *genSymbol(std::string name);
GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, std::function<Expr*()> bodyFn);
GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, Expr *body);
GroupingExpr *makeWrapperLambda(DeclarationExpr *param, std::function<Expr*()> bodyFn);

#endif
