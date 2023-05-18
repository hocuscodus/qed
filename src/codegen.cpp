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
#include "parser.hpp"
#include "attrset.hpp"
#include "debug.hpp"

static int nTabs = -1;
std::stringstream s;

bool needsSemicolon(Expr *expr) {
  return expr->type != EXPR_GROUPING && expr->type != EXPR_FUNCTION && !(expr->type == EXPR_SWAP && !needsSemicolon(((SwapExpr *) expr)->_expr));
}

static std::stringstream &str() {
  return s;
}

static std::stringstream &line() {
  for (int index = 0; index < nTabs; index++)
    str() << "  ";

  return str();
}

static void startBlock() {
  if (nTabs++ >= 0)
    str() << "{\n";
}

static void endBlock() {
  if (--nTabs >= 0)
    line() << "}\n";
}

void AssignExpr::toCode(Parser &parser, ObjFunction *function) {
  if (varExp)
    varExp->toCode(parser, function);

  if (value) {
    str() << " " << op.getString() << " ";
    value->toCode(parser, function);
  }
  else
    str() << op.getString();
}

void UIAttributeExpr::toCode(Parser &parser, ObjFunction *function) {
  parser.error("Cannot generate UI code from UI expression.");
}

void UIDirectiveExpr::toCode(Parser &parser, ObjFunction *function) {
  parser.error("Cannot generate UI code from UI expression.");
}

void BinaryExpr::toCode(Parser &parser, ObjFunction *function) {
  if (op.type == TOKEN_WHILE) {
    str() << "while(" << left << ") ";
    startBlock();
    right->toCode(parser, function);
    endBlock();
  } else {
    str() << "(";
    left->toCode(parser, function);
    str() << " ";

    switch (op.type) {
      case TOKEN_EQUAL_EQUAL:
        str() << "===";
        break;
      case TOKEN_BANG_EQUAL:
        str() << "!==";
        break;
      default:
        str() << op.getString();
        break;
    }

    str() << " ";
    right->toCode(parser, function);
    str() << ")";
  }
}

void CallExpr::toCode(Parser &parser, ObjFunction *function) {
  if (newFlag)
    str() << "new ";

  callee->toCode(parser, function);
  str() << "(";

  for (int index = 0; index < count; index++) {
    if (index)
      str() << ", ";

    arguments[index]->toCode(parser, function);
  }

  if (handler) {
    if (count)
      str() << ", ";

    if (handler)
      handler->toCode(parser, handlerFunction);

    if (parser.hadError)
      return;
  }

  str() << ")";
}

void ArrayElementExpr::toCode(Parser &parser, ObjFunction *function) {
  callee->toCode(parser, function);

  for (int index = 0; index < count; index++)
    indexes[index]->toCode(parser, function);
}

void DeclarationExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << (_declaration->isField ? "this." : _declaration->function->isClass() ? "const." : "let ") << _declaration->getRealName() << " = ";

  if (initExpr)
    initExpr->toCode(parser, function);
  else
    str() << "null";
}

void FunctionExpr::toCode(Parser &parser, ObjFunction *function) {
  ObjFunction *function2 = (ObjFunction *) _declaration->type.objType;

  if (this->function != function2)
    function2 = this->function;

  str() << (_declaration->isField ? "this." : "let ");
  str() << _declaration->getRealName() << " = function(";

  for (int index = 0; index < count; index++) {
    if (index)
      str() << ", ";

    str() << params[index]->name.getString();
  }

  if (this->function->isUserClass()) {
    if (count)
      str() << ", ";

    str() << "ReturnHandler_";
  }

  str() << ") ";

//    for (ObjFunction *child = this->function->firstChild; this->function; child = child->next)
//      generator.accept<int>(child->bodyExpr);
  if (body)
    body->toCode(parser, this->function);

  if (parser.hadError)
    return;

//    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(this->function)));

  for (int i = 0; i < this->function->upvalueCount; i++) {
//      emitByte(this->function->upvalues[i].isField ? 1 : 0);
//      emitByte(this->function->upvalues[i].index);
  }
}

void GetExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "(";
  object->toCode(parser, function);
  str() << "). " << name.getString();
}

