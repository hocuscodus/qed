/*
 * The QED Programming Language
 * Copyright (C) 2022-2024  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <list>
#include <string.h>
#include "parser.hpp"
#include "compiler.hpp"

struct ExprInfo {
  Expr *&expr;
  int index;
  ExprInfo *parent;

  ExprInfo(Expr *&expr, ExprInfo *parent);
  Token getNextToken(ExprStack *exprStack, Parser &parser);
};

struct ExprStack {
  ExprInfo *top = NULL;

  Token push(Expr *&expr, Parser &parser);
  Token pop(Expr *expr, Parser &parser);
  Token getNextToken(Parser &parser);
  Expr *getBody(Parser &parser);
};

void analyzeStatements(Expr *&body, TokenType tokenType, Parser &parser);

Expr *analyzeStatement(Expr *expr, Parser &parser) {
  ExprStack stack;
  Type type = UNKNOWN_TYPE;
  Token token = stack.push(expr, parser);

  switch(token.type) {
    case TOKEN_TYPE_LITERAL: {
        char *primitiveTypes[] = {"void", "bool", "int", "float", "String", "var"};
        Type types[] = {VOID_TYPE, BOOL_TYPE, INT_TYPE, FLOAT_TYPE, stringType, anyType};

        for (int index = 0; type.valueType == VAL_UNKNOWN && index < sizeof(primitiveTypes) / sizeof(char *); index++)
          if (!memcmp(primitiveTypes[index], token.start, strlen(primitiveTypes[index])))
            type = types[index];
      }
      break;

    case TOKEN_IDENTIFIER:
      Expr *dec = resolveReferenceExpr(token, NULL, 0);

      if (dec)
        switch(dec->type) {
          case EXPR_FUNCTION:
//            ((ReferenceExpr *) expr)->declaration = dec;
            type = OBJ_TYPE(&((FunctionExpr *) dec)->_function);
            break;

          case EXPR_DECLARATION: {
//            ((ReferenceExpr *) expr)->declaration = dec;
              Type type2 = ((DeclarationExpr *) dec)->_declaration.type;

              if (IS_FUNCTION(type2))
                type2 = type2;
            }
            break;
        }
//      type = resolveType(stack.top->expr);
      break;
  }

  if (type.valueType != VAL_UNKNOWN) {
    Token name = stack.getNextToken(parser);

    if (name.type == TOKEN_STAR) {
      name = stack.getNextToken(parser);
      // change type
      if (IS_FUNCTION(type))
        type = OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(type)));
    }

    while (name.type == TOKEN_LEFT_BRACKET) {
      name = stack.getNextToken(parser);

      if (name.type != TOKEN_RIGHT_BRACKET)
        type = OBJ_TYPE(newArray(type));
      else {
        name = stack.getNextToken(parser);
        type = OBJ_TYPE(newArray(type));
      }
    }

    if (name.type == TOKEN_IDENTIFIER) {
      token = stack.getNextToken(parser);

      switch (token.type) {
        case TOKEN_SUPER:
          expr = newDeclarationExpr(type, name, NULL);
//          ((DeclarationExpr *) expr)->declarationLevel = 1;
          checkDeclaration(((DeclarationExpr *) expr)->_declaration, name, NULL, &parser, 1);
          break;

        case TOKEN_EQUAL:
          expr = newDeclarationExpr(type, name, stack.getBody(parser));
//          ((DeclarationExpr *) expr)->declarationLevel = 1;
          checkDeclaration(((DeclarationExpr *) expr)->_declaration, name, NULL, &parser, 1);
          break;

        case TOKEN_CALL: {
            CallExpr *callExpr = (CallExpr *) stack.top->expr;
            Expr *params = callExpr->args;

            token = stack.getNextToken(parser);

            FunctionExpr *functionExpr = newFunctionExpr(type, name, 0, NULL, (GroupingExpr *) stack.getBody(parser));

            expr = functionExpr;
            checkDeclaration(functionExpr->_declaration, name, functionExpr, &parser, 1);
            pushScope(functionExpr);
            analyzeStatements(params, TOKEN_COMMA, parser);

            for (Expr *paramRef = params; paramRef; paramRef = cdr(paramRef, TOKEN_COMMA)) {
              ((DeclarationExpr *) car(paramRef, TOKEN_COMMA))->declarationLevel = 2;
              functionExpr->arity++;
            }

            functionExpr->params = params;
            popScope();
          }
          break;
      }
    }
  }

  return expr;
}

void analyzeStatements(Expr *&body, TokenType tokenType, Parser &parser) {
  Expr **statements = initAddress(body);

  while (statements) {
    Expr *&statement = *carAddress(statements, tokenType);

    statement = analyzeStatement(statement, parser);
    statements = cdrAddress(*statements, tokenType);
  }
}

ExprInfo::ExprInfo(Expr *&expr, ExprInfo *parent) : expr(expr) {
  index = 0;
  this->parent = parent;
}

Token ExprInfo::getNextToken(ExprStack *exprStack, Parser &parser) {
  return expr ? expr->getNextToken(exprStack, index++, parser) : buildToken(TOKEN_SUPER, "");
}

Token ExprStack::push(Expr *&expr, Parser &parser) {
  top = new ExprInfo(expr, top);
  return getNextToken(parser);
}

Token ExprStack::pop(Expr *expr, Parser &parser) {
  if (top)
    if (top->expr != expr) {
      delete top->expr;
      top->expr = expr;
      top->index = 0;
    }
    else {
      ExprInfo *parent = top->parent;

      delete top;
      top = parent;
    }

  return getNextToken(parser);
}

Token ExprStack::getNextToken(Parser &parser) {
  return top ? top->getNextToken(this, parser) : buildToken(TOKEN_SUPER, "");
}

Expr *ExprStack::getBody(Parser &parser) {
  getNextToken(parser);

  Expr *expr = NULL;

  while (top) {
    ExprInfo *parent = top->parent;

    expr = top->expr;
    delete top;
    top = parent;
  }

  return expr;
}

Token IteratorExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token AssignExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  switch(index) {
    case 0:
      return exprStack->push(varExp, parser);

    case 1:
      return op;

    default:
      delete varExp;
      return exprStack->pop(value, parser);
  }
}

Token UIAttributeExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token UIDirectiveExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token BinaryExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  if (index && op.type == TOKEN_ARRAY)
    index++;

  switch(index) {
    case 0:
      return exprStack->push(left, parser);

    case 1:
      return op;

    default:
      delete left;
      return exprStack->pop(right, parser);
  }
}

Token CallExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  switch(index) {
    case 0:
      return exprStack->push(callee, parser);

    case 1:
      return paren;

    default:
      delete callee;
      return exprStack->pop(this, parser);
  }
}

Token ArrayElementExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  switch(index) {
    case 0:
      return exprStack->push(callee, parser);

    case 1:
      return bracket;

    default:
      delete callee;
      return exprStack->pop(this, parser);
  }
}

Token DeclarationExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token FunctionExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token GetExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  switch(index) {
    case 0:
      return exprStack->push(object, parser);

    case 1:
      return buildToken(TOKEN_DOT, ".");

    case 2:
      return name;

    default:
      delete object;
      return exprStack->pop(NULL, parser);
  }
}

Token GroupingExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token CastExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
//  expr = expr->findTypes(parser);
  return buildToken(TOKEN_SUPER, "");
}

Token IfExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token ArrayExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  if (!index)
    return buildToken(TOKEN_LEFT_BRACKET, "[");

  if (index % 2)
    return (index >> 1) < 0 ? buildToken(TOKEN_COMMA, ",") : buildToken(TOKEN_RIGHT_BRACKET, "]");
  else
    return (index >> 1) < 0 ? exprStack->push(body, parser) : exprStack->pop(this, parser);
}

Token LiteralExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
//  return index ? exprStack->pop(this, parser) : name;
}

Token LogicalExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  switch(index) {
    case 0:
      return exprStack->push(left, parser);

    case 1:
      return op;

    default:
      delete left;
      return exprStack->pop(right, parser);
  }
}

Token WhileExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token ReturnExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token SetExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token TernaryExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token ThisExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token UnaryExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token PrimitiveExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return index ? exprStack->pop(this, parser) : name;
}

Token ReferenceExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return index ? exprStack->pop(this, parser) : name;
}

Token SwapExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

Token NativeExpr::getNextToken(ExprStack *exprStack, int index, Parser &parser) {
  return buildToken(TOKEN_SUPER, "");
}

int IteratorExpr::findTypes(Parser &parser) {
  return value->findTypes(parser);
}

int AssignExpr::findTypes(Parser &parser) {
  return varExp->findTypes(parser) + (value ? value->findTypes(parser) : 0);
}

int UIAttributeExpr::findTypes(Parser &parser) {
  parser.error("Internal error, cannot be invoked..");
  return this->findTypes(parser);
}

int UIDirectiveExpr::findTypes(Parser &parser) {
  parser.error("Internal error, cannot be invoked..");
  return 0;
}

int BinaryExpr::findTypes(Parser &parser) {
  return left->findTypes(parser) + right->findTypes(parser);
}

int CallExpr::findTypes(Parser &parser) {
  int numTypes = callee->findTypes(parser);

  for (Expr *args = this->args; args; args = cdr(args, TOKEN_COMMA))
    numTypes += car(args, TOKEN_COMMA)->findTypes(parser);

  return numTypes;
}

int ArrayElementExpr::findTypes(Parser &parser) {
  int numTypes = callee->findTypes(parser);

  for (int index = 0; index < count; index++)
    numTypes += indexes[index]->findTypes(parser);

  return numTypes;
}

int DeclarationExpr::findTypes(Parser &parser) {
  int numTypes = initExpr ? initExpr->findTypes(parser) : 0;

  declarationLevel++;
  return numTypes;
}

int FunctionExpr::findTypes(Parser &parser) {
  if (body) {
    pushScope(this);
    analyzeStatements(body->body, TOKEN_SEPARATOR, parser);
    body->findTypes(parser);
    popScope();
  }

  return 0;
}

int GetExpr::findTypes(Parser &parser) {
  pushSignature(NULL);
  int objectType = object->findTypes(parser);
  popSignature();
  return 0;
}

int GroupingExpr::findTypes(Parser &parser) {
  int numChanges = 0;

  if (body) {
    bool functionFlag = getCurrent()->getGroup() == this;

    if (!functionFlag)
      pushScope(this);

    analyzeStatements(body, TOKEN_SEPARATOR, parser);
    body->findTypes(parser);

    if (!functionFlag)
      popScope();
  }

  return numChanges;
}

int CastExpr::findTypes(Parser &parser) {
  return expr->findTypes(parser);
}

int IfExpr::findTypes(Parser &parser) {
  condition->findTypes(parser);
  thenBranch->findTypes(parser);

  if (elseBranch)
    elseBranch->findTypes(parser);

  return 0;
}

int ArrayExpr::findTypes(Parser &parser) {
  ObjArray *objArray = newArray(VOID_TYPE);
  Type type = OBJ_TYPE(objArray);/*
//  Compiler compiler;

//  compiler.pushScope(newFunction(VOID_TYPE, NULL, 0));

  objArray->elementType = body ? body->findTypes(parser) : UNKNOWN_TYPE;
  hasSuperCalls = body && body->hasSuperCalls;

//  compiler.popScope();
//  compiler.function->type = type;
//  function = compiler.function;
*/  return 0;
}

