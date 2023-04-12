/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <array>
#include <set>
#include "codegen.hpp"
#include "attrset.hpp"
#include "debug.hpp"

static int nTabs = 0;
std::stringstream s;

std::stringstream &CodeGenerator::str() {
  return s;
}

std::stringstream &CodeGenerator::line() {
  for (int index = 0; index < nTabs; index++)
    str() << "  ";

  return str();
}

const char *CodeGenerator::startBlock() {
  nTabs++;
  return "";
}

const char *CodeGenerator::endBlock() {
  nTabs--;
  return "";
}

CodeGenerator::CodeGenerator(Parser &parser, ObjFunction *function) : ExprVisitor(), parser(parser) {
  this->function = function;
}

void CodeGenerator::visitAssignExpr(AssignExpr *expr) {
  if (expr->varExp)
    accept<int>(expr->varExp, 0);

  if (expr->value) {
    str() << " " << expr->op.getString() << " ";
    accept<int>(expr->value, 0);
  }
  else
    str() << expr->op.getString();
}

void CodeGenerator::visitUIAttributeExpr(UIAttributeExpr *expr) {
  parser.error("Cannot generate UI code from UI expression.");
}

void CodeGenerator::visitUIDirectiveExpr(UIDirectiveExpr *expr) {
  parser.error("Cannot generate UI code from UI expression.");
}

void CodeGenerator::visitBinaryExpr(BinaryExpr *expr) {
  if (expr->op.type == TOKEN_WHILE) {
    line() << "while(" << expr->left << ") {\n" << startBlock();
    accept<int>(expr->right, 0);
    endBlock();
    line() << "}";
  } else {
    str() << "(";
    accept<int>(expr->left, 0);
    str() << " " << expr->op.getString() << " ";
    accept<int>(expr->right, 0);
    str() << ")";
  }
}

void CodeGenerator::visitCallExpr(CallExpr *expr) {
  if (expr->newFlag)
    str() << "new ";

  accept<int>(expr->callee, 0);
  str() << "(";

  for (int index = 0; index < expr->count; index++) {
    if (index)
      str() << ", ";

    accept<int>(expr->arguments[index]);

    if (expr->callable->compiler->declarations[index].isField) {
    }
  }

  if (expr->handler) {
    {
      CodeGenerator generator(parser, expr->handlerFunction);

      if (expr->handler)
        generator.accept<int>(expr->handler);

      if (parser.hadError)
        return;
    }

    if (expr->count)
      str() << ", ";

    str() << "ReturnHandler_";
  }

  str() << ")";
}

void CodeGenerator::visitArrayElementExpr(ArrayElementExpr *expr) {
  accept<int>(expr->callee, 0);

  for (int index = 0; index < expr->count; index++)
    accept<int>(expr->indexes[index]);
}

void CodeGenerator::visitDeclarationExpr(DeclarationExpr *expr) {
  accept<int>(expr->initExpr, 0);
}

void CodeGenerator::visitFunctionExpr(FunctionExpr *expr) {
}

void CodeGenerator::visitGetExpr(GetExpr *expr) {
  str() << "(";
  accept<int>(expr->object, 0);
  str() << "). " << expr->name.getString();
}

void CodeGenerator::visitGroupingExpr(GroupingExpr *expr) {
  for (int index = 0; index < expr->count; index++) {
    line();
    accept<int>(expr->expressions[index], 0);
    str() << "\n";
  }

  if (expr->ui)
    accept<int>(expr->ui, 0);
}

void CodeGenerator::visitArrayExpr(ArrayExpr *expr) {
  CodeGenerator generator(parser, expr->function);

  str() << "[";

  for (int index = 0; index < expr->count; index++) {
    if (index)
      str() << ", ";

    accept<int>(expr->expressions[index]);
  }

  str() << "]";
}

