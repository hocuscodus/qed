/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 */
#include "expr.hpp"

Expr::Expr(ExprType type) {
  this->type = type;
}

void Expr::destroy() {
  cleanExprs();
  delete this;
}

TypeExpr::TypeExpr(Token name, bool functionFlag, bool noneFlag, int numDim, int index, Declaration* declaration) : Expr(EXPR_TYPE) {
  this->name = name;
  this->functionFlag = functionFlag;
  this->noneFlag = noneFlag;
  this->numDim = numDim;
  this->index = index;
  this->declaration = declaration;
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

GroupingExpr::GroupingExpr(Token name, int count, Expr** expressions, Expr* ui) : Expr(EXPR_GROUPING) {
  this->name = name;
  this->count = count;
  this->expressions = expressions;
  this->ui = ui;
}

ArrayExpr::ArrayExpr(int count, Expr** expressions, ObjFunction* function) : Expr(EXPR_ARRAY) {
  this->count = count;
  this->expressions = expressions;
  this->function = function;
}

CallExpr::CallExpr(bool newFlag, Expr* callee, Token paren, int count, Expr** arguments, Expr* handler) : Expr(EXPR_CALL) {
  this->newFlag = newFlag;
  this->callee = callee;
  this->paren = paren;
  this->count = count;
  this->arguments = arguments;
  this->handler = handler;
}

ArrayElementExpr::ArrayElementExpr(Expr* callee, Token bracket, int count, Expr** indexes) : Expr(EXPR_ARRAYELEMENT) {
  this->callee = callee;
  this->bracket = bracket;
  this->count = count;
  this->indexes = indexes;
}

DeclarationExpr::DeclarationExpr(Expr* typeExpr, Token name, Expr* initExpr) : Expr(EXPR_DECLARATION) {
  this->typeExpr = typeExpr;
  this->name = name;
  this->initExpr = initExpr;
}

FunctionExpr::FunctionExpr(Expr* typeExpr, Token name, int arity, GroupingExpr* body) : Expr(EXPR_FUNCTION) {
  this->typeExpr = typeExpr;
  this->name = name;
  this->arity = arity;
  this->body = body;
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

ListExpr::ListExpr(int count, Expr** expressions) : Expr(EXPR_LIST) {
  this->count = count;
  this->expressions = expressions;
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

ReferenceExpr::ReferenceExpr(Token name, Type returnType) : Expr(EXPR_REFERENCE) {
  this->name = name;
  this->returnType = returnType;
}

ReturnExpr::ReturnExpr(Token keyword, Expr* value) : Expr(EXPR_RETURN) {
  this->keyword = keyword;
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
