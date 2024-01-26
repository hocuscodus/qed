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

Type IteratorExpr::findTypes(Parser &parser) {
  Type type = value->findTypes(parser);

  hasSuperCalls = value->hasSuperCalls;
  return type;
}

Type AssignExpr::findTypes(Parser &parser) {
  Type type1 = varExp->findTypes(parser);
  Type type2 = value ? value->findTypes(parser) : type1;

  hasSuperCalls = varExp->hasSuperCalls || (value && value->hasSuperCalls);

  if (IS_VOID(type1))
    parser.error("Variable not found");
  else if (!type1.equals(type2)) {
//    value = convertToType(type1, value, type2, parser);

    if (!value) {
      parser.error("Value must match the variable type");
    }
  }

  return type1;
}

Type UIAttributeExpr::findTypes(Parser &parser) {
  parser.error("Internal error, cannot be invoked..");
  return VOID_TYPE;
}

Type UIDirectiveExpr::findTypes(Parser &parser) {
  parser.error("Internal error, cannot be invoked..");
  return VOID_TYPE;
}

Type BinaryExpr::findTypes(Parser &parser) {
  Type type1 = left->findTypes(parser);

//  if (IS_FUNCTION(type1))
//    return OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(type1)));

  Type type2 = right->findTypes(parser);
  Type type = type1;
  bool boolVal = false;

  hasSuperCalls = left->hasSuperCalls || right->hasSuperCalls;

  switch (op.type) {
  case TOKEN_PLUS:
    if (IS_OBJ(type1)) {
//      right = convertToString(right, type2, parser);
      return stringType;
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

      return BOOL_TYPE;
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
            return {boolVal ? VAL_BOOL : VAL_INT};
          }
          else {
            parser.error("Second operand must be numeric");
            return VOID_TYPE;
          }

//        left = convertToFloat(left, type1, parser);
        return {boolVal ? VAL_BOOL : VAL_FLOAT};
      }
      else
        return {boolVal ? VAL_BOOL : VAL_INT};

    case VAL_FLOAT:
      if (!type1.equals(type2)) {
        if (!IS_INT(type2) && !IS_ANY(type2)) {
          parser.error("Second operand must be numeric");
          return VOID_TYPE;
        }

//        right = convertToFloat(right, type2, parser);
      }

      return {boolVal ? VAL_BOOL : VAL_FLOAT};

    case VAL_UNKNOWN:
      return type2;

    default:
      parser.error("First operand must be numeric");
      return type1;
    }
  }
  break;

  case TOKEN_XOR:
    return {IS_BOOL(type1) ? VAL_BOOL : VAL_FLOAT};

  case TOKEN_OR:
  case TOKEN_AND:
  case TOKEN_GREATER_GREATER_GREATER:
  case TOKEN_GREATER_GREATER:
  case TOKEN_LESS_LESS:
  case TOKEN_PERCENT:
    return INT_TYPE;

  case TOKEN_OR_OR:
  case TOKEN_AND_AND:
    return BOOL_TYPE;

  case TOKEN_COMMA:
    return type2;

  case TOKEN_SEPARATOR:
    return type2;

  default:
    return VOID_TYPE; // Unreachable.
  }
}

static Token tok = buildToken(TOKEN_IDENTIFIER, "Capital");
Type CallExpr::findTypes(Parser &parser) {
  Signature signature;

  for (Expr *params = this->params; params; params = cdr(params, TOKEN_COMMA)) {
    Type type = car(params, TOKEN_COMMA)->findTypes(parser);

    signature.push_back(type);
  }

  pushSignature(&signature);
  Type type = callee->findTypes(parser);
  popSignature();

  if (IS_FUNCTION(type)) {
    ObjFunction *callable = AS_FUNCTION_TYPE(type);

    hasSuperCalls = !newFlag && callable->expr->_declaration.name.isUserClass();
    _declaration = &callable->expr->_declaration;
    return newFlag ? OBJ_TYPE(newInstance(callable)) : callable->expr->_declaration.type;
  } else {
    parser.error("Non-callable object type");
    return VOID_TYPE;
  }
}

Type ArrayElementExpr::findTypes(Parser &parser) {
  Type type = resolveType(callee);

  hasSuperCalls = callee->hasSuperCalls;

  if (!count)/*
    if (isType(type))
      return OBJ_TYPE(newArray(type));
    else*/ {
      parser.error("No index defined.");
      return VOID_TYPE;
    }
  else/*
    if (isType(type)) {
      parser.error("A type cannot have an index.");
      return VOID_TYPE;
    }
    else*/
      switch (AS_OBJ_TYPE(type)) {
      case OBJ_ARRAY: {
        ObjArray *array = AS_ARRAY_TYPE(type);

        for (int index = 0; index < count; index++) {
          indexes[index]->findTypes(parser);
          hasSuperCalls |= indexes[index]->hasSuperCalls;
        }

        return array->elementType;
      }
      case OBJ_STRING: {/*
        ObjString *string = (ObjString *)type.objType;

        if (count != string->arity)
          parser.error("Expected %d arguments but got %d.", string->arity, count);

        getCurrent()->addDeclaration(string->type.valueType);

        for (int index = 0; index < count; index++) {
          indexes[index]->findTypes(parser);
          Type argType = removeDeclaration();

          argType = argType;
        }*/
        return VOID_TYPE;
      }
      default:
        parser.error("Non-indexable object type");
        return VOID_TYPE;
      }
}

