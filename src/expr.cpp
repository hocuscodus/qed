/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 */
#include "expr.hpp"

Expr::Expr(ExprType type) {
  this->type = type;
}

ReferenceExpr::ReferenceExpr(Token name, int8_t index, bool upvalueFlag) : Expr(EXPR_REFERENCE) {
  this->name = name;
  this->index = index;
  this->upvalueFlag = upvalueFlag;
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

AssignExpr::AssignExpr(ReferenceExpr* varExp, Token op, Expr* value, OpCode opCode) : Expr(EXPR_ASSIGN) {
  this->varExp = varExp;
  this->op = op;
  this->value = value;
  this->opCode = opCode;
}

BinaryExpr::BinaryExpr(Expr* left, Token op, Expr* right, OpCode opCode) : Expr(EXPR_BINARY) {
  this->left = left;
  this->op = op;
  this->right = right;
  this->opCode = opCode;
}

GroupingExpr::GroupingExpr(Token name, int count, Expr** expressions, int popLevels, Expr* ui) : Expr(EXPR_GROUPING) {
  this->name = name;
  this->count = count;
  this->expressions = expressions;
  this->popLevels = popLevels;
  this->ui = ui;
}

ArrayExpr::ArrayExpr(int count, Expr** expressions, ObjFunction* function) : Expr(EXPR_ARRAY) {
  this->count = count;
  this->expressions = expressions;
  this->function = function;
}

CallExpr::CallExpr(Expr* callee, Token paren, int count, Expr** arguments, bool newFlag, Expr* handler, ObjCallable* callable, ObjFunction* handlerFunction) : Expr(EXPR_CALL) {
  this->callee = callee;
  this->paren = paren;
  this->count = count;
  this->arguments = arguments;
  this->newFlag = newFlag;
  this->handler = handler;
  this->callable = callable;
  this->handlerFunction = handlerFunction;
}

ArrayElementExpr::ArrayElementExpr(Expr* callee, Token bracket, int count, Expr** indexes) : Expr(EXPR_ARRAYELEMENT) {
  this->callee = callee;
  this->bracket = bracket;
  this->count = count;
  this->indexes = indexes;
}

DeclarationExpr::DeclarationExpr(Type type, Token name, Expr* initExpr) : Expr(EXPR_DECLARATION) {
  this->type = type;
  this->name = name;
  this->initExpr = initExpr;
}

FunctionExpr::FunctionExpr(Type type, Token name, int count, Expr** params, Expr* body, ObjFunction* function) : Expr(EXPR_FUNCTION) {
  this->type = type;
  this->name = name;
  this->count = count;
  this->params = params;
  this->body = body;
  this->function = function;
}

GetExpr::GetExpr(Expr* object, Token name, int index) : Expr(EXPR_GET) {
  this->object = object;
  this->name = name;
  this->index = index;
}

ListExpr::ListExpr(int count, Expr** expressions, ExprType listType) : Expr(EXPR_LIST) {
  this->count = count;
  this->expressions = expressions;
  this->listType = listType;
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

OpcodeExpr::OpcodeExpr(OpCode op, Expr* right) : Expr(EXPR_OPCODE) {
  this->op = op;
  this->right = right;
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

StatementExpr::StatementExpr(Expr* expr) : Expr(EXPR_STATEMENT) {
  this->expr = expr;
}

SuperExpr::SuperExpr(Token keyword, Token method) : Expr(EXPR_SUPER) {
  this->keyword = keyword;
  this->method = method;
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

TypeExpr::TypeExpr(Type type) : Expr(EXPR_TYPE) {
  this->type = type;
}

UnaryExpr::UnaryExpr(Token op, Expr* right) : Expr(EXPR_UNARY) {
  this->op = op;
  this->right = right;
}

SwapExpr::SwapExpr() : Expr(EXPR_SWAP) {
}

NativeExpr::NativeExpr(Token nativeCode) : Expr(EXPR_NATIVE) {
  this->nativeCode = nativeCode;
}
