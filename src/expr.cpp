/*
 * The QED Programming Language
 * Copyright (C) 2022  Hocus Codus Software inc.
 */
#include "expr.hpp"

Expr::Expr(ExprType type) {
  this->type = type;
}

VariableExpr::VariableExpr(Token name, int8_t index, bool upvalueFlag) : Expr(EXPR_VARIABLE) {
  this->name = name;
  this->index = index;
  this->upvalueFlag = upvalueFlag;
}

void VariableExpr::accept(ExprVisitor *visitor) {
  return visitor->visitVariableExpr(this);
}

AttributeExpr::AttributeExpr(Token name, Expr* handler) : Expr(EXPR_ATTRIBUTE) {
  this->name = name;
  this->handler = handler;
}

void AttributeExpr::accept(ExprVisitor *visitor) {
  return visitor->visitAttributeExpr(this);
}

AttributeListExpr::AttributeListExpr(int attCount, AttributeExpr** attributes, int childrenCount, AttributeListExpr** children, ChildAttrSets* attrSets) : Expr(EXPR_ATTRIBUTELIST) {
  this->attCount = attCount;
  this->attributes = attributes;
  this->childrenCount = childrenCount;
  this->children = children;
  this->attrSets = attrSets;
}

void AttributeListExpr::accept(ExprVisitor *visitor) {
  return visitor->visitAttributeListExpr(this);
}

AssignExpr::AssignExpr(VariableExpr* varExp, Token op, Expr* value) : Expr(EXPR_ASSIGN) {
  this->varExp = varExp;
  this->op = op;
  this->value = value;
}

void AssignExpr::accept(ExprVisitor *visitor) {
  return visitor->visitAssignExpr(this);
}

BinaryExpr::BinaryExpr(Expr* left, Token op, Expr* right, OpCode opCode, bool notFlag) : Expr(EXPR_BINARY) {
  this->left = left;
  this->op = op;
  this->right = right;
  this->opCode = opCode;
  this->notFlag = notFlag;
}

void BinaryExpr::accept(ExprVisitor *visitor) {
  return visitor->visitBinaryExpr(this);
}

GroupingExpr::GroupingExpr(Token name, int count, Expr** expressions, int popLevels, Expr* ui, ChildAttrSets* attrSets) : Expr(EXPR_GROUPING) {
  this->name = name;
  this->count = count;
  this->expressions = expressions;
  this->popLevels = popLevels;
  this->ui = ui;
  this->attrSets = attrSets;
}

void GroupingExpr::accept(ExprVisitor *visitor) {
  return visitor->visitGroupingExpr(this);
}

CallExpr::CallExpr(Expr* callee, Token paren, int count, Expr** arguments, bool newFlag, Expr* handler, GroupingExpr* groupingExpr) : Expr(EXPR_CALL) {
  this->callee = callee;
  this->paren = paren;
  this->count = count;
  this->arguments = arguments;
  this->newFlag = newFlag;
  this->handler = handler;
  this->groupingExpr = groupingExpr;
}

void CallExpr::accept(ExprVisitor *visitor) {
  return visitor->visitCallExpr(this);
}

DeclarationExpr::DeclarationExpr(Type type, Token name, Expr* initExpr) : Expr(EXPR_DECLARATION) {
  this->type = type;
  this->name = name;
  this->initExpr = initExpr;
}

void DeclarationExpr::accept(ExprVisitor *visitor) {
  return visitor->visitDeclarationExpr(this);
}

FunctionExpr::FunctionExpr(Type type, Token name, int count, Expr** params, Expr* body, ObjFunction* function) : Expr(EXPR_FUNCTION) {
  this->type = type;
  this->name = name;
  this->count = count;
  this->params = params;
  this->body = body;
  this->function = function;
}

void FunctionExpr::accept(ExprVisitor *visitor) {
  return visitor->visitFunctionExpr(this);
}

