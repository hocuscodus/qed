/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include "parser.hpp"
#include "compiler.hpp"

static int nTabs = 0;
const std::string emptyString;

bool needsSemicolon(Expr *expr) {
  return !isGroup(expr, TOKEN_SEPARATOR) && expr->type != EXPR_GROUPING && expr->type != EXPR_IF && expr->type != EXPR_WHILE && expr->type != EXPR_FUNCTION && !(expr->type == EXPR_SWAP && !needsSemicolon(((SwapExpr *) expr)->expr));
}

static ObjFunction *getFunction(Expr *callee) {
  switch(callee->type) {
    case EXPR_REFERENCE: {
        ReferenceExpr *reference = (ReferenceExpr *) callee;

        if (reference->declaration)
          switch (reference->declaration->type) {
            case EXPR_FUNCTION: return &(((FunctionExpr *) reference->declaration)->_function);
            case EXPR_DECLARATION: return AS_FUNCTION_TYPE(((DeclarationExpr *) reference->declaration)->_declaration.type);
            default: return NULL;
          }

        return NULL;
      }
    case EXPR_GET: {
        GetExpr *getExpr = (GetExpr *) callee;
        Type decType = getDeclarationType(getExpr->_declaration->expr);

        return getExpr->_declaration && IS_FUNCTION(decType) ? AS_FUNCTION_TYPE(decType) : NULL;
      }
    case EXPR_FUNCTION:
      return &((FunctionExpr *) callee)->_function;
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

std::stringstream &line(std::stringstream &str) {
  for (int index = 0; index < nTabs; index++)
    str << "  ";

  return str;
}

static void startBlock(std::stringstream &str) {
  if (nTabs++ >= 0)
    str << "{\n";
}

static void endBlock(std::stringstream &str) {
  if (--nTabs >= 0)
    line(str) << "}";
}

static void blockToCode(std::stringstream &str, Parser &parser, ObjFunction *function, Expr *statementRef, bool statementFlag) {
  bool braceFlag = isGroup(statementRef, TOKEN_SEPARATOR);

  if (!braceFlag && statementRef->type == EXPR_GROUPING && ((GroupingExpr *) statementRef)->name.type == TOKEN_LEFT_BRACE) {
    braceFlag = true;
    statementRef = ((GroupingExpr *) statementRef)->body;
  }

  if (braceFlag) {
    if (statementFlag)
      str << " ";

    startBlock(str);

    for (; statementRef; statementRef = cdr(statementRef, TOKEN_SEPARATOR)) {
      Expr *statement = car(statementRef, TOKEN_SEPARATOR);

      line(str) << statement->toCode(parser, function);

      if (needsSemicolon(statement))
        str << ";";

      str << "\n";
    }
    endBlock(str);
  }
  else {
    if (statementFlag) {
      nTabs++;
      str << "\n";
      line(str);
    }

    str << statementRef->toCode(parser, function);

    if (needsSemicolon(statementRef))
      str << ";";

    if (statementFlag)
      nTabs--;
  }
}

static std::string variableToCode(Declaration *declaration, ObjFunction *function) {
  if (declaration)
    if (declaration->function && isExternalField(declaration->function, declaration))
      if (declaration->function == function->expr)
        return (std::stringstream() << "this." << declaration->getRealName()).str();
      else
        return (std::stringstream() << declaration->function->_function.getThisVariableName() << "." << declaration->getRealName()).str();
    else
      return declaration->getRealName();
  else
    return NULL;
}

std::string IteratorExpr::toCode(Parser &parser, ObjFunction *function) {
  return value->toCode(parser, function);
}

std::string AssignExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  if (varExp)
    str << varExp->toCode(parser, function);

  if (value)
    str << " " << op.getString() << " " << value->toCode(parser, function);
  else
    str << op.getString();

  return str.str();
}

std::string UIAttributeExpr::toCode(Parser &parser, ObjFunction *function) {
  parser.error("Cannot generate UI code from UI expression.");
  return emptyString;
}

std::string UIDirectiveExpr::toCode(Parser &parser, ObjFunction *function) {
  parser.error("Cannot generate UI code from UI expression.");
  return emptyString;
}

std::string BinaryExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  switch (op.type) {
    case TOKEN_SEPARATOR:
      str << left->toCode(parser, function);

      if (needsSemicolon(left))
        str << ";\n";

      line(str);
      str << right->toCode(parser, function);

      if (needsSemicolon(right))
        str << ";\n";
      break;
    case TOKEN_COMMA:
      str << left->toCode(parser, function) << ", " << right->toCode(parser, function);
      break;
    default:
      str << left->toCode(parser, function) << " ";

      switch (op.type) {
        case TOKEN_EQUAL_EQUAL:
          str << "===";
          break;
        case TOKEN_BANG_EQUAL:
          str << "!==";
          break;
        default:
          str << op.getString();
          break;
      }

      str << " " << right->toCode(parser, function);
      break;
  }

  return str.str();
}

std::string CallExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;
  ObjFunction *calleeFunction = getFunction(callee);

  if (calleeFunction && isClass(calleeFunction->expr))
    str << "new ";

  str << callee->toCode(parser, function) << "(";

  if (args)
    str << args->toCode(parser, function);

  if (handler)
    handler = NULL;

  str << ")";
  return str.str();
}

std::string ArrayElementExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  str << callee->toCode(parser, function);

  for (int index = 0; index < count; index++)
    str << "[" << indexes[index]->toCode(parser, function) << "]";

  return str.str();
}

std::string DeclarationExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