int LiteralExpr::findTypes(Parser &parser) {
  return 0;
}

int LogicalExpr::findTypes(Parser &parser) {
  return left->findTypes(parser) + right->findTypes(parser);
}

int WhileExpr::findTypes(Parser &parser) {
  int type = condition->findTypes(parser);

  body->findTypes(parser);
  return 0;
}

int ReturnExpr::findTypes(Parser &parser) {
  if (value)
    value->findTypes(parser);

  return 0;
}

int SetExpr::findTypes(Parser &parser) {
  pushSignature(NULL);
  int objectType = object->findTypes(parser);
  int valueType = value->findTypes(parser);
  popSignature();
  return 0;
}

int TernaryExpr::findTypes(Parser &parser) {
  left->findTypes(parser);

  int type = middle->findTypes(parser);

  if (right)
    right->findTypes(parser);

  return 0;
}

int ThisExpr::findTypes(Parser &parser) {
  return 0;
}

int UnaryExpr::findTypes(Parser &parser) {
  int type = right->findTypes(parser);

  return 0;
}

int PrimitiveExpr::findTypes(Parser &parser) {
  return 0;
}

int ReferenceExpr::findTypes(Parser &parser) {
  Declaration *first = getFirstDeclarationRef(getCurrent(), name, 1);

  if (!first)
    return 0;

  declaration = resolveReference(first, name, NULL/*getSignature()*/, &parser);

  if (declaration)
    switch(declaration->type) {
      case EXPR_DECLARATION: return 0;
      case EXPR_FUNCTION: return 0;
      default: parser.error("Variable '%.*s' is not a call.", name.length, name.start);
    }

  return 0;
}

int SwapExpr::findTypes(Parser &parser) {
  return 0;
}

int NativeExpr::findTypes(Parser &parser) {
  return 0;
}
