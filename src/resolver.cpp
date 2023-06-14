/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include <list>
#include <set>
#include <sstream>
#include "parser.hpp"
#include "memory.h"
#include "qni.hpp"
#include <stack>

#define UI_PARSES_DEF \
    UI_PARSE_DEF( PARSE_VALUES, &processAttrs ), \
    UI_PARSE_DEF( PARSE_AREAS, &pushAreas ), \
    UI_PARSE_DEF( PARSE_LAYOUT, &recalcLayout ), \
    UI_PARSE_DEF( PARSE_REFRESH, &paint ), \
    UI_PARSE_DEF( PARSE_EVENTS, &onEvent ), \

#define UI_PARSE_DEF( identifier, directiveFn )  identifier
typedef enum { UI_PARSES_DEF } ParseStep;
#undef UI_PARSE_DEF

struct ValueStack3 {
  int max;
	std::stack<int> *map;

  ValueStack3(int max) {
    this->max = max;
    map = new std::stack<int>[max];

    for (int index = 0; index < max; index++)
      map[index].push(-1);
  }

  ~ValueStack3() {
    delete[] map;
  }

	void push(int key, int value) {
    if (key >= 0 && key < max)
      map[key].push(value);
  }

	void pop(int key) {
    if (key >= 0 && key < max)
      map[key].pop();
  }

	int get(int key) {
    return key >= 0 && key < max ? map[key].top() : -1;
  }
};

Type popType1();
bool resolve(Compiler *compiler);
void acceptGroupingExprUnits(GroupingExpr *expr);
void acceptSubExpr(Expr *expr);
void parse(GroupingExpr *groupingExpr, const char *source, int index, int replace, Expr *body);

ParseStep getParseStep();
int getParseDir();

void processAttrs(UIDirectiveExpr *expr, Parser &parser);
void pushAreas(UIDirectiveExpr *expr, Parser &parser);
void recalcLayout(UIDirectiveExpr *expr, Parser &parser);
int adjustLayout(UIDirectiveExpr *expr, Parser &parser);
void paint(UIDirectiveExpr *expr, Parser &parser);
void onEvent(UIDirectiveExpr *expr, Parser &parser);

typedef void (*DirectiveFn)(UIDirectiveExpr *expr, Parser &parser);

#define UI_PARSE_DEF( identifier, directiveFn )  { directiveFn }
DirectiveFn directiveFns[] = { UI_PARSES_DEF };
#undef UI_PARSE_DEF

extern ParseExpRule expRules[];

/*
#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif
*/
int uiParseCount = -1;

static Obj objInternalType = {OBJ_INTERNAL};

bool isType(Type &type) {
  switch (AS_OBJ_TYPE(type)) {
  case OBJ_FUNCTION: {
    ObjString *name = AS_FUNCTION_TYPE(type)->name;

    return name != NULL && name->chars[0] >= 'A' && name->chars[0] <= 'Z';
  }

  case OBJ_ARRAY:
  case OBJ_FUNCTION_PTR:
    return true;
  }

  return false;
}

Type convertType(Type &type) {
  return IS_FUNCTION(type) ? (Type) {VAL_OBJ, &newInstance(AS_FUNCTION_TYPE(type))->obj} : type;
}

static Expr *convertToString(Expr *expr, Type &type, Parser &parser) {
  if (type.valueType == VAL_VOID)
    parser.error("Value must not be void");
  else
    if (!type.equals(stringType)) {
      CastExpr *castExpr = new CastExpr(NULL, expr);

      castExpr->_srcType = type;
      castExpr->_dstType = stringType;
      expr = castExpr;
    }

  return expr;
}

static Expr *convertToInt(Expr *expr, Type &type, Parser &parser) {
  switch (type.valueType) {
  case VAL_INT:
    break;

  case VAL_VOID:
  case VAL_OBJ:
    parser.error("Value cannot be cast to integer");
    break;

  default: {
    CastExpr *castExpr = new CastExpr(NULL, expr);

    castExpr->_srcType = type;
    castExpr->_dstType = {VAL_INT};
    expr = castExpr;
    break;
    }
  }

  return expr;
}

static Expr *convertToFloat(Expr *expr, Type &type, Parser &parser) {
  switch (type.valueType) {
  case VAL_FLOAT:
    break;

  case VAL_VOID:
  case VAL_OBJ:
    parser.error("Value cannot be cast to float");
    break;

  default: {
    CastExpr *castExpr = new CastExpr(NULL, expr);

    castExpr->_srcType = type;
    castExpr->_dstType = {VAL_FLOAT};
    expr = castExpr;
    break;
    }
  }

  return expr;
}

static Expr *convertToObj(Obj *srcObjType, Expr *expr, Type &type, Parser &parser) {
  switch (srcObjType->type) {
  case OBJ_ARRAY:
//    expr = convertToArray(OP_BOOL_TO_STRING, expr);
    break;
/*
  OBJ_OBJECT
  OBJ_CLOSURE
  OBJ_FUNCTION
  OBJ_NATIVE
  OBJ_NATIVE_CLASS
  OBJ_STRING
  OBJ_FUNCTION_PTR
*/
  case OBJ_STRING:
    expr = convertToString(expr, type, parser);
    break;

  case OBJ_INSTANCE:
    break;

  default:
    parser.error("Cannot convert to object");
    break;
  }

  return expr;
}

static Expr *convertToType(Type srcType, Expr *expr, Type &type, Parser &parser) {
  switch (srcType.valueType) {
  case VAL_INT:
    expr = convertToInt(expr, type, parser);
    break;

  case VAL_FLOAT:
    expr = convertToFloat(expr, type, parser);
    break;

  case VAL_BOOL:
//    expr = convertToBool(OP_BOOL_TO_STRING, expr);
    break;

  case VAL_OBJ:
    expr = convertToObj(srcType.objType, expr, type, parser);
    break;

  case VAL_VOID:
    parser.error("Value must not be void");
    break;

  default:
    break;
  }

  return expr;
}

static int aCount;
static std::stringstream s;
static std::stringstream *ss = &s;

static int nTabs = 0;

static void insertTabs() {
  for (int index = 0; index < nTabs; index++)
    (*ss) << "  ";
}

static const char *getGroupName(UIDirectiveExpr *expr, int dir);

