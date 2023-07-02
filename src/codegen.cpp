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

static int nTabs = 0;
std::stringstream s;

bool needsSemicolon(Expr *expr) {
  return !isGroup(expr, TOKEN_SEPARATOR) && expr->type != EXPR_GROUPING && expr->type != EXPR_IF && expr->type != EXPR_RETURN && expr->type != EXPR_WHILE && expr->type != EXPR_FUNCTION && !(expr->type == EXPR_SWAP && !needsSemicolon(((SwapExpr *) expr)->_expr));
}

static std::stringstream &str() {
  return s;
}

std::stringstream &line() {
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
  switch (op.type) {
    case TOKEN_SEPARATOR:
      left->toCode(parser, function);

      if (needsSemicolon(left))
        str() << ";\n";

      line();
      right->toCode(parser, function);

      if (needsSemicolon(right))
        str() << ";\n";
      break;
    case TOKEN_COMMA:
      left->toCode(parser, function);
      str() << ", ";
      right->toCode(parser, function);
      break;
    default:
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
      break;
  }
}

void CallExpr::toCode(Parser &parser, ObjFunction *function) {
  if (newFlag)
    str() << "new ";

  callee->toCode(parser, function);
  str() << "(";

  if (params)
    params->toCode(parser, function);

  if (handler) {
    if (params)
      str() << ", ";

    if (handler)
      handler->toCode(parser, &((FunctionExpr *) handler)->_function);

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
  ObjFunction *function2 = (ObjFunction *) (_declaration ? _declaration->type.objType : NULL);

  if (function2 && function != function2)
    function2 = function;

  if (!body || body->_compiler.enclosing) {
    str() << (_declaration->isField ? "this." : "let ");
    str() << _declaration->getRealName() << " = function(";

    for (int index = 0; index < arity; index++) {
      if (index)
        str() << ", ";

      str() << params[index]->name.getString();
    }
  /*
    if (_function.isUserClass()) {
      if (arity)
        str() << ", ";

      str() << "ReturnHandler_";
    }
  */
      str() << ") ";
  }

//    for (ObjFunction *child = this->function->firstChild; this->function; child = child->next)
//      generator.accept<int>(child->bodyExpr);
  if (body)
    body->toCode(parser, &_function);

  if (parser.hadError)
    return;

//    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(this->function)));

  for (int i = 0; i < _function.upvalueCount; i++) {
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
  if (_compiler.enclosing)
    startBlock();

  if (function->expr->body == this) {
    bool classFlag = function->isClass();
    bool arityFlag = false;

    for (int index = 0; !arityFlag && index < function->expr->arity; index++)
      arityFlag = function->compiler->declarations[index].isField;

//    for (int index = 0; !classFlag && index < function->upvalueCount; index++)
//      classFlag = function->upvalues[index].isField;

    for (int index = 0; index < function->expr->arity; index++) {
      Declaration &declaration = function->compiler->declarations[index];

      if (declaration.isField) {
        std::string name = declaration.name.getString();

        line() << "this." << name << " = " << name << ";\n";
      }
    }
/*
    if (function->isUserClass())
        line() << "this.ReturnHandler_ = ReturnHandler_;\n";
*/
    if (classFlag)
      line() << "const " << function->getThisVariableName() << " = this;\n";
  }

  if (body) {
    line();
    body->toCode(parser, function);

    if (!isGroup(body, TOKEN_SEPARATOR) && needsSemicolon(body))
      str() << ";\n";
  }

  if (function->expr->body == this && function->expr->ui)
    function->expr->ui->toCode(parser, function);

  if (_compiler.enclosing)
    endBlock();
}

void IfExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "if (";
  condition->toCode(parser, function);
  str() << ") ";
  startBlock();
  line();
  thenBranch->toCode(parser, function);
  endBlock();

  if (elseBranch) {
    str() << " else ";
    startBlock();
    line();
    elseBranch->toCode(parser, function);
    endBlock();
  }
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

void ReturnExpr::toCode(Parser &parser, ObjFunction *function) {
  if (function->isClass()) {
    if (value)
      value->toCode(parser, function);

    line() << "return;\n";
  }
  else {
    str() << "return";

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

void TernaryExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "(";
  left->toCode(parser, function);
  str() << " ? ";
  middle->toCode(parser, function);
  str() << " : ";
  right->toCode(parser, function);
  str() << ")";
}

void ThisExpr::toCode(Parser &parser, ObjFunction *function) {
}

void TypeExpr::toCode(Parser &parser, ObjFunction *function) {
  if (declaration)
    if (declaration->function->isClass())
      if (declaration->function == function)
        str() << "this." << declaration->getRealName();
      else
        str() << declaration->function->getThisVariableName() << "." << declaration->getRealName();
    else
      str() << declaration->getRealName();
  else
    str() << name.getString();
}

void CastExpr::toCode(Parser &parser, ObjFunction *function) {
  expr->toCode(parser, function);
}

void WhileExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "while(" << condition << ") ";
  startBlock();
  body->toCode(parser, function);
  endBlock();
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