Type DeclarationExpr::findTypes(Parser &parser) {
  if (initExpr) {
    Type type1 = initExpr->findTypes(parser);

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

  return VOID_TYPE;
}

Type FunctionExpr::findTypes(Parser &parser) {
  pushScope(this);

  if (body) {
    for (Declaration *declaration = body->declarations; declaration; declaration = declaration->next)
      if (declaration->expr->type == EXPR_FUNCTION)
        declaration->expr->findTypes(parser);

    Expr *group = getStatement(body, arity + (_declaration.name.isUserClass() ? 1 : 0));

    if (group) {
      group->findTypes(parser);
      body->hasSuperCalls |= _declaration.name.isClass();
/*
      for (int index = 0; index < arity; index++)
        getStatement(body, index)->hasSuperCalls = hasSuperCalls;

      if (_declaration.name.isUserClass())
        getStatement(body, arity)->hasSuperCalls = hasSuperCalls;*/
    }
  }

  popScope();

  if (getCurrent())
    getCurrent()->add(&_declaration);

  return OBJ_TYPE(&_function);
}

Type GetExpr::findTypes(Parser &parser) {
  pushSignature(NULL);
  Type objectType = object->findTypes(parser);
  popSignature();

  hasSuperCalls = object->hasSuperCalls;

  switch (AS_OBJ_TYPE(objectType)) {
    case OBJ_FUNCTION: {
        FunctionExpr *function = AS_FUNCTION_TYPE(objectType)->expr;
        Scope scope(function, function->body, NULL);
        Declaration *dec = getDeclarationRef(name, scope.group->declarations);
        Expr *refExpr = dec ? resolveReference(dec, name, getSignature(), &parser) : NULL;

        if (refExpr) {
          _declaration = getDeclaration(refExpr);
          return getDeclarationType(refExpr);
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
          return getDeclarationType(refExpr);
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
          return getDeclarationType(refExpr);
        }

        parser.errorAt(&name, "Method '%.*s' not found.", name.length, name.start);
      }
      break;

    default:
      parser.errorAt(&name, "Only instances have properties.");
      break;
  }

  return VOID_TYPE;
}

Type GroupingExpr::findTypes(Parser &parser) {
  Type type;

  pushScope(this);

  if (name.type == TOKEN_LEFT_BRACKET) {
    Expr **next;
    Expr **lastExpr = getLastBodyExpr(&body, TOKEN_SEPARATOR);
    Expr **initBody = getLastBodyExpr(lastExpr, TOKEN_COMMA);
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
        expr->findTypes(parser);
      }
      else {
        *initBody = initExpr;
        type = OBJ_TYPE(newArray(expr->findTypes(parser)));

        Type elementType = AS_ARRAY_TYPE(type)->elementType;

        if (!IS_VOID(elementType)) {
          Token arrayName = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "SQEDArray" : "qedArray");
          ReferenceExpr *qedArrayExpr = new ReferenceExpr(arrayName, NULL);

          wrapperFunc->_declaration.name = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "L" : "l");

          if (IS_INSTANCE(elementType) && ((ObjFunction *) AS_INSTANCE_TYPE(elementType)->callable)->expr->ui) {
            GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);
            lastExpr = addExpr(lastExpr, group, buildToken(TOKEN_COMMA, ","));
          }
          else
            lastExpr = addExpr(lastExpr, new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "Qui_"), NULL), buildToken(TOKEN_COMMA, ","));

          *lastExpr = new CallExpr(false, qedArrayExpr, buildToken(TOKEN_LEFT_PAREN, "("), *lastExpr, NULL);
          (*lastExpr)->findTypes(parser);
          (*lastExpr)->hasSuperCalls = true;
          body->hasSuperCalls = true;
          *bodyElement = new ReturnExpr(buildToken(TOKEN_RETURN, "return"), expr->hasSuperCalls, expr);
        }
        else {
          Token arrayName = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "VSQEDArray" : "vqedArray");
          ReferenceExpr *qedArrayExpr = new ReferenceExpr(arrayName, NULL);

          wrapperFunc->_declaration.name = buildToken(TOKEN_IDENTIFIER, expr->hasSuperCalls ? "L" : "l");
          *lastExpr = new CallExpr(false, qedArrayExpr, buildToken(TOKEN_LEFT_PAREN, "("), *lastExpr, NULL);
          (*lastExpr)->findTypes(parser);
          (*lastExpr)->hasSuperCalls = true;
          body->hasSuperCalls = true;
          *bodyElement = new ReturnExpr(buildToken(TOKEN_RETURN, "return"), expr->hasSuperCalls, expr);
        }
      }
    }

    popScope();
    popScope();
  }
  else {
    Type bodyType = body ? body->findTypes(parser) : VOID_TYPE;

    type = name.type != TOKEN_LEFT_BRACE ? bodyType : VOID_TYPE;
  }

  hasSuperCalls = body && body->hasSuperCalls;
  popScope();
  return type;
}