Type acceptGroupingExprUnits(GroupingExpr *expr, int start, Parser &parser) {
  Type returnType = {VAL_VOID};
  TokenType type = expr->name.type;
  bool parenFlag = type == TOKEN_LEFT_PAREN;

  for (int index = start; index < expr->count; index++) {
    Expr *subExpr = expr->expressions[index];

    if (subExpr->type == EXPR_UIDIRECTIVE) {
      if (++uiParseCount >= 0)
        ss->str("");

      if (getParseStep() == PARSE_EVENTS) {
        insertTabs();
        (*ss) << "bool flag = false\n";
      }
    }

    Type subType = subExpr->resolve(parser);

    if (parenFlag)
      returnType = subType;

    if (subExpr->type == EXPR_UIDIRECTIVE) {
      UIDirectiveExpr *exprUI = (UIDirectiveExpr *) subExpr;

      if (uiParseCount >= 0) {
        if (uiParseCount == 0) {
          getCurrent()->function->eventFlags = exprUI->_eventFlags;

          for (int ndx2 = -1; (ndx2 = getCurrent()->function->instanceIndexes->getNext(ndx2)) != -1;)
            (*ss) << "v" << ndx2 << ".ui_ = new v" << ndx2 << ".UI_();\n";
        }

        if (uiParseCount == 3) {
          nTabs++;
          insertTabs();
          (*ss) << "var size = (" << getGroupName(exprUI, 0) << " << 16) | " << getGroupName(exprUI, 1) << "\n";
          nTabs--;
        }
        std::string *str = new std::string(ss->str().c_str());
#ifdef DEBUG_PRINT_CODE
        printf("CODE %d\n%s", uiParseCount, str->c_str());
#endif
        parse(expr, str->c_str(), index, 1, expr);
      }/*
      else {
        getCurrent()->function->eventFlags = exprUI->_eventFlags;
        expressions = RESIZE_ARRAY(Expr *, expressions, count, count + uiExprs.size() - 1);
        memmove(&expressions[index + uiExprs.size()], &expressions[index + 1], (--count - index) * sizeof(Expr *));

        for (Expr *uiExpr : uiExprs)
          expressions[index++] = uiExpr;

        count += uiExprs.size();
        uiExprs.clear();
      }
*/
      index--;
    }
  }

  if (expr->ui != NULL) {
    UIDirectiveExpr *exprUI = (UIDirectiveExpr *) expr->ui;

    if (exprUI->previous || exprUI->lastChild) {
      // Perform the UI AST magic
      ss->str("");
      insertTabs();
      (*ss) << "void UI_() {\n";
      nTabs++;
      processAttrs(exprUI, parser);

      for (int ndx2 = -1; (ndx2 = getCurrent()->function->instanceIndexes->getNext(ndx2)) != -1;)
        (*ss) << "v" << ndx2 << ".ui_ = new v" << ndx2 << ".UI_();\n";
      nTabs--;
      insertTabs();
      (*ss) << "}\n";
      printf(ss->str().c_str());
      parse(expr, ss->str().c_str(), 0, 0, NULL);

      FunctionExpr *uiFunctionExpr = (FunctionExpr *) expr->expressions[expr->count - 1];

      uiFunctionExpr->resolve(parser);
      uiFunctionExpr->_function.eventFlags = exprUI->_eventFlags;

      ObjFunction *uiFunction = (ObjFunction *) uiFunctionExpr->_declaration->type.objType;

      getCurrent()->function->uiFunction = uiFunction;
      parse(expr, "UI_ ui_;\n", 0, 0, NULL);
      expr->expressions[expr->count - 1]->resolve(parser);

      uiFunction->compiler->pushScope();
      ss->str("");
      insertTabs();
      (*ss) << "void Layout_() {\n";
      nTabs++;
      aCount = 1;
      pushAreas(exprUI, parser);
      uiParseCount = PARSE_LAYOUT;
      recalcLayout(exprUI, parser);
      uiParseCount++;
      recalcLayout(exprUI, parser);
      insertTabs();
      (*ss) << "var size = (" << getGroupName(exprUI, 0) << " << 16) | " << getGroupName(exprUI, 1) << "\n";
      nTabs--;
      insertTabs();
      (*ss) << "}\n";
      printf(ss->str().c_str());
      parse(uiFunctionExpr->body, ss->str().c_str(), 0, 0, NULL);

      FunctionExpr *layoutFunctionExpr = (FunctionExpr *) uiFunctionExpr->body->expressions[uiFunctionExpr->body->count - 1];

      layoutFunctionExpr->resolve(parser);

      ObjFunction *layoutFunction = (ObjFunction *) layoutFunctionExpr->_declaration->type.objType;

      layoutFunction->compiler->pushScope();

      ss->str("");
      insertTabs();
      (*ss) << "void paint(int pos0, int pos1, int size0, int size1) {\n";
      nTabs++;
      paint(exprUI, parser);
      nTabs--;
      insertTabs();
      (*ss) << "}\n";
      printf(ss->str().c_str());
      parse(layoutFunctionExpr->body, ss->str().c_str(), 0, 0, NULL);

      FunctionExpr *paintFunctionExpr = (FunctionExpr *) layoutFunctionExpr->body->expressions[layoutFunctionExpr->body->count - 1];

      paintFunctionExpr->resolve(parser);

      ss->str("");
      insertTabs();
      (*ss) << "bool onEvent(int event, int pos0, int pos1, int size0, int size1) {\n";
      nTabs++;
      insertTabs();
      (*ss) << "bool flag = false\n";
      onEvent(exprUI, parser);
      nTabs--;
      insertTabs();
      (*ss) << "}\n";
      printf(ss->str().c_str());
      parse(layoutFunctionExpr->body, ss->str().c_str(), 0, 0, NULL);

      FunctionExpr *eventFunctionExpr = (FunctionExpr *) layoutFunctionExpr->body->expressions[layoutFunctionExpr->body->count - 1];

      eventFunctionExpr->resolve(parser);

      getCurrent()->endScope();
      getCurrent()->endScope();
    }

    delete exprUI;
    expr->ui = NULL;
  }

  return returnType;
}

Type AssignExpr::resolve(Parser &parser) {
  Type type1 = varExp->resolve(parser);
  Type type2 = value ? value->resolve(parser) : type1;

  if (IS_VOID(type1))
    parser.error("Variable not found");
  else if (!type1.equals(type2)) {
    value = convertToType(type2, value, type1, parser);

    if (!value) {
      parser.error("Value must match the variable type");
    }
  }

  return type1;
}

