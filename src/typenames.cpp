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

// typeExpr $$ nameExpr -> Obj objFn; int a;
// typeExpr * nameExpr -> Obj *obj;
// typeExpr[]+ $$ nameExpr -> Obj[] obj;
// (typeExpr * []+) $$ nameExpr -> Obj*[] obj;
// (typeExpr $$ nameExpr) = (initExpr) -> Obj objFn = fn; int a = 1;
// (typeExpr * nameExpr) = (initExpr) -> Obj *obj = getObj();
// (typeExpr[]+ $$ nameExpr) = (initExpr) -> Obj[] obj = [objFn1, objFn2];
// ((typeExpr * []+) $$ nameExpr) = (initExpr) -> Obj*[] obj = 3 new Obj();
// typeExpr $$ (nameExpr(...) $$ (body)) -> Obj fn(int a) {return fn}
// (typeExpr * nameExpr(...)) $$ (body) -> Obj *fn(int a) {return objs[a]}
// typeExpr[]+ $$ (nameExpr(...) $$ (body)) -> Obj[] fn(int a) {return objFnArray}
// (typeExpr * []+) $$ (nameExpr(...) $$ (body)) -> Obj*[] fn(int a) {return objs[a]}
//
// typeExpr['*']['[]']* nameExpr [ = initExpr|(params) body]
// dec = identifier['*']['[]']* identifier [ = initExpr|([dec, ]*) initExpr]
// findDecs(initExpr)
//
// generate declaration and init expr
/*
bool analyzeType(Declaration &declaration, Expr *expr, Parser &parser) {
  declaration.type = UNKNOWN_TYPE;

  if (expr->type == EXPR_BINARY) {
    BinaryExpr *binaryExpr = (BinaryExpr *) expr;

    switch (binaryExpr->op.type) {
      case TOKEN_STAR:
        if (binaryExpr->left->type == EXPR_REFERENCE) {
          Type type = binaryExpr->left->resolve(parser);

          if (IS_FUNCTION(type)) {
            declaration.type = OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(type)));

            if (binaryExpr->right->type == EXPR_ARRAY)
              ;

//            rest = binaryExpr->right;
            return true;
          }
        }
        break;

      case TOKEN_ARRAY: {
          Expr *ref = NULL;

          switch (binaryExpr->left->type) {
            case EXPR_ARRAYELEMENT: {
                ArrayElementExpr *arrayElement = (ArrayElementExpr *) binaryExpr->left;

                if (arrayElement->callee->type == EXPR_REFERENCE)
                  ;
              }
              break;

            case EXPR_REFERENCE:
              return binaryExpr->left->resolve(parser);
              break;

            case EXPR_PRIMITIVE:
              return ((PrimitiveExpr *) binaryExpr->left)->primitiveType;
//              rest = binaryExpr->right;
              break;
          }
        }
        break;
    }
  }

  return false;
}

Expr *analyzeBinaryStatement(BinaryExpr *expr, Parser &parser) {
  Type type = UNKNOWN_TYPE;
  Expr *typeExpr = NULL;
  Expr *rest = NULL;

  // check if left is a type
  switch (expr->op.type) {
    case TOKEN_STAR:
      if (expr->left->type == EXPR_REFERENCE) {
        type = expr->left->resolve(parser);

        if (IS_FUNCTION(type)) {
          type = OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(type)));

          if (expr->right->type == EXPR_ARRAY)
            ;

          rest = expr->right;
        }
      }
      break;

    case TOKEN_ARRAY: {
        Expr *ref = NULL;

        switch (expr->left->type) {
          case EXPR_ARRAYELEMENT: {
              ArrayElementExpr *arrayElement = (ArrayElementExpr *) expr->left;

              if (arrayElement->callee->type == EXPR_REFERENCE)
                ;
            }
            break;

          case EXPR_REFERENCE:
            type = expr->left->resolve(parser);
            break;

          case EXPR_PRIMITIVE:
            type = ((PrimitiveExpr *) expr->left)->primitiveType;
            rest = expr->right;
            break;
        }
      }
      break;
  }

  if (rest) {
    Expr *nameExpr = car(rest, TOKEN_ARRAY);
    Expr *nameRef = NULL;

    switch (nameExpr->type) {
      case EXPR_ASSIGN:
        nameRef = ((AssignExpr *) nameExpr)->varExp;
        break;

      case EXPR_REFERENCE:
        nameRef = (ReferenceExpr *) nameExpr;
        break;

      case EXPR_CALL: {
          CallExpr *callExpr = (CallExpr *) nameExpr;

          nameRef = callExpr->callee;

          if (nameRef->type == EXPR_REFERENCE) {
            int arity = 0;
            Token name = ((ReferenceExpr *) nameRef)->name;

            for (Expr *params = callExpr->params; params; params = cdr(params, TOKEN_COMMA)) {
              Expr **param = carAddress(params, TOKEN_COMMA);

              *param = (*param)->findTypes(parser);
              arity++;
            }

            rest = cdr(rest, TOKEN_ARRAY);
            nameExpr = car(rest, TOKEN_ARRAY);

            if (nameExpr->type == EXPR_GROUPING) {
              Expr *expr = newFunctionExpr(type, name, arity, (GroupingExpr *) nameExpr);

              expr->findTypes(parser);
              return expr;
            }
          }
        }
        break;
    }

    if (!nameRef)
      parser.error("A declaration name must follow the type");
  }
//  else
//    return left->findTypes(parser) + (right ? right->findTypes(parser) : 0);
/ *
  if (!rest) {
    parser.error("Cannot declare an empty type");
    return this;
  }
* // *
  switch(right->type) {
    case EXPR_BINARY:
      break;

    case EXPR_REFERENCE:
    default:
      return 0;
  }
* /
  expr->left = expr->left->findTypes(parser);

  if (expr->right)
    expr->right = expr->right->findTypes(parser);

  return expr;
/ *
  // check for declaration
  if (arrayExpr->op.type == TOKEN_ARRAY) {
    int numTypes = 0;
    Expr *statement = left;

    if (statement->type == EXPR_PRIMITIVE) {
      // we have a first level type
      body = cdr(body, TOKEN_SEPARATOR);
      statement = car(body, TOKEN_SEPARATOR);

      switch (statement->type) {
        case EXPR_REFERENCE:
          statement = NULL;
          break;

        case EXPR_ASSIGN:
          statement = NULL;
          break;

        case EXPR_CALL: {
            CallExpr *callExpr = (CallExpr *) statement;

            if (callExpr->callee->type == EXPR_REFERENCE) {
              body = cdr(body, TOKEN_SEPARATOR);
              statement = car(body, TOKEN_SEPARATOR);

              if (statement->type == EXPR_GROUPING) {
                int arity = 0;
                ReferenceExpr *callee = (ReferenceExpr *) callExpr->callee;
                GroupingExpr *body = (GroupingExpr *) statement;

                for (Expr *paramRef = callExpr->params; paramRef; paramRef = cdr(paramRef, TOKEN_COMMA))
                  arity++;

                FunctionExpr *function = newFunctionExpr(anyType, callee->name, arity, body);

//                checkDeclaration(function->_declaration, function->_declaration.name, NULL, &parser);
              }
            }
            break;
          }

        default:
          statement = NULL;
          break;
      }
        
//      checkDeclaration()
    }

//    numTypes = left ? left->findTypes(parser) : 0;
  }* /
/ *
    Token nameToken = buildToken(TOKEN_IDENTIFIER, "l");
    GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), *initBody, NULL);
    FunctionExpr *wrapperFunc = newFunctionExpr(anyType, nameToken, 1, group);
    GroupingExpr *initExpr = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), wrapperFunc, NULL);

    name = buildToken(TOKEN_LEFT_PAREN, "(");
    pushScope(initExpr);
    checkDeclaration(wrapperFunc->_declaration, nameToken, wrapperFunc, NULL);
    pushScope(wrapperFunc);

    for (Expr **bodyElement = initBody; bodyElement; bodyElement = next) {
      Expr *expr = car(*bodyElement, TOKEN_SEPARATOR);

      next = cdrAddress(*bodyElement, TOKEN_SEPARATOR);

      if (next) {
        Declaration *dec = getDeclaration(expr);

        checkDeclaration(*dec, dec->name, NULL, NULL);
        expr->resolve(parser);
      }
      else {
        *initBody = initExpr;
        type = OBJ_TYPE(newArray(expr->resolve(parser)));

        Type elementType = AS_ARRAY_TYPE(type)->elementType;

        if (!IS_VOID(elementType)) {
          Token arrayName = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "SQEDArray" : "qedArray");
          ReferenceExpr *qedArrayExpr = new ReferenceExpr(arrayName, NULL);

          wrapperFunc->_declaration.name = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "L" : "l");

          if (IS_INSTANCE(elementType) && ((ObjFunction *) AS_INSTANCE_TYPE(elementType)->callable)->expr->ui) {
            GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);

            // Perform the array UI AST magic
            ss->str("");
            (*ss) << "void UI_(Ball *[] array, int[] dims) {\n";
            (*ss) << "  for(let index = 0; index < dims[0]; index++)\n";
            (*ss) << "    array[index].ui_ = new array[index].UI_()\n";
//            (*ss) << "    array[index].setUI()\n";
            (*ss) << "\n";
            (*ss) << "  void Layout_() {\n";
            (*ss) << "    Layout_*[] layouts;\n";
            (*ss) << "    for(let index = 0; index < dims[0]; index++)\n";
            (*ss) << "      layouts[index] = new array[index].ui_.Layout_()\n";
            (*ss) << "\n";
            (*ss) << "    void paint(int pos0, int pos1, int size0, int size1) {\n";
            (*ss) << "      for(let index = 0; index < dims[0]; index++)\n";
            (*ss) << "        layouts[index].paint(pos0, pos1, size0, size1)\n";
            (*ss) << "    }\n";
            (*ss) << "  }\n";
            (*ss) << "}\n";
            fprintf(stderr, ss->str().c_str());
            parse(group, ss->str().c_str());
            group->resolve(parser);
            lastExpr = addExpr(lastExpr, group, buildToken(TOKEN_COMMA, ","));
          }
          else
            lastExpr = addExpr(lastExpr, new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "Qui_"), NULL), buildToken(TOKEN_COMMA, ","));

          *lastExpr = new CallExpr(false, qedArrayExpr, buildToken(TOKEN_LEFT_PAREN, "("), *lastExpr, NULL);
          (*lastExpr)->resolve(parser);
          (*lastExpr)->hasSuperCalls = true;
          body->hasSuperCalls = true;
          *bodyElement = new ReturnExpr(buildToken(TOKEN_RETURN, "return"), expr->hasSuperCalls, expr);
        }
        else {
          Token arrayName = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "VSQEDArray" : "vqedArray");
          ReferenceExpr *qedArrayExpr = new ReferenceExpr(arrayName, NULL);

          wrapperFunc->_declaration.name = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "L" : "l");
          *lastExpr = new CallExpr(false, qedArrayExpr, buildToken(TOKEN_LEFT_PAREN, "("), *lastExpr, NULL);
          (*lastExpr)->resolve(parser);
          (*lastExpr)->hasSuperCalls = true;
          body->hasSuperCalls = true;
          *bodyElement = new ReturnExpr(buildToken(TOKEN_RETURN, "return"), expr->hasSuperCalls, expr);
        }
      }
    }

    popScope();
    popScope();* /
}
*/
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
/*
Expr *Parser::primitiveType() {
  switch (previous.start[0]) {
  case 'v':
    switch (previous.start[1]) {
    case 'a':
      return new PrimitiveExpr(previous, anyType);
    case 'o':
      return new PrimitiveExpr(previous, VOID_TYPE);
    }
  case 'b': return new PrimitiveExpr(previous, BOOL_TYPE);
  case 'i': return new PrimitiveExpr(previous, INT_TYPE);
  case 'f': return new PrimitiveExpr(previous, FLOAT_TYPE);
  case 'S': return new PrimitiveExpr(previous, stringType);
  default: return NULL; // Unreachable.
  }
}
*/
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
      Expr *dec = resolveReferenceExpr(token, NULL);

      if (dec)
        switch(dec->type) {
          case EXPR_FUNCTION:
//            ((ReferenceExpr *) expr)->declaration = dec;
            type = OBJ_TYPE(&((FunctionExpr *) dec)->_function);
            break;

          case EXPR_DECLARATION:
//            ((ReferenceExpr *) expr)->declaration = dec;
//            type = OBJ_TYPE(((ObjFunction *) ((DeclarationExpr *) dec)->_declaration.type.objType));
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
          checkDeclaration(((DeclarationExpr *) expr)->_declaration, name, NULL, &parser);
          break;

        case TOKEN_EQUAL:
          expr = newDeclarationExpr(type, name, stack.getBody(parser));
          checkDeclaration(((DeclarationExpr *) expr)->_declaration, name, NULL, &parser);
          break;

        case TOKEN_CALL: {
            CallExpr *callExpr = (CallExpr *) stack.top->expr;
            Expr *params = callExpr->args;

            token = stack.getNextToken(parser);

            FunctionExpr *functionExpr = newFunctionExpr(type, name, 0, NULL, (GroupingExpr *) stack.getBody(parser));

            expr = functionExpr;
            checkDeclaration(functionExpr->_declaration, name, functionExpr, &parser);
            pushScope(functionExpr);
            analyzeStatements(params, TOKEN_COMMA, parser);

            for (Expr *paramRef = params; paramRef; paramRef = cdr(paramRef, TOKEN_COMMA))
              functionExpr->arity++;

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
    expr = top->expr;
    delete top;
    top = top->parent;
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
  int type1 = left->findTypes(parser);

//  if (IS_FUNCTION(type1))
//    return 0;

  int type2 = right->findTypes(parser);/*
  Type type = type1;
  bool boolVal = false;

  hasSuperCalls = left->hasSuperCalls || right->hasSuperCalls;

  switch (op.type) {
  case TOKEN_PLUS:
    if (IS_OBJ(type1)) {
//      right = convertToString(right, type2, parser);
      return 0;
    }
    // no break statement, fall through

  case TOKEN_BANG_EQUAL:
  case TOKEN_EQUAL_EQUAL:
  case TOKEN_GREATER:
  case TOKEN_GREATER_EQUAL:
  case TOKEN_LESS:
  case TOKEN_LESS_EQUAL:
    boolVal = op.type != TOKEN_PLUS;
    if (IS_OBJ(type1)) {
      if (!IS_OBJ(type2))
        parser.error("Second operand must be a string");

      return 0;
    }
    // no break statement, fall through

  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH: {
    switch (type1.valueType) {
    case VAL_INT:
      if (!type1.equals(type2)) {
        if (!IS_FLOAT(type2))
          if (IS_ANY(type2)) {
//            right = convertToInt(right, type2, parser);
            return 0;
          }
          else {
            parser.error("Second operand must be numeric");
            return 0;
          }

//        left = convertToFloat(left, type1, parser);
        return 0;
      }
      else
        return 0;

    case VAL_FLOAT:
      if (!type1.equals(type2)) {
        if (!IS_INT(type2) && !IS_ANY(type2)) {
          parser.error("Second operand must be numeric");
          return 0;
        }

//        right = convertToFloat(right, type2, parser);
      }

      return 0;

    case VAL_UNKNOWN:
      return 0;

    default:
      parser.error("First operand must be numeric");
      return 0;
    }
  }
  break;

  case TOKEN_XOR:
    return 0;

  case TOKEN_OR:
  case TOKEN_AND:
  case TOKEN_GREATER_GREATER_GREATER:
  case TOKEN_GREATER_GREATER:
  case TOKEN_LESS_LESS:
  case TOKEN_PERCENT:
    return 0;

  case TOKEN_OR_OR:
  case TOKEN_AND_AND:
    return 0;

  case TOKEN_COMMA:
    return 0;

  case TOKEN_SEPARATOR:
    return 0;

  default:
    return 0; // Unreachable.
  }*/
  return 0;
}

static Token tok = buildToken(TOKEN_IDENTIFIER, "Capital");
int CallExpr::findTypes(Parser &parser) {
  Signature signature;

  for (Expr *args = this->args; args; args = cdr(args, TOKEN_COMMA)) {
    int type = car(args, TOKEN_COMMA)->findTypes(parser);

//    signature.push_back(type);
  }

  pushSignature(&signature);
  int type = callee->findTypes(parser);
  popSignature();
/*
  if (IS_FUNCTION(type)) {
    ObjFunction *callable = AS_FUNCTION_TYPE(type);

    hasSuperCalls = !newFlag && callable->expr->_declaration.name.isUserClass();
    _declaration = &callable->expr->_declaration;
    return 0;
  } else {
    parser.error("Non-callable object type");
    return 0;
  }*/
  return 0;
}

int ArrayElementExpr::findTypes(Parser &parser) {
  int numTypes = callee->findTypes(parser);

  for (int index = 0; index < count; index++)
    numTypes += indexes[index]->findTypes(parser);

  return numTypes;
}

int DeclarationExpr::findTypes(Parser &parser) {
  if (initExpr) {
    int type1 = initExpr->findTypes(parser);
  }
/*
    hasSuperCalls = initExpr->hasSuperCalls;

    if (IS_ANY(_declaration.type))
      _declaration.type = type1;
    else if (!type1.equals(_declaration.type)) {
//      initExpr = convertToType(_declaration.type, initExpr, type1, parser);

      if (!initExpr) {
        parser.error("Value must match the variable type");
      }
    }
  }
  else
    switch (_declaration.type.valueType) {
    case VAL_VOID:
      parser.error("Cannot declare a void variable.");
      break;

    case VAL_INT:
      initExpr = new LiteralExpr(VAL_INT, {.integer = 0});
      break;

    case VAL_BOOL:
      initExpr = new LiteralExpr(VAL_BOOL, {.boolean = false});
      break;

    case VAL_FLOAT:
      initExpr = new LiteralExpr(VAL_FLOAT, {.floating = 0.0});
      break;

    case VAL_OBJ:
      switch (AS_OBJ_TYPE(_declaration.type)) {
      case OBJ_STRING:
        initExpr = new LiteralExpr(VAL_OBJ, {.obj = &copyString("", 0)->obj});
        break;
      }
      break;
    }
*/
  return 0;
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
/*
  hasSuperCalls = object->hasSuperCalls;

  switch (AS_OBJ_TYPE(objectType)) {
    case OBJ_FUNCTION: {
        FunctionExpr *function = AS_FUNCTION_TYPE(objectType)->expr;
        Scope scope(function, function->body, NULL);
        Declaration *dec = getDeclarationRef(name, scope.group->declarations);
        Expr *refExpr = dec ? resolveReference(dec, name, getSignature(), &parser) : NULL;

        if (refExpr) {
          _declaration = getDeclaration(refExpr);
          return 0;
        }

        parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
      }
      break;

    case OBJ_INSTANCE: {
        ObjInstance *type = AS_INSTANCE_TYPE(objectType);
        FunctionExpr *function = ((ObjFunction *) type->callable)->expr;
        Scope scope(function, function->body, NULL);
        Declaration *dec = getDeclarationRef(name, scope.group->declarations);
        Expr *refExpr = dec ? resolveReference(dec, name, getSignature(), &parser) : NULL;

        if (refExpr) {
          _declaration = getDeclaration(refExpr);
          return 0;
        }

        parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
      }
      break;

    case OBJ_ARRAY: {
        ObjArray *type = AS_ARRAY_TYPE(objectType);
        Type elementType = type->elementType;
        FunctionExpr *function = (FunctionExpr *) arrayDeclaration->expr;
        Scope scope(function, function->body, NULL);
        Declaration *dec = getDeclarationRef(name, scope.group->declarations);
        Expr *refExpr = dec ? resolveReference(dec, name, getSignature(), &parser) : NULL;

        if (refExpr) {
          _declaration = getDeclaration(refExpr);
          return 0;
        }

        parser.errorAt(&name, "Method '%.*s' not found.", name.length, name.start);
      }
      break;

    default:
      parser.errorAt(&name, "Only instances have properties.");
      break;
  }
*/
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
  int exprType = expr->findTypes(parser);

//  hasSuperCalls = expr->hasSuperCalls;
  return 0;
}

int IfExpr::findTypes(Parser &parser) {/*
  if (IS_VOID(condition->findTypes(parser)))
    parser.error("Value must not be void");

  thenBranch->findTypes(parser);
  hasSuperCalls = condition->hasSuperCalls || thenBranch->hasSuperCalls;

  if (elseBranch) {
    elseBranch->findTypes(parser);
    hasSuperCalls |= elseBranch->hasSuperCalls;
  }
*/
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
  if (type == VAL_OBJ)
    return 0;
  else
    return 0;
}

int LogicalExpr::findTypes(Parser &parser) {
  int type1 = left->findTypes(parser);
  int type2 = right->findTypes(parser);

//  hasSuperCalls = left->hasSuperCalls || right->hasSuperCalls;

//  if (!IS_BOOL(type1) || !IS_BOOL(type2))
    parser.error("Value must be boolean");

  return 0;
}

int WhileExpr::findTypes(Parser &parser) {
  int type = condition->findTypes(parser);

  body->findTypes(parser);
//  hasSuperCalls = condition->hasSuperCalls || body->hasSuperCalls;
  return 0;
}

int ReturnExpr::findTypes(Parser &parser) {
  // sync processing below

  if (value) {
    value->findTypes(parser);
//    hasSuperCalls = value->hasSuperCalls;

//  if (!getCurrent()->isClass())
//    Type type = removeDeclaration();
    // verify that type is the function return type if not an instance
    // else return void
  }

  return 0;
}

int SetExpr::findTypes(Parser &parser) {
  pushSignature(NULL);
  int objectType = object->findTypes(parser);
  int valueType = value->findTypes(parser);
  popSignature();

//  hasSuperCalls = object->hasSuperCalls || value->hasSuperCalls;
/*
  if (AS_OBJ_TYPE(objectType) != OBJ_INSTANCE)
    parser.errorAt(&name, "Only instances have properties.");
  else {
    ObjInstance *type = AS_INSTANCE_TYPE(objectType);
    FunctionExpr *function = ((ObjFunction *) type->callable)->expr;
    Scope scope(function, function->body, NULL);
    Declaration *dec = getDeclarationRef(name, scope.group->declarations);
    Expr *refExpr = dec ? resolveReference(dec, name, getSignature(), &parser) : NULL;

    if (refExpr) {
      _declaration = getDeclaration(refExpr);
      return 0;
    }

    parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
  }
*/
  return 0;
}

int TernaryExpr::findTypes(Parser &parser) {
  left->findTypes(parser);
//  if (IS_VOID(left->findTypes(parser)))
//    parser.error("Value must not be void");

  int type = middle->findTypes(parser);

//  hasSuperCalls = left->hasSuperCalls || middle->hasSuperCalls;

  if (right) {
    right->findTypes(parser);
//    hasSuperCalls |= right->hasSuperCalls;
  }

  return 0;
}

int ThisExpr::findTypes(Parser &parser) {
  return 0;
}

int UnaryExpr::findTypes(Parser &parser) {
  int type = right->findTypes(parser);

//  hasSuperCalls = right->hasSuperCalls;
/*
  if (IS_VOID(type))
    parser.error("Value must not be void");

  switch (op.type) {
  case TOKEN_NEW: {
    bool errorFlag = true;

    switch (AS_OBJ_TYPE(type)) {
    case OBJ_FUNCTION:
      errorFlag = false;
      break;
    }

    if (errorFlag)
      parser.error("Operator new must target a callable function");

    return 0;
  }

  case TOKEN_PERCENT:
//    right = convertToFloat(right, type, parser);
    return 0;

  default:
    return 0;
  }*/
  return 0;
}

int PrimitiveExpr::findTypes(Parser &parser) {
  return 0;
}

int ReferenceExpr::findTypes(Parser &parser) {
  Declaration *first = getFirstDeclarationRef(getCurrent(), name);

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
  Type type;/* = uiTypes.front();

  _expr = uiExprs.front();
  uiExprs.pop_front();
  uiTypes.pop_front();
*/
  if (type.valueType == VAL_UNKNOWN)
    return 0;

  return 0;
}

int NativeExpr::findTypes(Parser &parser) {
  return 0;
}
