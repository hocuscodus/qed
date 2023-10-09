/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * This code has been generated using astgenerator.cpp.
 * Do not manually modify it.
 */
#include "expr.hpp"

Expr::Expr(ExprType type) {
  this->type = type;
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

CastExpr::CastExpr(Expr* typeExpr, Expr* expr) : Expr(EXPR_CAST) {
  this->typeExpr = typeExpr;
  this->expr = expr;
}

GroupingExpr::GroupingExpr(Token name, Expr* body, Declaration* declarations) : Expr(EXPR_GROUPING) {
  this->name = name;
  this->body = body;
  this->declarations = declarations;
}

ArrayExpr::ArrayExpr(Expr* body) : Expr(EXPR_ARRAY) {
  this->body = body;
}

CallExpr::CallExpr(bool newFlag, Expr* callee, Token paren, Expr* params, Expr* handler) : Expr(EXPR_CALL) {
  this->newFlag = newFlag;
  this->callee = callee;
  this->paren = paren;
  this->params = params;
  this->handler = handler;
}

ArrayElementExpr::ArrayElementExpr(Expr* callee, Token bracket, int count, Expr** indexes) : Expr(EXPR_ARRAYELEMENT) {
  this->callee = callee;
  this->bracket = bracket;
  this->count = count;
  this->indexes = indexes;
}

DeclarationExpr::DeclarationExpr(Expr* initExpr) : Expr(EXPR_DECLARATION) {
  this->initExpr = initExpr;
}

FunctionExpr::FunctionExpr(int arity, GroupingExpr* body, Expr* ui) : Expr(EXPR_FUNCTION) {
  this->arity = arity;
  this->body = body;
  this->ui = ui;
}

GetExpr::GetExpr(Expr* object, Token name, int index) : Expr(EXPR_GET) {
  this->object = object;
  this->name = name;
  this->index = index;
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

SetExpr::SetExpr(Expr* object, Token name, Token op, Expr* value, int index) : Expr(EXPR_SET) {
  this->object = object;
  this->name = name;
  this->op = op;
  this->value = value;
  this->index = index;
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

SwapExpr::SwapExpr() : Expr(EXPR_SWAP) {
}

NativeExpr::NativeExpr(Token nativeCode) : Expr(EXPR_NATIVE) {
  this->nativeCode = nativeCode;
}