Type UIAttributeExpr::resolve(Parser &parser) {
  parser.error("Internal error, cannot be invoked..");
  return {VAL_VOID};
}

Type UIDirectiveExpr::resolve(Parser &parser) {
  (*directiveFns[getParseStep()])(this, parser);
  return {VAL_VOID};
}

Type BinaryExpr::resolve(Parser &parser) {
  Type type1 = left->resolve(parser);
  Type type2 = right->resolve(parser);
  Type type = type1;
  bool boolVal = false;

  switch (op.type) {
  case TOKEN_PLUS:
    if (IS_OBJ(type1)) {
      right = convertToString(right, type2, parser);
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

      return {VAL_BOOL};
    }
    // no break statement, fall through

  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH: {
    switch (type1.valueType) {
    case VAL_INT:
      if (!type1.equals(type2)) {
        if (!IS_FLOAT(type2)) {
          parser.error("Second operand must be numeric");
          return {VAL_VOID};
        }

        left = convertToFloat(left, type1, parser);
        return {boolVal ? VAL_BOOL : VAL_FLOAT};
      }
      else
        return {boolVal ? VAL_BOOL : VAL_INT};

    case VAL_FLOAT:
      if (!type1.equals(type2)) {
        if (!IS_INT(type2)) {
          parser.error("Second operand must be numeric");
          return {VAL_VOID};
        }

        right = convertToFloat(right, type2, parser);
      }

      return {boolVal ? VAL_BOOL : VAL_FLOAT};

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
    return {VAL_INT};

  case TOKEN_OR_OR:
  case TOKEN_AND_AND:
    return {VAL_BOOL};

  default:
    return {VAL_VOID}; // Unreachable.
  }
}

static Token tok = buildToken(TOKEN_IDENTIFIER, "Capital", 7, -1);
Type CallExpr::resolve(Parser &parser) {
  Compiler compiler;
  ObjFunction signature;

  signature.compiler = &compiler;
  compiler.function = &signature;
  compiler.function->name = copyString("Capital", 7);
  compiler.declarationCount = 0;

//  return {VAL_OBJ}; // Dummy callee for space purposes

  compiler.addDeclaration({VAL_VOID}, tok, NULL, false, &parser);

  for (int index = 0; index < count; index++) {
    Type type = arguments[index]->resolve(parser);

    compiler.addDeclaration(type, tok, NULL, false, &parser);
  }

  pushSignature(&signature);
  Type type = callee->resolve(parser);
  popSignature();

  switch (AS_OBJ_TYPE(type)) {
  case OBJ_FUNCTION: {
    callable = AS_FUNCTION_TYPE(type);

    if (newFlag) {/*
      if (handler != NULL) {
        char buffer[256] = "";

        if (!IS_VOID(callable->type))
          sprintf(buffer, "%s _ret", callable->type.toString());

        handler = generateFunction("void", "ReturnHandler_", !IS_VOID(callable->type) ? buffer : NULL, handler, 1, 0, NULL);
        handler->resolve(parser);
        Type type = getCurrent()->peekDeclaration();// = popType();
        handlerFunction = AS_FUNCTION_TYPE(type);
        getCurrent()->declarationCount--;
      }
*/
      return (Type) {VAL_OBJ, &newInstance(callable)->obj};
    }
    else
      return callable->type;
    break;
  }/*
  case OBJ_FUNCTION_PTR: {
    ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;

    if (newFlag) {
      if (handler != NULL) {
        char buffer[256] = "";

        if (!IS_VOID(callable->type))
          sprintf(buffer, "%s _ret", callable->type.toString());

        handler = generateFunction("void", "ReturnHandler_", !IS_VOID(callable->type) ? buffer : NULL, handler, 1, 0, NULL);
        handler->resolve(parser);
      }

;//        getCurrent()->addDeclaration((Type) {VAL_OBJ, &newInstance(callable)->obj});
      return {VAL_VOID};
    }
    else
      return callable->type;
    break;
  }*/
  default:
    parser.error("Non-callable object type");
    return {VAL_VOID};
    break; // Non-callable object type.
  }
}

Type ArrayElementExpr::resolve(Parser &parser) {
  Type type = callee->resolve(parser);

  if (!count)
    if (isType(type)) {
      Type returnType = convertType(type);
      ObjArray *array = newArray();

      array->elementType = returnType;
      return {VAL_OBJ, &array->obj};
    }
    else {
      parser.error("No index defined.");
      return {VAL_VOID};
    }
  else
    if (isType(type)) {
      parser.error("A type cannot have an index.");
      return {VAL_VOID};
    }
    else
      switch (AS_OBJ_TYPE(type)) {
      case OBJ_ARRAY: {
        ObjArray *array = AS_ARRAY_TYPE(type);

        for (int index = 0; index < count; index++)
          indexes[index]->resolve(parser);

        return array->elementType;
      }
      case OBJ_STRING: {/*
        ObjString *string = (ObjString *)type.objType;

        if (count != string->arity)
          parser.error("Expected %d arguments but got %d.", string->arity, count);

        getCurrent()->addDeclaration(string->type.valueType);

        for (int index = 0; index < count; index++) {
          indexes[index]->resolve(parser);
          Type argType = removeDeclaration();

          argType = argType;
        }*/
        return {VAL_VOID};
      }
      default:
        parser.error("Non-indexable object type");
        return {VAL_VOID};
      }
}

Type DeclarationExpr::resolve(Parser &parser) {
  Type type = typeExpr->resolve(parser);
  Type returnType = convertType(type);

  if (initExpr) {
    Type type1 = initExpr->resolve(parser);
    Type internalType = {VAL_OBJ, &objInternalType};

    if (returnType.equals(internalType) || type.valueType == VAL_UNKNOWN)
      returnType = type1;
    else if (!type1.equals(returnType)) {
      initExpr = convertToType(returnType, initExpr, type1, parser);

      if (!initExpr) {
        parser.error("Value must match the variable type");
      }
    }
  }
  else
    switch (type.valueType) {
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
      switch (AS_OBJ_TYPE(returnType)) {
      case OBJ_STRING:
        initExpr = new LiteralExpr(VAL_OBJ, {.obj = &copyString("", 0)->obj});
        break;

      case OBJ_INTERNAL:
        initExpr = new LiteralExpr(VAL_OBJ, {.obj = &newInternal()->obj});
        break;
      }
      break;
    }

  _declaration = getCurrent()->checkDeclaration(returnType, name, NULL, &parser);
  return {VAL_VOID};
}

Type FunctionExpr::resolve(Parser &parser) {
//  Type type = typeExpr->resolve(parser);
//  Type returnType = convertType(type);
  Compiler &compiler = body->_compiler;

//  function = newFunction(returnType, copyString(name.start, name.length), count);
//  compiler.parser = &parser;
  compiler.pushScope();
//  bindFunction(compiler.prefix, function);

//  for (int index = 0; index < count; index++)
//    params[index]->resolve(parser);

  const char *str = name.getString().c_str();
  char firstChar = str[0];
  bool handlerFlag = str[strlen(str) - 1] == '_';

  if (!handlerFlag && firstChar >= 'A' && firstChar <= 'Z') {
    char buffer[256] = "";
    char buf[512];

    if (!IS_VOID(compiler.function->type))
      sprintf(buffer, "%s _ret", compiler.function->type.toString());

    sprintf(buf, "void ReturnHandler_(%s) {};", buffer);

    parse(getCurrent()->groupingExpr, buf, 0, 0, NULL);

    getCurrent()->groupingExpr->expressions[getCurrent()->groupingExpr->count - 1]->resolve(parser);
  }

//  _declaration = getCurrent()->enclosing->checkDeclaration({VAL_OBJ, &function->obj}, name, function);
//  function->bodyExpr = body;

  if (body)
    acceptGroupingExprUnits(body, arity, parser);

  compiler.endScope();
  return {VAL_VOID};
}

Type GetExpr::resolve(Parser &parser) {
  pushSignature(NULL);
  Type objectType = object->resolve(parser);
  popSignature();

  if (AS_OBJ_TYPE(objectType) != OBJ_INSTANCE)
    parser.errorAt(&name, "Only instances have properties.");
  else {
    ObjInstance *type = AS_INSTANCE_TYPE(objectType);

    for (int count = 0, i = 0; i < type->callable->compiler->declarationCount; i++) {
      Declaration *dec = &type->callable->compiler->declarations[i];

      if (dec->isField) {
        if (identifiersEqual(&name, &dec->name)) {
          index = count;
          return dec->type;
        }

        count++;
      }
    }

    parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
  }

  return {VAL_VOID};
}

Type GroupingExpr::resolve(Parser &parser) {
  TokenType type = name.type;
  bool parenFlag = type == TOKEN_LEFT_PAREN;
  bool groupFlag = type == TOKEN_LEFT_BRACE || parenFlag;
  Type parenType;

  if (groupFlag) {
    _compiler.pushScope();
    parenType = acceptGroupingExprUnits(this, 0, parser);
    _compiler.endScope();
  }
  else
    parenType = acceptGroupingExprUnits(this, 0, parser);

  return parenType;
}

Type CastExpr::resolve(Parser &parser) {
  if (typeExpr) {
    Type type = typeExpr->resolve(parser);

   _dstType = convertType(type);
  }

  return _dstType;
}

Type IfExpr::resolve(Parser &parser) {
  if (IS_VOID(condition->resolve(parser)))
    parser.error("Value must not be void");

  thenBranch->resolve(parser);

  if (elseBranch)
    elseBranch->resolve(parser);

  return {VAL_VOID};
}

Type ArrayExpr::resolve(Parser &parser) {
  ObjArray *objArray = newArray();
  Type type = {VAL_OBJ, &objArray->obj};
//  Compiler compiler;

//  compiler.beginScope(newFunction({VAL_VOID}, NULL, 0));

  for (int index = 0; index < count; index++)
    objArray->elementType = expressions[index]->resolve(parser);

//  compiler.endScope();
//  compiler.function->type = type;
//  function = compiler.function;
  return type;
}

Type ListExpr::resolve(Parser &parser) {
  Type type = expressions[0]->resolve(parser);

  for (int index = 1; index < count; index++) {
    Expr *subExpr = expressions[index];
    Type type = subExpr->resolve(parser);

    if (IS_VOID(getCurrent()->peekDeclaration()))
      return type;
  }

  return {VAL_VOID};
}

Type LiteralExpr::resolve(Parser &parser) {
  if (type == VAL_OBJ)
    return stringType;
  else
    return {type};
}

Type LogicalExpr::resolve(Parser &parser) {
  Type type1 = left->resolve(parser);
  Type type2 = right->resolve(parser);

  if (!IS_BOOL(type1) || !IS_BOOL(type2))
    parser.error("Value must be boolean");

  return {VAL_BOOL};
}

Type WhileExpr::resolve(Parser &parser) {
  parser.compilerError("In OpcodeExpr!");
  return body->resolve(parser);
}

static std::list<Expr *> uiExprs;
static std::list<Type> uiTypes;

Type ReturnExpr::resolve(Parser &parser) {
  if (getCurrent()->function->isClass()) {
    char buf[128] = "{post(ReturnHandler_)}";

    if (value) {
      uiExprs.push_back(value);
      uiTypes.push_back({(ValueType)-1});
      strcpy(buf, "{void Ret_() {ReturnHandler_($EXPR)}; post(Ret_)}");
    }

    parse(getCurrent()->groupingExpr, buf, 0, 0, NULL);

    getCurrent()->groupingExpr->expressions[getCurrent()->groupingExpr->count - 1]->resolve(parser);
  }

  // sync processing below

  if (value) {
    value->resolve(parser);

//  if (!getCurrent()->isClass())
//    Type type = removeDeclaration();
    // verify that type is the function return type if not an instance
    // else return void
  }

  return {VAL_VOID};
}

Type SetExpr::resolve(Parser &parser) {
  Type objectType = object->resolve(parser);
  Type valueType = value->resolve(parser);

  if (AS_OBJ_TYPE(objectType) != OBJ_INSTANCE)
    parser.errorAt(&name, "Only instances have properties.");
  else {
    ObjInstance *type = AS_INSTANCE_TYPE(objectType);

    for (int count = 0, i = 0; i < type->callable->compiler->declarationCount; i++) {
      Declaration *dec = &type->callable->compiler->declarations[i];

      if (dec->isField) {
        if (identifiersEqual(&name, &dec->name)) {
          index = count;
          return dec->type;
        }

        count++;
      }
    }

    parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
  }

  return {VAL_VOID};
}

Type TernaryExpr::resolve(Parser &parser) {
  if (IS_VOID(left->resolve(parser)))
    parser.error("Value must not be void");

  Type type = middle->resolve(parser);

  if (right)
    right->resolve(parser);

  return type;
}

Type ThisExpr::resolve(Parser &parser) {
  return {VAL_VOID};
}

Type TypeExpr::resolve(Parser &parser) {/*
  if (index != -1) {
    Obj *obj = primitives[index];

    index = -1;
    _declaration = NULL;
    return {VAL_OBJ, obj};
  }

  return getCurrent()->resolveReferenceExpr(this);*/
  return {VAL_VOID};
}

Type UnaryExpr::resolve(Parser &parser) {
  Type type = right->resolve(parser);

  if (IS_VOID(type))
    parser.error("Value must not be void");

  switch (op.type) {
  case TOKEN_PRINT:
    right = convertToString(right, type, parser);
    return {VAL_VOID};

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
    right = convertToFloat(right, type, parser);
    return {VAL_FLOAT};

  default:
    return type;
  }
}

Type ReferenceExpr::resolve(Parser &parser) {
  if (returnType.valueType != VAL_UNKNOWN || name.equal("var"))
    return returnType;

  returnType = getCurrent()->resolveReferenceExpr(this, &parser);
  return returnType;
}

Type SwapExpr::resolve(Parser &parser) {
  Type type = uiTypes.front();

  _expr = uiExprs.front();
  uiExprs.pop_front();
  uiTypes.pop_front();

  if (type.valueType < VAL_VOID)
    return _expr->resolve(parser);

  return type;
}

Type NativeExpr::resolve(Parser &parser) {
  return {VAL_VOID};
}
/*
void AttrSet::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, int dimFlags, const Path &path, Value *values, IndexList *instanceIndexes, LocationUnit **areaUnits) {
  for (std::string key : flagsMap) {
//    int flags = attrs[key]->flags;

//    if ((flags & dimFlags) == flags) {
//      Path filteredPath = path.filterPathWrong(dimFlags, flags);
      int localIndex = attrs[key]->localIndex;
      Value *value = &values[localIndex];//filteredPath.getValue(valueTree.getValue(key));

      valueStack.push(key, value);

      if (!key.compare("out"))
        (dimFlags <<= 1) |= instanceIndexes->get(localIndex);
//    }
  }

  if (children != NULL)
    children->parseCreateAreasTree(vm, valueStack, dimFlags, path, values, instanceIndexes, areaUnits);
  else {
    Value *value = valueStack.get("out");

    if (value != NULL) {
      LocationUnit *unitArea;

      if (dimFlags & 1)
        unitArea = AS_INSTANCE(*value)->recalculate(vm, valueStack);
      else {
//        LambdaDeclaration func = Op.getPredefinedType(value);

        unitArea = new UnitArea({10, 10});
//vm.getStringSize(AS_OBJ(*value), valueStack);
      }

      areaUnits[offset] = LocationUnit::addValue(&areaUnits[offset], path, unitArea);
    }
  }

  for (std::string key : flagsMap) {
//    int flags = attrs[key]->flags;

//    if ((flags & dimFlags) == flags)
      valueStack.pop(key);
  }
}

void ChildAttrSets::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, int dimFlags, const Path &path, Value *values, IndexList *instanceIndexes, LocationUnit **areaUnits) {
  for (int index = 0; index < size(); index++) {
    AttrSet *attrSet = operator[](index);

    if ((attrSet->areaParseFlags & (1 << dimFlags)) != 0)
      attrSet->parseCreateAreasTree(vm, valueStack, dimFlags, path, values, instanceIndexes, areaUnits);
  }
}

LayoutUnitArea *ObjFunction::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, Value *values, IndexList *instanceIndexes, int dimFlags) {
  LocationUnit **subAreas = new LocationUnit *[attrSets->numAreas];

  memset(subAreas, 0, attrSets->numAreas * sizeof(LocationUnit *));
  attrSets->parseCreateAreasTree(vm, valueStack, 0, Path(), values, instanceIndexes, subAreas);

  DirType<IntTreeUnit *> sizeTrees;

  for (int dir = 0; dir < NUM_DIRS; dir++)
    sizeTrees[dir] = parseResize(dir, {0, 0}, *subAreas, 0);

  return new LayoutUnitArea(sizeTrees, subAreas);
}

IntTreeUnit *ObjFunction::parseResize(int dir, const Point &limits, LocationUnit *areas, int offset) {
  return topSizers[dir]->parseResize(&areas, Path(limits.size()), dir, limits);
}

void acceptSubExpr(Expr *expr) {
  expr->resolve(parser);

  if (type == EXPR_VARIABLE) {
    Type type = removeDeclaration();

    if (type.valueType == VAL_OBJ && type.objType->type == OBJ_FUNCTION) {
      ObjFunction *function = (ObjFunction *) type.objType;
      Type parms[function->arity];

      for (int index = 0; index < function->arity; index++)
        parms[index] = function->fields[index].type;

      type.objType = &newFunctionPtr(function->type, function->arity, parms)->obj;
    }

    getCurrent()->addDeclaration(type);
  }
}*/

void parse(GroupingExpr *groupingExpr, const char *source, int index, int replace, Expr *body) {
  Scanner scanner((new std::string(source))->c_str());
  Parser parser(scanner);

  parser.expList(groupingExpr, TOKEN_EOF, "Expect end of file.");
/*
  if (body == NULL)
    body = group;
  else
    if (group) {
      if (replace || group->count) {
        if (body->type != EXPR_GROUPING) {
          Expr **newExprList = RESIZE_ARRAY(Expr *, NULL, 0, 1);

          newExprList[0] = body;
          body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{", 1, -1), 1, newExprList, NULL, new Compiler);
          index = 0;
          replace = 0;
        }

        GroupingExpr *bodyGroup = (GroupingExpr *) body;

        bodyGroup->expressions = RESIZE_ARRAY(Expr *, bodyGroup->expressions, bodyGroup->count, bodyGroup->count + group->count - replace);
        bodyGroup->count -= replace;

        if (index < bodyGroup->count)
          memmove(&bodyGroup->expressions[index + group->count], &bodyGroup->expressions[index + replace], (bodyGroup->count - index) * sizeof(Expr *));

        memcpy(&bodyGroup->expressions[index], group->expressions, group->count * sizeof(Expr *));
        group->expressions = RESIZE_ARRAY(Expr *, group->expressions, group->count, 0);
        bodyGroup->count += group->count;
      }

      delete group;
    }

  return groupingExpr;*/
}

ParseStep getParseStep() {
  return uiParseCount <= PARSE_AREAS ? (ParseStep) uiParseCount : uiParseCount <= PARSE_AREAS + NUM_DIRS ? PARSE_LAYOUT : (ParseStep) (uiParseCount - NUM_DIRS + 1);
}

int getParseDir() {
  return uiParseCount - PARSE_LAYOUT;
}

#define UI_ATTRIBUTE_DEF( identifier, text, conversionFunction )  text
static const char *uiAttributes[] = { UI_ATTRIBUTES_DEF };
#undef UI_ATTRIBUTE_DEF

#define UI_EVENT_DEF( identifier, text )  text
static const char *uiEvents[] = { UI_EVENTS_DEF };
#undef UI_EVENT_DEF

static int findAttribute(const char **array, const char *attribute) {
  for (int index = 0; array[index]; index++)
    if (!strcmp(array[index], attribute))
      return index;

  return -1;
}

static char *generateInternalVarName(const char *prefix, int suffix) {
  char buffer[20];

  sprintf(buffer, "%s%d", prefix, suffix);

  char *str = new char[strlen(buffer) + 1];

  strcpy(str, buffer);
  return str;
}

static bool isEventHandler(UIAttributeExpr *attExpr) {
  return !memcmp(attExpr->name.getString().c_str(), "on", strlen("on"));
}

UIDirectiveExpr *parent = NULL;

void processAttrs(UIDirectiveExpr *expr, Parser &parser) {
  if (expr->previous)
    processAttrs(expr->previous, parser);

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    processAttrs(expr->lastChild, parser);
    parent = oldParent;
  }

  for (int index = 0; index < expr->attCount; index++) {
    UIAttributeExpr *attExpr = expr->attributes[index];
    const char *attrName = attExpr->name.getString().c_str();

    if (isEventHandler(attExpr)) {
      attExpr->_uiIndex = findAttribute(uiEvents, attrName);

      if (attExpr->_uiIndex != -1 && attExpr->handler)
        expr->_eventFlags |= 1L << attExpr->_uiIndex;
    }
    else {
      attExpr->_uiIndex = findAttribute(uiAttributes, attrName);

      if (attExpr->_uiIndex != -1 && attExpr->handler) {
        Type type = attExpr->handler->resolve(parser);

        if (!IS_VOID(type)) {
          if (attExpr->_uiIndex == ATTRIBUTE_OUT)
            switch (AS_OBJ_TYPE(type)) {
              case OBJ_FUNCTION:
                break;

              case OBJ_INSTANCE: {
                  getCurrent()->function->instanceIndexes->set(getCurrent()->vCount);

                  ObjFunction *function = (ObjFunction *) AS_INSTANCE_TYPE(type)->callable;

                  expr->_eventFlags |= function->uiFunction->eventFlags;
                }
                break;

              default:
                attExpr->handler = convertToString(attExpr->handler, type, parser);
                type = stringType;
                break;
            }

            insertTabs();
            (*ss) << "var v" << getCurrent()->vCount << " = $EXPR;\n";
            uiExprs.push_back(attExpr->handler);
            uiTypes.push_back(type);
            attExpr->_index = getCurrent()->vCount++;
            attExpr->handler = NULL;
/*
          char *name = generateInternalVarName("v", getCurrent()->getDeclarationCount());
          DeclarationExpr *decExpr = new DeclarationExpr(type, buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), attExpr->handler);

          attExpr->handler = NULL;
          attExpr->_index = getCurrent()->getDeclarationCount();
          uiExprs.push_back(decExpr);
          getCurrent()->addDeclaration(type, decExpr->name);*/
        }
        else
          parser.error("Value must not be void");
//          attExprs.push_back(expr);
      }
    }
  }

  if (parent)
    parent->_eventFlags |= expr->_eventFlags;
}

ValueStack3 valueStackSize(ATTRIBUTE_ALIGN);
ValueStack3 valueStackPaint(ATTRIBUTE_COLOR);

static UIAttributeExpr *findAttr(UIDirectiveExpr *expr, Attribute uiIndex) {
  static char name[20];

  for (int index = 0; index < expr->attCount; index++)
    if (!isEventHandler(expr->attributes[index]) && expr->attributes[index]->_uiIndex == uiIndex)
      return expr->attributes[index];

  return NULL;
}

static int findAttrName(UIDirectiveExpr *expr, Attribute uiIndex) {
  UIAttributeExpr *attrExp = findAttr(expr, uiIndex);

  return attrExp ? attrExp->_index : -1;
}

static const char *getValueVariableName(UIDirectiveExpr *expr, Attribute uiIndex) {
  static char name[20];
  UIAttributeExpr *attrExpr = findAttr(expr, uiIndex);

  if (attrExpr) {
    sprintf(name, "v%d", attrExpr->_index);
    return name;
  }
  else
    return NULL;
}

static bool hasAreas(UIDirectiveExpr *expr) {
  return expr->childrenViewFlag || expr->viewIndex;
}

static bool isAreaHeritable(int uiIndex) {
  return uiIndex > ATTRIBUTE_AREA_HERITABLE && uiIndex < ATTRIBUTE_AREA_END;
}

static bool isHeritable(int uiIndex) {
  return isAreaHeritable(uiIndex) || (uiIndex > ATTRIBUTE_HERITABLE && uiIndex < ATTRIBUTE_END);
}

// viewIndex = -1 -> non-sized unit or group
// viewIndex = 0  -> sized group
// viewIndex > 0  -> sized unit, has an associated variable
void pushAreas(UIDirectiveExpr *expr, Parser &parser) {
  if (expr->previous)
    pushAreas(expr->previous, parser);

  for (int index = 0; index < expr->attCount; index++)
    if (!isEventHandler(expr->attributes[index]))
      valueStackSize.push(expr->attributes[index]->_uiIndex, expr->attributes[index]->_index);

  for (int index = 0; index < expr->attCount; index++)
    if (!isEventHandler(expr->attributes[index]) && isAreaHeritable(expr->attributes[index]->_uiIndex) && expr->attributes[index]->_index != -1) {
      char name[20] = "";

      sprintf(name, "v%d", expr->attributes[index]->_index);
      insertTabs();
      (*ss) << "pushAttribute(" << expr->attributes[index]->_uiIndex << ", " << name << ")\n";
    }

  if (expr->lastChild) {
    pushAreas(expr->lastChild, parser);

    for (UIDirectiveExpr *child = expr->lastChild; !expr->childrenViewFlag && child; child = child->previous)
      expr->childrenViewFlag = hasAreas(child);
  }

  const char *size = getValueVariableName(expr, ATTRIBUTE_SIZE);

  if (size != NULL) {
    expr->viewIndex = aCount;
    (*ss) << "  var a" << aCount++ << " = (" << size << " << 16) | " << size << "\n";
  }
  else {
    const char *name = getValueVariableName(expr, ATTRIBUTE_OUT);

    if (name != NULL) {
      Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
      Type outType = getCurrent()->getDeclaration(getCurrent()->resolveReference2(&token, &parser)).type;
      char *callee = NULL;

      switch (AS_OBJ_TYPE(outType)) {
        case OBJ_INSTANCE:
          expr->viewIndex = -aCount;
          (*ss) << "  var a" << aCount++ << " = new " << name << ".ui_.Layout_()\n";
          break;

        case OBJ_STRING:
          callee = "getTextSize";
          break;
      }

      if (callee) {
        expr->viewIndex = aCount;
        (*ss) << "  var a" << aCount++ << " = " << callee << "(" << name << ")\n";
      }
    }
  }

  for (int index = expr->attCount - 1; index >= 0; index--)
    if (!isEventHandler(expr->attributes[index]) && isAreaHeritable(expr->attributes[index]->_uiIndex) && expr->attributes[index]->_index != -1) {
      insertTabs();
      (*ss) << "popAttribute(" << expr->attributes[index]->_uiIndex << ")\n";
    }

  for (int index = 0; index < expr->attCount; index++)
    if (!isEventHandler(expr->attributes[index]))
      valueStackSize.pop(expr->attributes[index]->_uiIndex);
}

static const char *getUnitName(UIDirectiveExpr *expr, int dir) {
  if (expr->viewIndex) {
    static char name[16];

    sprintf(name, "u%d", expr->_layoutIndexes[dir]);
    return name;
  }
  else
    return getGroupName(expr->lastChild, dir);
}

static UIDirectiveExpr *getPrevious(UIDirectiveExpr *expr) {
  for (UIDirectiveExpr *previous = expr->previous; previous; previous = previous->previous)
    if (hasAreas(previous))
      return previous;

  return NULL;
}

static const char *getGroupName(UIDirectiveExpr *expr, int dir) {
  if (getPrevious(expr)) {
    static char name[16];

    sprintf(name, "l%d", expr->_layoutIndexes[dir]);
    return name;
  }
  else
    return getUnitName(expr, dir);
}

void recalcLayout(UIDirectiveExpr *expr, Parser &parser) {
  int dir = getParseDir();

  if (expr->previous)
    recalcLayout(expr->previous, parser);

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    recalcLayout(expr->lastChild, parser);
    parent = oldParent;
  }

  if (hasAreas(expr)) {
    UIDirectiveExpr *previous = getPrevious(expr);

    if (expr->viewIndex || previous)
      expr->_layoutIndexes[dir] = aCount++;

    if (expr->viewIndex) {
      (*ss) << "  var " << getUnitName(expr, dir) << " = a" << abs(expr->viewIndex);

      if (expr->viewIndex < 0)
        (*ss) << ".size";

      (*ss) << (dir ? " & 0xFFFF" : " >> 16") << "\n";
    }

    if (previous) {
      (*ss) << "  var " << getGroupName(expr, dir) << " = ";

      if (parent && parent->childDir & (1 << dir))
        (*ss) << getGroupName(previous, dir) << " + " << getUnitName(expr, dir) << "\n";
      else
        (*ss) << "max(" << getGroupName(previous, dir) << ", " << getUnitName(expr, dir) << ")\n";
    }
  }
}

