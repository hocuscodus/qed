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

Expr *IteratorExpr::findTypes(Parser &parser) {
  value = value->findTypes(parser);
  return this;
}

Expr *AssignExpr::findTypes(Parser &parser) {
  varExp = varExp->findTypes(parser);

  if (value)
    value = value->findTypes(parser);

  return this;
}

Expr *UIAttributeExpr::findTypes(Parser &parser) {
  handler = handler->findTypes(parser);
  return this;
}

Expr *UIDirectiveExpr::findTypes(Parser &parser) {
  return this;
}

Expr *BinaryExpr::findTypes(Parser &parser) {
  Type type = UNKNOWN_TYPE;
  Expr *typeExpr = NULL;
  Expr *rest = NULL;

  // check if left is a type
  switch (op.type) {
    case TOKEN_STAR:
      if (left->type == EXPR_REFERENCE) {
        type = left->resolve(parser);

        if (IS_FUNCTION(type)) {
          type = OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(type)));

          if (right->type == EXPR_ARRAY)
            ;

          rest = right;
        }
      }
      break;

    case TOKEN_ARRAY: {
        Expr *ref = NULL;

        switch (left->type) {
          case EXPR_ARRAYELEMENT: {
              ArrayElementExpr *arrayElement = (ArrayElementExpr *) left;

              if (arrayElement->callee->type == EXPR_REFERENCE)
                ;
            }
            break;

          case EXPR_REFERENCE:
            type = left->resolve(parser);
            break;

          case EXPR_PRIMITIVE:
            type = ((PrimitiveExpr *) left)->primitiveType;
            rest = right;
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
              Expr *expr = newFunctionExpr(type, name, arity, (GroupingExpr *) nameExpr, NULL);

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
/*
  if (!rest) {
    parser.error("Cannot declare an empty type");
    return this;
  }
*//*
  switch(right->type) {
    case EXPR_BINARY:
      break;

    case EXPR_REFERENCE:
    default:
      return 0;
  }
*/
  left = left->findTypes(parser);

  if (right)
    right = right->findTypes(parser);

  return this;
/*
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

                FunctionExpr *function = newFunctionExpr(anyType, callee->name, arity, body, NULL);

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
  }*/
/*
    Token nameToken = buildToken(TOKEN_IDENTIFIER, "l");
    GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), *initBody, NULL);
    FunctionExpr *wrapperFunc = newFunctionExpr(anyType, nameToken, 1, group, NULL);
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
    popScope();*/
}

Expr *CallExpr::findTypes(Parser &parser) {
  callee = callee->findTypes(parser);

  for (Expr *params = this->params; params; params = cdr(params, TOKEN_COMMA)) {
    Expr **param = carAddress(params, TOKEN_COMMA);

    *param = (*param)->findTypes(parser);
  }

  return this;
}

Expr *ArrayElementExpr::findTypes(Parser &parser) {
  callee = callee->findTypes(parser);

  for (int index = 0; index < count; index++)
    indexes[index] = indexes[index]->findTypes(parser);

  return this;
}

Expr *DeclarationExpr::findTypes(Parser &parser) {
  if (initExpr)
    initExpr = initExpr->findTypes(parser);

  return this;
}

Expr *FunctionExpr::findTypes(Parser &parser) {
  if (body)
    body = (GroupingExpr *) body->findTypes(parser);

  return this;
}

Expr *GetExpr::findTypes(Parser &parser) {
  object = object->findTypes(parser);
  return this;
}

Expr *GroupingExpr::findTypes(Parser &parser) {
  if (body) {
    pushScope(this);
    body = body->findTypes(parser);
    popScope();
  }

  return this;
}

Expr *CastExpr::findTypes(Parser &parser) {
  expr = expr->findTypes(parser);
  return this;
}

Expr *IfExpr::findTypes(Parser &parser) {
  condition = condition->findTypes(parser);
  thenBranch = thenBranch->findTypes(parser);

  if (elseBranch)
    elseBranch = elseBranch->findTypes(parser);

  return this;
}

Expr *ArrayExpr::findTypes(Parser &parser) {
  if (body)
    body = body->findTypes(parser);

  return this;
}

Expr *LiteralExpr::findTypes(Parser &parser) {
  return this;
}

Expr *LogicalExpr::findTypes(Parser &parser) {
  left = left->findTypes(parser);
  right = right->findTypes(parser);
  return this;
}

Expr *WhileExpr::findTypes(Parser &parser) {
  condition = condition->findTypes(parser);
  body = body->findTypes(parser);
  return this;
}

Expr *ReturnExpr::findTypes(Parser &parser) {
  if (value)
    value = value->findTypes(parser);

  return this;
}

Expr *SetExpr::findTypes(Parser &parser) {
  object = object->findTypes(parser);
  value = value->findTypes(parser);
  return this;
}

Expr *TernaryExpr::findTypes(Parser &parser) {
  middle = middle->findTypes(parser);
  left = left->findTypes(parser);
  right = right->findTypes(parser);
  return this;
}

Expr *ThisExpr::findTypes(Parser &parser) {
  return this;
}

Expr *UnaryExpr::findTypes(Parser &parser) {
  right = right->findTypes(parser);
  return this;
}

Expr *PrimitiveExpr::findTypes(Parser &parser) {
  return this;
}

Expr *ReferenceExpr::findTypes(Parser &parser) {
//  declaration = resolveReference(first, name, getSignature(), &parser);
  return this;
}

Expr *SwapExpr::findTypes(Parser &parser) {
  return this;
}

Expr *NativeExpr::findTypes(Parser &parser) {
  return this;
}
