/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include "parser.hpp"
#include "compiler.hpp"

static int nTabs = 0;
std::stringstream s;

bool needsSemicolon(Expr *expr) {
  return !isGroup(expr, TOKEN_SEPARATOR) && expr->type != EXPR_GROUPING && expr->type != EXPR_IF && expr->type != EXPR_RETURN && expr->type != EXPR_WHILE && expr->type != EXPR_FUNCTION && !(expr->type == EXPR_SWAP && !needsSemicolon(((SwapExpr *) expr)->_expr));
}

static ObjFunction *getFunction(Expr *callee) {
  switch(callee->type) {
    case EXPR_REFERENCE: {
        ReferenceExpr *reference = (ReferenceExpr *) callee;

        return reference->declaration && reference->declaration->type == EXPR_FUNCTION ? &(((FunctionExpr *) reference->declaration)->_function) : NULL;
      }
    case EXPR_GET: {
        GetExpr *getExpr = (GetExpr *) callee;
        Type decType = getDeclarationType(getExpr->_declaration->expr);

        return getExpr->_declaration && IS_FUNCTION(decType) ? AS_FUNCTION_TYPE(decType) : NULL;
      }
    case EXPR_FUNCTION: return &((FunctionExpr *) callee)->_function;
    case EXPR_GROUPING: {
        GroupingExpr *group = (GroupingExpr *) callee;

        if(group->name.type != TOKEN_LEFT_BRACE) {
          Expr *lastExpr = *getLastBodyExpr(&group->body, TOKEN_SEPARATOR);

          return lastExpr ? getFunction(lastExpr) : NULL;
        }

        return NULL;
      }
    default: return NULL;
  }
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

void IteratorExpr::toCode(Parser &parser, ObjFunction *function) {
  value->toCode(parser, function);
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
      break;
  }
}

void CallExpr::toCode(Parser &parser, ObjFunction *function) {
  ObjFunction *calleeFunction = getFunction(callee);

  if (calleeFunction && calleeFunction->isClass())
    str() << "new ";

  callee->toCode(parser, function);
  str() << "(";

  if (params)
    params->toCode(parser, function);

  if (handler)
    handler = NULL;

  str() << ")";
}

void ArrayElementExpr::toCode(Parser &parser, ObjFunction *function) {
  callee->toCode(parser, function);

  for (int index = 0; index < count; index++) {
    str() << "[";
    indexes[index]->toCode(parser, function);
    str() << "]";
  }
}

void DeclarationExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << (_declaration.isExternalField() ? "this." : /*_declaration->function->isClass() ? "const " : */"let ") << _declaration.getRealName() << " = ";

  if (initExpr)
    initExpr->toCode(parser, function);
  else
    str() << "null";
}

void FunctionExpr::toCode(Parser &parser, ObjFunction *function) {
  pushScope(this);

  if (getCurrent()->enclosing) {
    if (getFunction(getCurrent()->enclosing)->_declaration.isInRegularFunction())
      str() << "this." << _declaration.getRealName() << " = ";

    str() << "function " << _declaration.getRealName() << "(";

    for (int index = 0; index < arity; index++) {
      if (index)
        str() << ", ";

      str() << getParam(this, index)->_declaration.name.getString();
    }

    str() << ") ";
  }

  if (body)
    body->toCode(parser, &_function);

  popScope();
}

void GetExpr::toCode(Parser &parser, ObjFunction *function) {
  object->toCode(parser, function);
  str() << "." << name.getString();
}

void GroupingExpr::toCode(Parser &parser, ObjFunction *function) {
  bool functionFlag = getCurrent()->group == this;

  if (!functionFlag)
    pushScope(this);

  if (name.type != TOKEN_LEFT_BRACE) {
    str() << "(";
    body->toCode(parser, function);
    str() << ")";
  } else {
    if (getCurrent()->enclosing)
      startBlock();

    if (function->expr->body == this && function->isClass()) {
        for (int index = 0; index < function->expr->arity - 1; index++) {
          DeclarationExpr *declaration = getParam(function->expr, index);

          if (isField(function->expr, declaration)) {
            std::string name = declaration->_declaration.name.getString();

            line() << "this." << name << " = " << name << ";\n";
          }
        }

        for (Expr *body = function->expr->body->body; body; body = cdr(body, TOKEN_SEPARATOR)) {
          Expr *expr = car(body, TOKEN_SEPARATOR);

          if (expr->type == EXPR_DECLARATION && ((DeclarationExpr *) expr)->isInternalField) {
            line() << "const " << function->getThisVariableName() << " = this;\n";
            break;
          }
        }
      }

    if (body) {
      line();
      body->toCode(parser, function);

      if (!isGroup(body, TOKEN_SEPARATOR) && needsSemicolon(body))
        str() << ";\n";
    }

    if (function->expr->body == this && function->expr->ui)
      function->expr->ui->toCode(parser, function);

    if (getCurrent()->enclosing)
      endBlock();
  }

  if (!functionFlag)
    popScope();
}

void IfExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "if (";
  condition->toCode(parser, function);
  str() << ") ";
  startBlock();
  line();
  thenBranch->toCode(parser, function);

  if (needsSemicolon(thenBranch))
    str() << ";\n";

  endBlock();

  if (elseBranch) {
    line() << "else ";
    startBlock();
    line();
    elseBranch->toCode(parser, function);

    if (needsSemicolon(elseBranch))
      str() << ";\n";

    endBlock();
  }
}

void ArrayExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "[";

  if (body)
    body->toCode(parser, function);

  str() << "]";
}
/*
  this.size = 2;
  this.dims = [];
  this.array = [][];

  this.dims[0] = 10;
  this.dims[1] = 10;
;
  for (let x0 = 0; x0 < this.dims[0]; x0++) {
    for (let x1 = 0; x1 < this.dims[1]; x1++) {
      this.array = 11;
    }
  }
10 {
	{
		10 {
			{
				; @out((&1 + 1) * (&0 + 1)) @size(16) @bgcol(0xDDDDDD)
			} @out(&0 + 1) @cdir(1) @bgcol(0x0000FF) @textcol(0xFFFFFF) @size(14)
		} @adir(2) @apack(0)
	} @out(&0 + 1) @cdir(2) @bgcol(0x0000FF) @textcol(0xFFFFFF) @size(14)
} @adir(1) @apack(0)
*/
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
          str() << "\"" << std::string(((ObjString *) as.obj)->str) << "\"";
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
  left->toCode(parser, function);
  str() << " " << op.getString() << " ";
  right->toCode(parser, function);
}

void ReturnExpr::toCode(Parser &parser, ObjFunction *function) {
  if (isUserClass) {
    value->toCode(parser, function);
    str() << ";\n";
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
  object->toCode(parser, function);
  str() << "." << name.getString() << " = ";
  value->toCode(parser, function);
}

void TernaryExpr::toCode(Parser &parser, ObjFunction *function) {
  left->toCode(parser, function);
  str() << " ? ";
  middle->toCode(parser, function);
  str() << " : ";
  right->toCode(parser, function);
}

void ThisExpr::toCode(Parser &parser, ObjFunction *function) {
}

void CastExpr::toCode(Parser &parser, ObjFunction *function) {
  expr->toCode(parser, function);
}

void WhileExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << "while(";
  condition->toCode(parser, function);
  str() << ") ";
  body->toCode(parser, function);

  if (needsSemicolon(body))
    str() << ";\n";
}

void UnaryExpr::toCode(Parser &parser, ObjFunction *function) {
  switch (op.type) {
    case TOKEN_PERCENT:
      str() << "((";
      right->toCode(parser, function);
      str() << ") / 100)";
      break;

    default:
      str() << op.getString();
      right->toCode(parser, function);
      break;
  }
}

void PrimitiveExpr::toCode(Parser &parser, ObjFunction *function) {
}

void ReferenceExpr::toCode(Parser &parser, ObjFunction *function) {
  Declaration *declaration = getDeclaration(this->declaration);

  if (declaration)
    if (declaration->isExternalField())
      if (declaration->function == function->expr)
        str() << "this." << declaration->getRealName();
      else
        str() << declaration->function->_function.getThisVariableName() << "." << declaration->getRealName();
    else
      str() << declaration->getRealName();
  else
    str() << name.getString();
}

void SwapExpr::toCode(Parser &parser, ObjFunction *function) {
  _expr->toCode(parser, function);
}

void NativeExpr::toCode(Parser &parser, ObjFunction *function) {
  str() << std::string(nativeCode.start, nativeCode.length);
}