static void insertPoint(const char *prefix) {/*
  ss << "new Point(";

  for (int dir = 0; dir < NUM_DIRS; dir++)
    ss << (dir == 0 ? "" : ", ") << prefix << dir;

  (*ss) << ")";*/
  (*ss) << "((" << prefix << "0 << 16) | " << prefix << "1)";
}

int adjustLayout(UIDirectiveExpr *expr, Parser &parser) {
  int posDiffDirs = 0;

  for (int dir = 0; dir < NUM_DIRS; dir++)
    if (parent && (parent->childDir & (1 << dir))) { // +
      UIDirectiveExpr *previous = getPrevious(expr);

      insertTabs();
      (*ss) << "int size" << dir << " = " << getGroupName(expr, dir);

      if (previous) {
        (*ss) << " - " << getGroupName(previous, dir) << "\n";
        insertTabs();
        (*ss) << "int posDiff" << dir << " = " << getGroupName(previous, dir);
        posDiffDirs |= 1 << dir;
      }

      (*ss) << "\n";
    }
    else {
      int expand = findAttrName(expr, ATTRIBUTE_EXPAND);
      int align = findAttrName(expr, ATTRIBUTE_ALIGN);

      if (expand != -1 || align != -1) {
        insertTabs();
        (*ss) << "int childSize" << dir << " = ";

        if (hasAreas(expr)) {
          (*ss) << getUnitName(expr, dir);

          if (expand != -1)
            (*ss) << " + (";
        }
        else
          (*ss) << "size" << dir;

        if (expand != -1) {
          if (hasAreas(expr))
            (*ss) << "size" << dir << " - " << getUnitName(expr, dir) << ")";

          (*ss) << " * v" << expand << "\n";
        }
        else
          (*ss) << "\n";
// expand != -1 viewindex != -1
// 00 int childSize = sizeX
// 01 int childSize = unitX
// 10 int childSize = sizeX * vExpand
// 11 int childSize = unitX + (sizeX - unitX) * vExpand
        if (align != -1) {
          insertTabs();
          (*ss) << "int posDiff" << dir << " = (size" << dir << " - childSize" << dir << ") * v" << align << "\n";
          posDiffDirs |= 1 << dir;
        }

        insertTabs();
        (*ss) << "int size" << dir << " = childSize" << dir << "\n";
      }
    }

  return posDiffDirs;
}