void GroupingExpr::toCode(Parser &parser, ObjFunction *function) {
  startBlock();

  if (this == parser.expr) {
    line() << "const canvas = document.getElementById(\"canvas\");\n";
    line() << "let postCount = 1;\n";
    line() << "let attributeStacks = [];\n";
    line() << "const ctx = canvas.getContext(\"2d\");\n";
    line() << "function toColor(color) {return \"#\" + color.toString(16).padStart(6, '0');}\n";
    line() << "let getAttribute = function(index) {\n";
    line() << "  return attributeStacks[index][attributeStacks[index].length - 1];\n";
    line() << "}\n";
  }

  if (function->name ? function->bodyExpr == this : this == parser.expr) {
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

  for (int index = 0; index < count; index++) {
    Expr *subExpr = expressions[index];

    subExpr->toCode(parser, function);
  }

  if (ui)
    ui->toCode(parser, function);

  if (this == parser.expr) {
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

void ArrayExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "[";

  for (int index = 0; index < count; index++) {
    if (index)
      str() << ", ";

    expressions[index]->toCode(parser, function);
  }

  str() << "]";
}

void ListExpr::toCode(Parser &parser, ObjFunction *function) {
  for (int index = 0; index < count; index++) {
    if (index)
      str() << " ";

    expressions[index]->toCode(parser, function);
  }
}

void LiteralExpr::toCode(Parser &parser, ObjFunction *function) {
  switch (type) {
    case VAL_BOOL:
      str() << (as.boolean ? "true" : "false");
      break;

    case VAL_FLOAT:
      str() << as.floating;
      break;

    case VAL_INT:
      str() << as.integer;
      break;

    case VAL_OBJ:
      switch (as.obj->type) {
        case OBJ_STRING:
          str() << "\"" << std::string(((ObjString *) as.obj)->chars, ((ObjString *) as.obj)->length) << "\"";
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

void LogicalExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "(";
  left->toCode(parser, function);
  str() << " " << op.getString() << " ";
  right->toCode(parser, function);
  str() << ")";
}

void OpcodeExpr::toCode(Parser &parser, ObjFunction *function) {
  if (right)
    right->toCode(parser, function);
}

void ReturnExpr::toCode(Parser &parser, ObjFunction *function) {
  if (function->isClass()) {
    value->toCode(parser, function);
    line() << "return;";
  }
  else {
    line() << "return";

    if (value) {
      str() << " (";
      value->toCode(parser, function);
      str() << ")";
    }

    str() << ";\n";
  }
}

void SetExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "(";
  object->toCode(parser, function);
  str() << "). " << name.getString() << " = ";
  value->toCode(parser, function);
}

void StatementExpr::toCode(Parser &parser, ObjFunction *function) {
  line();
  expr->toCode(parser, function);

  if (needsSemicolon(expr))
    str() << ";\n";
}

void SuperExpr::toCode(Parser &parser, ObjFunction *function) {
}

void TernaryExpr::toCode(Parser &parser, ObjFunction *function) {
  if (right) {
    str() << "(";
    left->toCode(parser, function);
    str() << " ? ";
    middle->toCode(parser, function);
    str() << " : ";
    right->toCode(parser, function);
    str() << ")";
  }
  else {
    line() << "if (";
    left->toCode(parser, function);
    str() << ") ";
    startBlock();
    middle->toCode(parser, function);
    endBlock();
  }
}

void ThisExpr::toCode(Parser &parser, ObjFunction *function) {
}

void TypeExpr::toCode(Parser &parser, ObjFunction *function) {
}

void UnaryExpr::toCode(Parser &parser, ObjFunction *function) {
  switch (op.type) {
    case TOKEN_PERCENT:
      str() << "((";
      right->toCode(parser, function);
      str() << ") / 100)";
      break;

    default:
      str() << op.getString() << "(";
      right->toCode(parser, function);
      str() << ")";
      break;
  }
}

void ReferenceExpr::toCode(Parser &parser, ObjFunction *function) {
  if (_declaration)
    if (_declaration->function->isClass())
      if (_declaration->function == function)
        str() << "this." << _declaration->getRealName();
      else
        str() << _declaration->function->getThisVariableName() << "." << _declaration->getRealName();
    else
      str() << _declaration->getRealName();
  else
    str() << name.getString();
}

void SwapExpr::toCode(Parser &parser, ObjFunction *function) {
  _expr->toCode(parser, function);
}

void NativeExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << std::string(nativeCode.start, nativeCode.length);
}
