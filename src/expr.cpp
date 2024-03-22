/*
 * The QED Programming Language
 * Copyright (C) 2022-2024  Hocus Codus Software inc.
 *
 * This code has been generated using astgenerator.cpp.
 * Do not manually modify it.
 */
#include "expr.hpp"

Expr::Expr(ExprType type) {
  this->type = type;
  hasSuperCalls = false;
}

void Expr::destroy() {
  cleanExprs();
  delete this;
}

UIAttributeExpr::UIAttributeExpr(Token name, Expr* handler) : Expr(EXPR_UIATTRIBUTE) {
  this->name = name;
  this->handler = handler;
}

UIDirectiveExpr::UIDirectiveExpr(int childDir, int attCount, UIAttributeExpr** attributes, UIDirectiveExpr* previous, UIDirectiveExpr* lastChild, int viewIndex, bool childrenViewFlag) : Expr(EXPR_UIDIRECTIVE) {
  this->childDir = childDir;
  this->attCount = attCount;
  this->attributes = attributes;
  this->previous = previous;
  this->lastChild = lastChild;
  this->viewIndex = viewIndex;
  this->childrenViewFlag = childrenViewFlag;
}

IteratorExpr::IteratorExpr(Token name, Token op, Expr* value) : Expr(EXPR_ITERATOR) {
  this->name = name;
  this->op = op;
  this->value = value;
}

AssignExpr::AssignExpr(Expr* varExp, Token op, Expr* value) : Expr(EXPR_ASSIGN) {
  this->varExp = varExp;
  this->op = op;
  this->value = value;
}

BinaryExpr::BinaryExpr(Expr* left, Token op, Expr* right) : Expr(EXPR_BINARY) {
  this->left = left;
  this->op = op;
  this->right = right;
}

CastExpr::CastExpr(Type type, Expr* expr) : Expr(EXPR_CAST) {
  this->type = type;
  this->expr = expr;
}

GroupingExpr::GroupingExpr(Token name, Expr* body, Declaration* declarations, Expr* ui) : Expr(EXPR_GROUPING) {
  this->name = name;
  this->body = body;
  this->declarations = declarations;
  this->ui = ui;
}

ArrayExpr::ArrayExpr(Expr* body) : Expr(EXPR_ARRAY) {
  this->body = body;
}

CallExpr::CallExpr(bool newFlag, Expr* callee, Token paren, Expr* args, Expr* handler) : Expr(EXPR_CALL) {
  this->newFlag = newFlag;
  this->callee = callee;
  this->paren = paren;
  this->args = args;
  this->handler = handler;
}

ArrayElementExpr::ArrayElementExpr(Expr* callee, Token bracket, int count, Expr** indexes) : Expr(EXPR_ARRAYELEMENT) {
  this->callee = callee;
  this->bracket = bracket;
  this->count = count;
  this->indexes = indexes;
}

DeclarationExpr::DeclarationExpr(Expr* initExpr, int declarationLevel) : Expr(EXPR_DECLARATION) {
  this->initExpr = initExpr;
  this->declarationLevel = declarationLevel;
}

FunctionExpr::FunctionExpr(int arity, Expr* params, Declaration* declarations, GroupingExpr* body) : Expr(EXPR_FUNCTION) {
  this->arity = arity;
  this->params = params;
  this->declarations = declarations;
  this->body = body;
}

GetExpr::GetExpr(Expr* object, Token name) : Expr(EXPR_GET) {
  this->object = object;
  this->name = name;
}

IfExpr::IfExpr(Expr* condition, Expr* thenBranch, Expr* elseBranch) : Expr(EXPR_IF) {
  this->condition = condition;
  this->thenBranch = thenBranch;
  this->elseBranch = elseBranch;
}

LiteralExpr::LiteralExpr(ValueType type, As as) : Expr(EXPR_LITERAL) {
  this->type = type;
  this->as = as;
}

LogicalExpr::LogicalExpr(Expr* left, Token op, Expr* right) : Expr(EXPR_LOGICAL) {
  this->left = left;
  this->op = op;
  this->right = right;
}

PrimitiveExpr::PrimitiveExpr(Token name, Type primitiveType) : Expr(EXPR_PRIMITIVE) {
  this->name = name;
  this->primitiveType = primitiveType;
}

ReferenceExpr::ReferenceExpr(Token name, Expr* declaration) : Expr(EXPR_REFERENCE) {
  this->name = name;
  this->declaration = declaration;
}

ReturnExpr::ReturnExpr(Token keyword, bool isUserClass, Expr* value) : Expr(EXPR_RETURN) {
  this->keyword = keyword;
  this->isUserClass = isUserClass;
  this->value = value;
}

SetExpr::SetExpr(Expr* object, Token name, Token op, Expr* value) : Expr(EXPR_SET) {
  this->object = object;
  this->name = name;
  this->op = op;
  this->value = value;
}

TernaryExpr::TernaryExpr(Token op, Expr* left, Expr* middle, Expr* right) : Expr(EXPR_TERNARY) {
  this->op = op;
  this->left = left;
  this->middle = middle;
  this->right = right;
}

ThisExpr::ThisExpr(Token keyword) : Expr(EXPR_THIS) {
  this->keyword = keyword;
}

UnaryExpr::UnaryExpr(Token op, Expr* right) : Expr(EXPR_UNARY) {
  this->op = op;
  this->right = right;
}

WhileExpr::WhileExpr(Expr* condition, Expr* body) : Expr(EXPR_WHILE) {
  this->condition = condition;
  this->body = body;
}

SwapExpr::SwapExpr(Expr* expr) : Expr(EXPR_SWAP) {
  this->expr = expr;
}

NativeExpr::NativeExpr(Token nativeCode) : Expr(EXPR_NATIVE) {
  this->nativeCode = nativeCode;
}