void paint(UIDirectiveExpr *expr, Parser &parser) {
  if (expr->previous)
    paint(expr->previous, parser);

  if (nTabs) {
    insertTabs();
    (*ss) << "{\n";
  }

  nTabs++;

  for (int index = 0; index < expr->attCount; index++)
    if (!isEventHandler(expr->attributes[index]))
      valueStackPaint.push(expr->attributes[index]->_uiIndex, expr->attributes[index]->_index);

  for (int index = 0; index < expr->attCount; index++)
    if (!isEventHandler(expr->attributes[index]) && isHeritable(expr->attributes[index]->_uiIndex) && expr->attributes[index]->_index != -1) {
      char name[20] = "";

      sprintf(name, "v%d", expr->attributes[index]->_index);
      insertTabs();
      (*ss) << "pushAttribute(" << expr->attributes[index]->_uiIndex << ", " << name << ")\n";
    }

  if (nTabs > 1) {
    int posDiffDirs = adjustLayout(expr, parser);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      if (posDiffDirs & (1 << dir)) {
        insertTabs();
        (*ss) << "int pos" << dir << " = pos" << dir << " + posDiff" << dir << "\n";
      }
  }

  char *name = (char *) getValueVariableName(expr, ATTRIBUTE_OUT);
  char *callee = NULL;

  if (name) {
    if (expr->lastChild) {
      insertTabs();
      (*ss) << "saveContext()\n";
    }

    Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
    Type outType = getCurrent()->enclosing->getDeclaration(getCurrent()->enclosing->resolveReference2(&token, &parser)).type;
    switch (AS_OBJ_TYPE(outType)) {
      case OBJ_INSTANCE:
        insertTabs();
        (*ss) << "a" << abs(expr->viewIndex) << ".paint(pos0, pos1, size0, size1)\n";
        break;

      case OBJ_STRING:
        callee = "displayText";
        break;

      case OBJ_FUNCTION:
        callee = AS_FUNCTION_TYPE(outType)->name->chars;
        name[0] = 0;
        break;

      default:
        if (!IS_OBJ(outType) || !outType.objType)
          parser.error("Out value having an illegal type");
        break;
    }

    if (callee) {
      insertTabs();
      (*ss) << callee << "(" << name << (name[0] ? ", " : "");
      insertPoint("pos");
      (*ss) << ", ";
      insertPoint("size");
      (*ss) << ")\n";
    }
  }

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    paint(expr->lastChild, parser);
    parent = oldParent;
  }

  if (name && callee && expr->lastChild) {
    insertTabs();
    (*ss) << "restoreContext()\n";
  }

  for (int index = expr->attCount - 1; index >= 0; index--)
    if (!isEventHandler(expr->attributes[index]) && isHeritable(expr->attributes[index]->_uiIndex) && expr->attributes[index]->_index != -1) {
      insertTabs();
      (*ss) << "popAttribute(" << expr->attributes[index]->_uiIndex << ")\n";
    }

  for (int index = expr->attCount - 1; index >= 0; index--)
    if (!isEventHandler(expr->attributes[index]))
      valueStackPaint.pop(expr->attributes[index]->_uiIndex);

  --nTabs;

  if (nTabs) {
    insertTabs();
    (*ss) << "}\n";
  }
}