void CodeGenerator::visitListExpr(ListExpr *expr) {
  switch(expr->listType) {
  case EXPR_ASSIGN: {
    AssignExpr *subExpr = (AssignExpr *) expr->expressions[expr->count - 1];

    str() << "let " << subExpr->varExp->name.getString() << " = ";
    accept<int>(subExpr->value, 0);
    break;
  }
  case EXPR_CALL: {
    ObjFunction *function = (ObjFunction *) expr->_declaration->type.objType;
    CodeGenerator generator(parser, function);
    Expr *bodyExpr = function->bodyExpr;
    CallExpr *callExpr = (CallExpr *) expr->expressions[1];

    generator.line() << "function " << std::string(function->name->chars, function->name->length) << "(";

    for (int index = 0; index < callExpr->count; index++) {
      ListExpr *param = (ListExpr *)callExpr->arguments[index];
      Expr *paramExpr = param->expressions[1];

      if (index)
        generator.str() << ", ";

      generator.str() << ((ReferenceExpr *)paramExpr)->name.getString();
    }

    generator.str() << ") {\n" << generator.startBlock();

//    for (ObjFunction *child = function->firstChild; function; child = child->next)
//      generator.accept<int>(child->bodyExpr);

    if (bodyExpr)
      generator.accept<int>(bodyExpr);

    if (parser.hadError)
      return;

//    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
//      emitByte(function->upvalues[i].isField ? 1 : 0);
//      emitByte(function->upvalues[i].index);
    }
    generator.endBlock();
    generator.line() << "}";
    break;
  }
  default:
    for (int index = 0; index < expr->count; index++) {
      if (index)
        str() << " ";

      accept<int>(expr->expressions[index], 0);
    }
    break;
  }
}

void CodeGenerator::visitLiteralExpr(LiteralExpr *expr) {
  str() << VALUE(expr->type, expr->as).integer;
}

void CodeGenerator::visitLogicalExpr(LogicalExpr *expr) {
  str() << "(";
  accept<int>(expr->left, 0);
  str() << " " << expr->op.getString() << " ";
  accept<int>(expr->right, 0);
  str() << ")";
}

void CodeGenerator::visitOpcodeExpr(OpcodeExpr *expr) {
  if (expr->right)
    expr->right->accept(this);
}

void CodeGenerator::visitReturnExpr(ReturnExpr *expr) {
  str() << "return (";

  if (expr->value)
    accept<int>(expr->value, 0);

  str() << ")";
}

void CodeGenerator::visitSetExpr(SetExpr *expr) {
  str() << "(";
  accept<int>(expr->object, 0);
  str() << "). " << expr->name.getString() << " = ";
  accept<int>(expr->value, 0);
}

void CodeGenerator::visitStatementExpr(StatementExpr *expr) {
  line();
  expr->expr->accept(this);
}

void CodeGenerator::visitSuperExpr(SuperExpr *expr) {
}

void CodeGenerator::visitTernaryExpr(TernaryExpr *expr) {
  if (expr->right) {
    expr->left->accept(this);
    str() << " ? ";
    expr->middle->accept(this);
    str() << " : ";
    expr->right->accept(this);
  }
  else {
    str() << "if (";
    expr->left->accept(this);
    str() << ") {\n" << startBlock();
    expr->middle->accept(this);
    endBlock();
    line() << "}";
  }
}

void CodeGenerator::visitThisExpr(ThisExpr *expr) {
}

void CodeGenerator::visitTypeExpr(TypeExpr *expr) {
}

void CodeGenerator::visitUnaryExpr(UnaryExpr *expr) {
  switch (expr->op.type) {
    case TOKEN_PERCENT:
      str() << "((";
      accept<int>(expr->right, 0);
      str() << ") / 100)";
      break;

    default:
      str() << expr->op.getString() << "(";
      accept<int>(expr->right, 0);
      str() << ")";
      break;
  }
}

void CodeGenerator::visitReferenceExpr(ReferenceExpr *expr) {
  str() << expr->name.getString();
}

void CodeGenerator::visitSwapExpr(SwapExpr *expr) {
  accept<int>(expr->_expr);
}
