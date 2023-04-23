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

static int nTabs = -1;
std::stringstream s;

bool needsSemicolon(Expr *expr) {
  return expr->type != EXPR_GROUPING && (expr->type != EXPR_LIST || ((ListExpr *) expr)->listType != EXPR_CALL) && (expr->type != EXPR_SWAP || needsSemicolon(((SwapExpr *) expr)->_expr));
}

std::stringstream &CodeGenerator::str() {
  return s;
}

std::stringstream &CodeGenerator::line() {
  for (int index = 0; index < nTabs; index++)
    str() << "  ";

  return str();
}

void CodeGenerator::startBlock() {
  if (nTabs++ >= 0)
    str() << "{\n";
}

void CodeGenerator::endBlock() {
  if (--nTabs >= 0)
    line() << "}\n";
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
    str() << "while(" << expr->left << ") ";
    startBlock();
    accept<int>(expr->right, 0);
    endBlock();
  } else {
    str() << "(";
    accept<int>(expr->left, 0);
    str() << " ";

    switch (expr->op.type) {
      case TOKEN_EQUAL_EQUAL:
        str() << "===";
        break;
      case TOKEN_BANG_EQUAL:
        str() << "!==";
        break;
      default:
        str() << expr->op.getString();
        break;
    }

    str() << " ";
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
  }

  if (expr->handler) {
    if (expr->count)
      str() << ", ";

    {
      CodeGenerator generator(parser, expr->handlerFunction);

      if (expr->handler)
        generator.accept<int>(expr->handler);

      if (parser.hadError)
        return;
    }
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
  startBlock();

  if (function->name ? function->bodyExpr == expr : expr == parser.expr) {
    bool classFlag = function->isClass();
    bool arityFlag = false;

    for (int index = 0; !arityFlag && index < function->arity; index++)
      arityFlag = function->compiler->declarations[index + 1].isField;

//    for (int index = 0; !classFlag && index < function->upvalueCount; index++)
//      classFlag = function->upvalues[index].isField;

    for (int index = 0; index < function->arity; index++) {
      Declaration &declaration = function->compiler->declarations[index + 1];

      if (declaration.isField) {
        std::string name = declaration.name.getString();

        line() << "this." << name << " = " << name << ";\n";
      }
    }

    if (function->isUserClass())
        line() << "this.ReturnHandler_ = ReturnHandler_;\n";

    if (classFlag)
      line() << "const " << function->getThisVariableName() << " = this;\n";
  }

  for (int index = 0; index < expr->count; index++) {
    Expr *subExpr = expr->expressions[index];

    accept<int>(subExpr, 0);
  }

  if (expr->ui)
    accept<int>(expr->ui, 0);

  if (expr == parser.expr)
    line() << "const ui_ = new UI_;\n";

  endBlock();
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

    str() << (expr->_declaration->isField ? "this." : expr->_declaration->function->isClass() ? "const." : "let ") << subExpr->varExp->name.getString() << " = ";
    accept<int>(subExpr->value, 0);
    break;
  }
  case EXPR_CALL: {
    ObjFunction *function = (ObjFunction *) expr->_declaration->type.objType;
    CodeGenerator generator(parser, function);
    Expr *bodyExpr = function->bodyExpr;
    CallExpr *callExpr = (CallExpr *) expr->expressions[1];
    std::string functionName = std::string(function->name->chars, function->name->length);

    generator.str() << "this." << functionName << " = function(";

    for (int index = 0; index < callExpr->count; index++) {
      ListExpr *param = (ListExpr *)callExpr->arguments[index];
      Expr *paramExpr = param->expressions[1];

      if (index)
        generator.str() << ", ";

      generator.str() << ((ReferenceExpr *)paramExpr)->name.getString();
    }

    if (function->isUserClass()) {
      if (callExpr->count)
        generator.str() << ", ";

      generator.str() << "ReturnHandler_";
    }

    str() << ") ";

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
    break;
  }
  case EXPR_REFERENCE: {
    ReferenceExpr *varExp = (ReferenceExpr *) expr->expressions[expr->count - 1];

    str() << (expr->_declaration->isField ? "this." : "let ") << varExp->name.getString() << " = null";
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
  if (function->isClass()) {
    accept<int>(expr->value, 0);
    line() << "return;";
  }
  else {
    line() << "return";

    if (expr->value) {
      str() << " (";
      accept<int>(expr->value, 0);
      str() << ")";
    }

    str() << ";\n";
  }
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

  if (needsSemicolon(expr->expr))
    str() << ";\n";
}

void CodeGenerator::visitSuperExpr(SuperExpr *expr) {
}

void CodeGenerator::visitTernaryExpr(TernaryExpr *expr) {
  if (expr->right) {
    str() << "(";
    expr->left->accept(this);
    str() << " ? ";
    expr->middle->accept(this);
    str() << " : ";
    expr->right->accept(this);
    str() << ")";
  }
  else {
    line() << "if (";
    expr->left->accept(this);
    str() << ") ";
    startBlock();
    expr->middle->accept(this);
    endBlock();
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
  if (expr->_declaration && expr->_declaration->function->isClass())
    if (expr->_declaration->function == function)
      str() << "this." << expr->_declaration->name.getString();
    else
      str() << expr->_declaration->function->getThisVariableName() << "." << expr->_declaration->name.getString();
  else
    str() << expr->name.getString();
}

void CodeGenerator::visitSwapExpr(SwapExpr *expr) {
  accept<int>(expr->_expr);
}