void onEvent(UIDirectiveExpr *expr, Parser &parser) {
  if (expr->_eventFlags) {
    insertTabs();
    (*ss) << "if ((" << expr->_eventFlags << " & (1 << event)) != 0) {\n";
    nTabs++;

    int posDiffDirs = adjustLayout(expr, parser);
    int count = 0;

    insertTabs();
    (*ss) << "flag = ";

    for (int dir = 0; dir < NUM_DIRS; dir++)
      if (posDiffDirs & (1 << dir)) {
        (*ss) << (count++ ? " && " : "") << "pos" << dir << " >= posDiff" << dir;
        (*ss) << " && pos" << dir << " < posDiff" << dir << " + size" << dir;
      }
      else {
        (*ss) << (count++ ? " && " : "") << "pos" << dir << " >= 0";
        (*ss) << " && pos" << dir << " < size" << dir;
      }

    (*ss) << "\n";
    insertTabs();
    (*ss) << "if (flag) {\n";
    nTabs++;

    for (int dir = 0; dir < NUM_DIRS; dir++)
      if (posDiffDirs & (1 << dir)) {
        insertTabs();
        (*ss) << "pos" << dir << " = pos" << dir << " - posDiff" << dir << "\n";
      }

    for (int index = 0; index < expr->attCount; index++)
      if (!isEventHandler(expr->attributes[index]))
        valueStackPaint.push(expr->attributes[index]->_uiIndex, expr->attributes[index]->_index);

    if (expr->lastChild) {
      UIDirectiveExpr *oldParent = parent;

      parent = expr;
      onEvent(expr->lastChild, parser);
      parent = oldParent;
    }
    else {
      const char *name = getValueVariableName(expr, ATTRIBUTE_OUT);
      Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
      int index = getCurrent()->enclosing->resolveReference2(&token, &parser);
      Type outType = getCurrent()->enclosing->getDeclaration(index).type;

      switch (AS_OBJ_TYPE(outType)) {
        case OBJ_INSTANCE:
          insertTabs();
          (*ss) << "return a" << abs(expr->viewIndex) << ".onEvent(event, pos0, pos1, size0, size1)\n";
          break;

        case OBJ_STRING:
        case OBJ_FUNCTION:
          for (int index = 0; index < expr->attCount; index++) {
            UIAttributeExpr *attExpr = expr->attributes[index];

            if (isEventHandler(attExpr))
              if (attExpr->handler) {
                insertTabs();
                (*ss) << "if (event == " << attExpr->_uiIndex << ") {\n";
                nTabs++;

                insertTabs();
                (*ss) << "{void Ret_() {$EXPR}; post(Ret_)}\n";
                uiExprs.push_back(attExpr->handler);
                uiTypes.push_back({(ValueType)-1});
                attExpr->handler = NULL;

                --nTabs;
                insertTabs();
                (*ss) << "}\n";
              }
              else
                ;
          }
          break;

        default:
          parser.error("Out value having an illegal type");
          break;
      }
    }

    for (int index = 0; index < expr->attCount; index++)
      if (!isEventHandler(expr->attributes[index]))
        valueStackPaint.pop(expr->attributes[index]->_uiIndex);

    --nTabs;
    insertTabs();
    (*ss) << "}\n";
    --nTabs;
    insertTabs();
    (*ss) << "}\n";
  }

  if (expr->previous) {
    if (expr->_eventFlags) {
      insertTabs();
      (*ss) << "if (!flag) {\n";
      nTabs++;
    }

    onEvent(expr->previous, parser);

    if (expr->_eventFlags) {
      --nTabs;
      insertTabs();
      (*ss) << "}\n";
    }
  }
}