//  if (initExpr) {
    if (_declaration.function && isExternalField(_declaration.function, &_declaration))
      str << variableToCode(&_declaration, function);
    else
      str << /*_declaration->function->isClass() ? "const " : */"let " << _declaration.getRealName();

    str << " = " << (initExpr ? initExpr->toCode(parser, function) : "null");
//  }

  return str.str();
}

std::string FunctionExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  pushScope(this);

  if (getCurrent()->enclosing) {
    if (isInRegularFunction(getCurrent()->enclosing->getFunction()))
      str << "this." << _declaration.getRealName() << " = function(";
    else
      str << "function " << _declaration.getRealName() << "(";

    for (int index = 0; index < arity; index++) {
      if (index)
        str << ", ";

      str << getParam(this, index)->_declaration.getRealName();
    }

    str << ") ";
  }

  if (getCurrent()->enclosing)
    startBlock(str);

  for (int index = 0; index < arity - (_declaration.name.isUserClass() ? 1 : 0); index++) {
    DeclarationExpr *declarationExpr = getParam(this, index);

    if (isField(this, &declarationExpr->_declaration)) {
      std::string name = declarationExpr->_declaration.getRealName();

      line(str) << "this." << name << " = " << name << ";\n";
    }
  }

  if (_function.hasInternalFields)
    line(str) << "const " << _function.getThisVariableName() << " = this;\n";

  for (Expr *statementRef = body->body; statementRef; statementRef = cdr(statementRef, TOKEN_SEPARATOR)) {
    Expr *statement = car(statementRef, TOKEN_SEPARATOR);

    line(str) << statement->toCode(parser, &_function);

    if (needsSemicolon(statement))
      str << ";";

    str << "\n";
  }

  if (getCurrent()->enclosing)
    endBlock(str);

  popScope();
  return str.str();
}

std::string GetExpr::toCode(Parser &parser, ObjFunction *function) {
  return (std::stringstream() << object->toCode(parser, function) << "." << name.getString()).str();
}

std::string GroupingExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;
  bool functionFlag = getCurrent()->getGroup() == this;

  if (!functionFlag)
    pushScope(this);

  if (name.type != TOKEN_LEFT_BRACE && !isGroup(body, TOKEN_SEPARATOR))
    str << "(" << body->toCode(parser, function) << ")";
  else
    blockToCode(str, parser, function, body, false);

  if (!functionFlag)
    popScope();

  return str.str();
}

std::string IfExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  str << "if (" << condition->toCode(parser, function) << ")";
  blockToCode(str, parser, function, thenBranch, true);

  if (elseBranch) {
    str << "\n";
    line(str) << "else";
    blockToCode(str, parser, function, elseBranch, true);
  }

  return str.str();
}

std::string ArrayExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  str << "[";

  for (Expr *statementRef = body; statementRef; statementRef = cdr(statementRef, TOKEN_COMMA)) {
    Expr *statement = car(statementRef, TOKEN_COMMA);

    if (statementRef != body)
      str << ", ";

    str << statement->toCode(parser, function);
  }

  str << "]";
  return str.str();
}

std::string LiteralExpr::toCode(Parser &parser, ObjFunction *function) {
  switch (type) {
    case VAL_BOOL: return as.boolean ? "true" : "false";
    case VAL_FLOAT: return std::to_string(as.floating);
    case VAL_INT: return std::to_string(as.integer);
    case VAL_OBJ:
      switch (as.obj->type) {
        case OBJ_STRING: return (std::stringstream() << '"' << std::string(((ObjString *) as.obj)->str) << '"').str();
        default: return "null";
      }
    default: return "null";
  }
}

std::string LogicalExpr::toCode(Parser &parser, ObjFunction *function) {
  return (std::stringstream() << left->toCode(parser, function) << " " << op.getString() << " " << right->toCode(parser, function)).str();
}

std::string ReturnExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  str << "return";

  if (value)
    str << " (" << value->toCode(parser, function) << ")";

  return str.str();
}

std::string SetExpr::toCode(Parser &parser, ObjFunction *function) {
  return (std::stringstream() << object->toCode(parser, function) << "." << name.getString() << " = " << value->toCode(parser, function)).str();
}

std::string TernaryExpr::toCode(Parser &parser, ObjFunction *function) {
  return (std::stringstream() << left->toCode(parser, function) << " ? " << middle->toCode(parser, function) << " : " << right->toCode(parser, function)).str();
}

std::string ThisExpr::toCode(Parser &parser, ObjFunction *function) {
  return emptyString;
}

std::string CastExpr::toCode(Parser &parser, ObjFunction *function) {
  return expr->toCode(parser, function);
}

std::string WhileExpr::toCode(Parser &parser, ObjFunction *function) {
  std::stringstream str;

  str << "while(" << condition->toCode(parser, function) << ")";
  blockToCode(str, parser, function, body, true);

  return str.str();
}

std::string UnaryExpr::toCode(Parser &parser, ObjFunction *function) {
  switch (op.type) {
    case TOKEN_PERCENT: return (std::stringstream() << "((" << right->toCode(parser, function) << ") / 100)").str();
    default: return (std::stringstream() << op.getString() << right->toCode(parser, function)).str();
  }
}

std::string PrimitiveExpr::toCode(Parser &parser, ObjFunction *function) {
  return emptyString;
}

std::string ReferenceExpr::toCode(Parser &parser, ObjFunction *function) {
  Declaration *declaration = this->declaration ? getDeclaration(this->declaration) : NULL;

  return declaration ? variableToCode(declaration, function) : name.getString();
}

std::string SwapExpr::toCode(Parser &parser, ObjFunction *function) {
  return expr->toCode(parser, function);
}

std::string NativeExpr::toCode(Parser &parser, ObjFunction *function) {
  return std::string(nativeCode.start, nativeCode.length);
}