GetExpr::GetExpr(Expr* object, Token name, int index) : Expr(EXPR_GET) {
  this->object = object;
  this->name = name;
  this->index = index;
}

void GetExpr::accept(ExprVisitor *visitor) {
  return visitor->visitGetExpr(this);
}

ListExpr::ListExpr(int count, Expr** expressions, ExprType listType, ObjFunction* function) : Expr(EXPR_LIST) {
  this->count = count;
  this->expressions = expressions;
  this->listType = listType;
  this->function = function;
}

void ListExpr::accept(ExprVisitor *visitor) {
  return visitor->visitListExpr(this);
}

LiteralExpr::LiteralExpr(ValueType type, As as) : Expr(EXPR_LITERAL) {
  this->type = type;
  this->as = as;
}

void LiteralExpr::accept(ExprVisitor *visitor) {
  return visitor->visitLiteralExpr(this);
}

LogicalExpr::LogicalExpr(Expr* left, Token op, Expr* right) : Expr(EXPR_LOGICAL) {
  this->left = left;
  this->op = op;
  this->right = right;
}

void LogicalExpr::accept(ExprVisitor *visitor) {
  return visitor->visitLogicalExpr(this);
}

OpcodeExpr::OpcodeExpr(OpCode op, Expr* right) : Expr(EXPR_OPCODE) {
  this->op = op;
  this->right = right;
}

void OpcodeExpr::accept(ExprVisitor *visitor) {
  return visitor->visitOpcodeExpr(this);
}

ReturnExpr::ReturnExpr(Token keyword, Expr* value) : Expr(EXPR_RETURN) {
  this->keyword = keyword;
  this->value = value;
}

void ReturnExpr::accept(ExprVisitor *visitor) {
  return visitor->visitReturnExpr(this);
}

SetExpr::SetExpr(Expr* object, Token name, Token op, Expr* value, int index) : Expr(EXPR_SET) {
  this->object = object;
  this->name = name;
  this->op = op;
  this->value = value;
  this->index = index;
}

void SetExpr::accept(ExprVisitor *visitor) {
  return visitor->visitSetExpr(this);
}

StatementExpr::StatementExpr(Expr* expr) : Expr(EXPR_STATEMENT) {
  this->expr = expr;
}

void StatementExpr::accept(ExprVisitor *visitor) {
  return visitor->visitStatementExpr(this);
}

SuperExpr::SuperExpr(Token keyword, Token method) : Expr(EXPR_SUPER) {
  this->keyword = keyword;
  this->method = method;
}

void SuperExpr::accept(ExprVisitor *visitor) {
  return visitor->visitSuperExpr(this);
}

TernaryExpr::TernaryExpr(Token op, Expr* left, Expr* middle, Expr* right) : Expr(EXPR_TERNARY) {
  this->op = op;
  this->left = left;
  this->middle = middle;
  this->right = right;
}

void TernaryExpr::accept(ExprVisitor *visitor) {
  return visitor->visitTernaryExpr(this);
}

ThisExpr::ThisExpr(Token keyword) : Expr(EXPR_THIS) {
  this->keyword = keyword;
}

void ThisExpr::accept(ExprVisitor *visitor) {
  return visitor->visitThisExpr(this);
}

TypeExpr::TypeExpr(Type type) : Expr(EXPR_TYPE) {
  this->type = type;
}

void TypeExpr::accept(ExprVisitor *visitor) {
  return visitor->visitTypeExpr(this);
}

UnaryExpr::UnaryExpr(Token op, Expr* right) : Expr(EXPR_UNARY) {
  this->op = op;
  this->right = right;
}

void UnaryExpr::accept(ExprVisitor *visitor) {
  return visitor->visitUnaryExpr(this);
}

SwapExpr::SwapExpr() : Expr(EXPR_SWAP) {
}

void SwapExpr::accept(ExprVisitor *visitor) {
  return visitor->visitSwapExpr(this);
}
