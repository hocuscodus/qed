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

  if (expr == parser.expr) {
    line() << "const canvas = document.getElementById(\"canvas\");\n";
    line() << "let postCount = 1;\n";
    line() << "let attributeStacks = [];\n";
    line() << "const ctx = canvas.getContext(\"2d\");\n";
    line() << "function toColor(color) {return \"#\" + color.toString(16).padStart(6, '0');}\n";
    line() << "let getAttribute = function(index) {\n";
    line() << "  return attributeStacks[index][attributeStacks[index].length - 1];\n";
    line() << "}\n";
  }

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

  if (expr == parser.expr) {
    line() << "this.pushAttribute(4, 20);\n";
    line() << "this.pushAttribute(10, 0);\n";
    line() << "this.pushAttribute(11, 1.0);\n";
    line() << "function _refresh() {\n";
    line() << "  if (--postCount == 0) {\n";
    line() << "    globalThis$.ui_ = new globalThis$.UI_();\n";
    line() << "    globalThis$.layout_ = new globalThis$.ui_.Layout_();\n";
    line() << "    ctx.globalAlpha = 1.0;\n";
    line() << "    ctx.clearRect(0, 0, canvas.width, canvas.height);\n";
    line() << "    globalThis$.layout_.paint(0, 0, globalThis$.layout_.size >> 16, globalThis$.layout_.size & 65535);\n";
    line() << "  }\n";
    line() << "}\n";
    line() << "_refresh();\n";
    line() << "canvas.addEventListener(\"mousedown\", function(ev) {\n";
    line() << "  postCount++;\n";
    line() << "  var rect = canvas.getBoundingClientRect();\n";
    line() << "  globalThis$.layout_.onEvent(0, ev.clientX - rect.left, ev.clientY - rect.top, globalThis$.layout_.size >> 16, globalThis$.layout_.size & 65535);\n";
    line() << "  globalThis$._refresh();\n";
    line() << "  });\n";
    line() << "canvas.addEventListener(\"mouseup\", function(ev) {\n";
    line() << "  postCount++;\n";
    line() << "  var rect = canvas.getBoundingClientRect();\n";
    line() << "  globalThis$.layout_.onEvent(1, ev.clientX - rect.left, ev.clientY - rect.top, globalThis$.layout_.size >> 16, globalThis$.layout_.size & 65535);\n";
    line() << "  globalThis$._refresh();\n";
    line() << "});\n";
    line() << "canvas.onselectstart = function () { return false; }\n";
  }

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

    str() << (subExpr->varExp->_declaration->isField ? "this." : subExpr->varExp->_declaration->function->isClass() ? "const." : "let ") << subExpr->varExp->_declaration->getRealName() << " = ";
    accept<int>(subExpr->value, 0);
    break;
  }
  case EXPR_CALL: {
    ObjFunction *function = (ObjFunction *) expr->_declaration->type.objType;
    CallExpr *callExpr = (CallExpr *) expr->expressions[1];
    ReferenceExpr *varExp = (ReferenceExpr *)callExpr->callee;
    ObjFunction *function2 = (ObjFunction *) varExp->_declaration->type.objType;

    if (function != function2)
      function2 = function;

    CodeGenerator generator(parser, function);
    Expr *bodyExpr = function->bodyExpr;

    generator.str() << (varExp->_declaration->isField ? "this." : "let ");// << varExp->name.getString() << " = null";
    generator.str() << varExp->_declaration->getRealName() << " = function(";

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

    str() << (varExp->_declaration->isField ? "this." : "let ") << varExp->name.getString() << " = null";
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
  switch (expr->type) {
    case VAL_BOOL:
      str() << (expr->as.boolean ? "true" : "false");
      break;

    case VAL_FLOAT:
      str() << expr->as.floating;
      break;

    case VAL_INT:
      str() << expr->as.integer;
      break;

    case VAL_OBJ:
      switch (expr->as.obj->type) {
        case OBJ_STRING:
          str() << "\"" << std::string(((ObjString *) expr->as.obj)->chars, ((ObjString *) expr->as.obj)->length) << "\"";
          break;

        default:
          str() << "null";
          break;
      }
      break;

    default:
      str() << "null";
      break;
  }
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
  if (expr->_declaration)
    if (expr->_declaration->function->isClass())
      if (expr->_declaration->function == function)
        str() << "this." << expr->_declaration->getRealName();
      else
        str() << expr->_declaration->function->getThisVariableName() << "." << expr->_declaration->getRealName();
    else
      str() << expr->_declaration->getRealName();
  else
    str() << expr->name.getString();
}

void CodeGenerator::visitSwapExpr(SwapExpr *expr) {
  accept<int>(expr->_expr);
}

void CodeGenerator::visitNativeExpr(NativeExpr *expr) {
  str() << std::string(expr->nativeCode.start, expr->nativeCode.length);
}