Type CastExpr::findTypes(Parser &parser) {
  Type exprType = expr->findTypes(parser);

  hasSuperCalls = expr->hasSuperCalls;
  return /*isExplicitConvert(exprType, type) ? type : */UNKNOWN_TYPE;
}

Type IfExpr::findTypes(Parser &parser) {
  if (IS_VOID(condition->findTypes(parser)))
    parser.error("Value must not be void");

  thenBranch->findTypes(parser);
  hasSuperCalls = condition->hasSuperCalls || thenBranch->hasSuperCalls;

  if (elseBranch) {
    elseBranch->findTypes(parser);
    hasSuperCalls |= elseBranch->hasSuperCalls;
  }

  return VOID_TYPE;
}

Type ArrayExpr::findTypes(Parser &parser) {
  ObjArray *objArray = newArray(VOID_TYPE);
  Type type = OBJ_TYPE(objArray);
//  Compiler compiler;

//  compiler.pushScope(newFunction(VOID_TYPE, NULL, 0));

  objArray->elementType = body ? body->findTypes(parser) : UNKNOWN_TYPE;
  hasSuperCalls = body && body->hasSuperCalls;

//  compiler.popScope();
//  compiler.function->type = type;
//  function = compiler.function;
  return type;
}

Type LiteralExpr::findTypes(Parser &parser) {
  if (type == VAL_OBJ)
    return stringType;
  else
    return {type};
}

Type LogicalExpr::findTypes(Parser &parser) {
  Type type1 = left->findTypes(parser);
  Type type2 = right->findTypes(parser);

  hasSuperCalls = left->hasSuperCalls || right->hasSuperCalls;

  if (!IS_BOOL(type1) || !IS_BOOL(type2))
    parser.error("Value must be boolean");

  return BOOL_TYPE;
}

Type WhileExpr::findTypes(Parser &parser) {
  Type type = condition->findTypes(parser);

  body->findTypes(parser);
  hasSuperCalls = condition->hasSuperCalls || body->hasSuperCalls;
  return VOID_TYPE;
}

Type ReturnExpr::findTypes(Parser &parser) {
  // sync processing below

  if (value) {
    value->findTypes(parser);
    hasSuperCalls = value->hasSuperCalls;

//  if (!getCurrent()->isClass())
//    Type type = removeDeclaration();
    // verify that type is the function return type if not an instance
    // else return void
  }

  return VOID_TYPE;
}

Type SetExpr::findTypes(Parser &parser) {
  pushSignature(NULL);
  Type objectType = object->findTypes(parser);
  Type valueType = value->findTypes(parser);
  popSignature();

  hasSuperCalls = object->hasSuperCalls || value->hasSuperCalls;

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
      return getDeclarationType(refExpr);
    }

    parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
  }

  return VOID_TYPE;
}

Type TernaryExpr::findTypes(Parser &parser) {
  if (IS_VOID(left->findTypes(parser)))
    parser.error("Value must not be void");

  Type type = middle->findTypes(parser);

  hasSuperCalls = left->hasSuperCalls || middle->hasSuperCalls;

  if (right) {
    right->findTypes(parser);
    hasSuperCalls |= right->hasSuperCalls;
  }

  return type;
}

Type ThisExpr::findTypes(Parser &parser) {
  return VOID_TYPE;
}

Type UnaryExpr::findTypes(Parser &parser) {
  Type type = right->findTypes(parser);

  hasSuperCalls = right->hasSuperCalls;

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

    return type;
  }

  case TOKEN_PERCENT:
//    right = convertToFloat(right, type, parser);
    return FLOAT_TYPE;

  default:
    return type;
  }
}

Type PrimitiveExpr::findTypes(Parser &parser) {
  return UNKNOWN_TYPE;
}

Type ReferenceExpr::findTypes(Parser &parser) {
  Declaration *first = getFirstDeclarationRef(getCurrent(), name);

  declaration = resolveReference(first, name, getSignature(), &parser);

  if (declaration)
    switch(declaration->type) {
      case EXPR_DECLARATION: return ((DeclarationExpr *) declaration)->_declaration.type;
      case EXPR_FUNCTION: return OBJ_TYPE(&((FunctionExpr *) declaration)->_function);
      default: parser.error("Variable '%.*s' is not a call.", name.length, name.start);
    }

  return UNKNOWN_TYPE;
}

Type SwapExpr::findTypes(Parser &parser) {
  Type type;/* = uiTypes.front();

  _expr = uiExprs.front();
  uiExprs.pop_front();
  uiTypes.pop_front();
*/
  if (type.valueType == VAL_UNKNOWN)
    return _expr->findTypes(parser);

  return type;
}

Type NativeExpr::findTypes(Parser &parser) {
  return anyType;
}
