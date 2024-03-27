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
  Expr *expr;
  Scope *enclosing;
  int vCount;

  Scope(Expr *expr, Scope *enclosing);
  void add(Declaration *declaration);
  FunctionExpr *getFunction();
  FunctionExpr *getTopFunction();
  GroupingExpr *getGroup();
  Declaration *&getDeclarations();
};

extern Declaration *arrayDeclaration;
extern Expr **currentExpr;

Expr *getCurrentExpr();
void setCurrentExpr(Expr *expr);

class Parser;

typedef std::vector<Type> Signature;

Type resolveType(Expr *expr);
void pushScope(Expr *expr);
void popScope();
DeclarationExpr *newDeclarationExpr(Type type, Token name, Expr* initExpr);
FunctionExpr *newFunctionExpr(Type type, Token name, int arity, Expr* params, GroupingExpr* body);
Expr *getDeclarationExpr(Expr *body);
Signature *getSignature();
Declaration *getDeclarationRef(Token name, Declaration *dec);
Declaration *getFirstDeclarationRef(Scope *current, Token &name, int level);
Expr *getStatement(GroupingExpr *expr, int index);
DeclarationExpr *getParam(FunctionExpr *expr, int index);
Expr *resolveReference(Declaration *decRef, Token &name, Signature *signature, Parser *parser);
Expr *resolveReferenceExpr(Token &name, Parser *parser, int level);
std::string getRealName(ObjFunction *function);
bool isInRegularFunction(FunctionExpr *function);
bool isClass(FunctionExpr *function);
bool isInClass();
bool isExternalField(FunctionExpr *function, Declaration *dec);
bool isField(FunctionExpr *function, Declaration *dec);
bool isInRegularFunction(ObjFunction *function);
void checkDeclaration(Declaration &declaration, Token &name, FunctionExpr *function, Parser *parser, int level);

std::string compile(FunctionExpr *expr, Parser *parser);

bool identifiersEqual(Token *a, Token *b);
Type getDeclarationType(Expr *expr);
Declaration *getDeclaration(Expr *expr);
void pushSignature(Signature *signature);
void popSignature();

Scope *getCurrent();
FunctionExpr *getFunction();
FunctionExpr *getTopFunction();

Expr **carAddress(Expr **body, TokenType tokenType);
Expr **initAddress(Expr *&body);
bool isNext(Expr *body, TokenType tokenType);
Expr **cdrAddress(Expr *body, TokenType tokenType);
Expr *addToGroup(Expr **body, Expr *exp);
Expr **addExpr(Expr **body, Expr *exp, Token op);
Expr **removeExpr(Expr **body, TokenType tokenType);
Expr **getLastBodyExpr(Expr **body, TokenType tokenType);
bool isGroup(Expr *exp, TokenType tokenType);
Expr *car(Expr *exp, TokenType tokenType);
Expr *cdr(Expr *exp, TokenType tokenType);
int getSize(Expr *exp, TokenType tokenType);

char *genSymbol(std::string name);
GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, std::function<Expr*()> bodyFn);
GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, Expr *body);
GroupingExpr *makeWrapperLambda(DeclarationExpr *param, std::function<Expr*()> bodyFn);

Expr *analyzeStatement(Expr *expr, Parser &parser);

#endif
