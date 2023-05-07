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

Type popType();
bool resolve(Compiler *compiler);
void acceptGroupingExprUnits(GroupingExpr *expr);
void acceptSubExpr(Expr *expr);
Expr *parse(const char *source, int index, int replace, Expr *body);

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
static Obj *primitives[] = {
  &newPrimitive("void", {VAL_VOID})->obj,
  &newPrimitive("bool", {VAL_BOOL})->obj,
  &newPrimitive("int", {VAL_INT})->obj,
  &newPrimitive("float", {VAL_FLOAT})->obj,
  &newPrimitive("String", stringType)->obj,
  &newPrimitive("var", {VAL_OBJ, &objInternalType})->obj,
};

static bool isType(Type &type) {
  switch (AS_OBJ_TYPE(type)) {
  case OBJ_FUNCTION: {
    ObjString *name = AS_FUNCTION_TYPE(type)->name;

    return name != NULL && name->chars[0] >= 'A' && name->chars[0] <= 'Z';
  }

  case OBJ_ARRAY:
  case OBJ_PRIMITIVE:
  case OBJ_FUNCTION_PTR:
    return true;
  }

  return false;
}

static Type convertType(Type &type) {
  ObjPrimitive *primitiveType = AS_PRIMITIVE_TYPE(type);

  return primitiveType ? primitiveType->type : IS_FUNCTION(type) ? (Type) {VAL_OBJ, &newInstance(AS_FUNCTION_TYPE(type))->obj} : type;
}

static Expr *convertToString(Expr *expr, Type &type, Parser &parser) {
  switch (type.valueType) {
  case VAL_INT:
    expr = new OpcodeExpr(OP_INT_TO_STRING, expr);
    break;

  case VAL_FLOAT:
    expr = new OpcodeExpr(OP_FLOAT_TO_STRING, expr);
    break;

  case VAL_BOOL:
    expr = new OpcodeExpr(OP_BOOL_TO_STRING, expr);
    break;

  case VAL_VOID:
    parser.error("Value must not be void");
    break;

  case VAL_OBJ:
    //    expr = new OpcodeExpr(OP_OBJ_TO_STRING, expr);
    break;

  default:
    break;
  }

  return expr;
}

static Expr *convertToInt(Expr *expr, Type &type, Parser &parser) {
  switch (type.valueType) {
  case VAL_INT:
    break;

  case VAL_FLOAT:
    expr = new OpcodeExpr(OP_FLOAT_TO_INT, expr);
    break;

  case VAL_VOID:
    parser.error("Value must not be void");
    break;

  default:
    break;
  }

  return expr;
}

static Expr *convertToFloat(Expr *expr, Type &type, Parser &parser) {
  switch (type.valueType) {
  case VAL_INT:
    expr = new OpcodeExpr(OP_INT_TO_FLOAT, expr);
    break;

  case VAL_FLOAT:
    break;

  case VAL_VOID:
    parser.error("Value must not be void");
    break;

  default:
    break;
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

static OpCode getOpCode(Type type, Token token) {
  OpCode opCode = OP_FALSE;

  switch (token.type) {
  case TOKEN_PLUS:
  case TOKEN_PLUS_PLUS:
  case TOKEN_PLUS_EQUAL:
    opCode = IS_OBJ(type) ? OP_ADD_STRING : IS_INT(type) ? OP_ADD_INT : OP_ADD_FLOAT;
    break;
  case TOKEN_MINUS:
  case TOKEN_MINUS_MINUS:
  case TOKEN_MINUS_EQUAL:
    opCode = IS_INT(type) ? OP_SUBTRACT_INT : OP_SUBTRACT_FLOAT;
    break;
  case TOKEN_STAR:
  case TOKEN_STAR_EQUAL:
    opCode = IS_INT(type) ? OP_MULTIPLY_INT : OP_MULTIPLY_FLOAT;
    break;
  case TOKEN_SLASH:
  case TOKEN_SLASH_EQUAL:
    opCode = IS_INT(type) ? OP_DIVIDE_INT : OP_DIVIDE_FLOAT;
    break;
  case TOKEN_BANG_EQUAL:
  case TOKEN_EQUAL_EQUAL:
    opCode = IS_STRING(type) ? OP_EQUAL_STRING : IS_INT(type) ? OP_EQUAL_INT : OP_EQUAL_FLOAT;
    break;
  case TOKEN_GREATER:
  case TOKEN_LESS_EQUAL:
    opCode = IS_STRING(type) ? OP_GREATER_STRING : IS_INT(type) ? OP_GREATER_INT : OP_GREATER_FLOAT;
    break;
  case TOKEN_GREATER_EQUAL:
  case TOKEN_LESS:
    opCode = IS_STRING(type) ? OP_LESS_STRING : IS_INT(type) ? OP_LESS_INT : OP_LESS_FLOAT;
    break;
  case TOKEN_OR:
  case TOKEN_OR_EQUAL:
    opCode = IS_INT(type) ? OP_BITWISE_OR : OP_LOGICAL_OR;
    break;
  case TOKEN_OR_OR:
    opCode = OP_LOGICAL_OR;
    break;
  case TOKEN_AND:
  case TOKEN_AND_EQUAL:
    opCode = IS_INT(type) ? OP_BITWISE_AND : OP_LOGICAL_AND;
    break;
  case TOKEN_AND_AND:
    opCode = OP_LOGICAL_AND;
    break;
  case TOKEN_XOR:
  case TOKEN_XOR_EQUAL:
    opCode = OP_BITWISE_XOR;
    break;
  case TOKEN_GREATER_GREATER:
  case TOKEN_GREATER_GREATER_EQUAL:
    opCode = OP_SHIFT_RIGHT;
    break;
  case TOKEN_GREATER_GREATER_GREATER:
    opCode = OP_SHIFT_URIGHT;
    break;
  case TOKEN_LESS_LESS:
  case TOKEN_LESS_LESS_EQUAL:
    opCode = OP_SHIFT_LEFT;
    break;
  }

  return opCode;
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

static Expr *generateFunction(const char *type, const char *name, char *args, Expr *copyExpr, int count, int restLength, Expr **rest) {
    ReferenceExpr *nameExpr = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), -1, false);
    Expr **bodyExprs = new Expr *[count + restLength];
    Expr **functionExprs = new Expr *[3];
    int nbParms = 0;
    Expr **parms = NULL;

    if (args != NULL) {
      Scanner scanner(args);
      Parser parser(scanner);
      TokenType tokens[] = {TOKEN_COMMA, TOKEN_EOF};

      do {
        Expr *expr = parser.expression(tokens);

        parms = RESIZE_ARRAY(Expr *, parms, nbParms, nbParms + 1);
        parms[nbParms++] = expr;
      } while (parser.match(TOKEN_COMMA));

      parser.consume(TOKEN_EOF, "Expect EOF after arguments.");
    }

    for (int index = 0; index < count; index++)
      bodyExprs[index] = copyExpr;

    for (int index = 0; index < restLength; index++)
      bodyExprs[count + index] = rest[index];

    int typeIndex = -1;

    for (int index = 0; typeIndex == -1 && index < sizeof(primitives) / sizeof(Obj *); index++)
      if (!strcmp(((ObjPrimitive *) primitives[index])->name->chars, type))
        typeIndex = index;

    functionExprs[0] = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, type, strlen(type), -1), typeIndex, false);
    functionExprs[1] = new CallExpr(nameExpr, buildToken(TOKEN_RIGHT_PAREN, ")", 1, -1), nbParms, parms, false, NULL, NULL, NULL);
    functionExprs[2] = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), count + restLength, bodyExprs, 0, NULL);

    return new ListExpr(3, functionExprs, EXPR_LIST);
}

static Expr *generateUIFunction(const char *type, const char *name, char *args, Expr *uiExpr, int count, int restLength, Expr **rest) {
  return new StatementExpr(generateFunction(type, name, args, uiExpr, count, restLength, rest));
}

void acceptGroupingExprUnits(GroupingExpr *expr, Parser &parser) {
  TokenType type = expr->name.type;
  bool parenFlag = type == TOKEN_RIGHT_PAREN;

  for (int index = 0; index < expr->count; index++) {
    Expr *subExpr = expr->expressions[index];

    if (subExpr->type == EXPR_UIDIRECTIVE) {
      if (++uiParseCount >= 0)
        ss->str("");

      if (getParseStep() == PARSE_EVENTS) {
        insertTabs();
        (*ss) << "bool flag = false\n";
      }
    }

    subExpr->resolve(parser);

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
        expr = (GroupingExpr *) parse(str->c_str(), index, 1, expr);
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
    else
      if (IS_VOID(getCurrent()->peekDeclaration()) && (!parenFlag || index < expr->count - 1))
        popType();
  }

  if (expr->ui != NULL) {
    UIDirectiveExpr *exprUI = (UIDirectiveExpr *) expr->ui;

    if (exprUI->previous || exprUI->lastChild) {
      // Perform the UI AST magic
      Expr *clickFunction = generateUIFunction("bool", "onEvent", "int event, int pos0, int pos1, int size0, int size1", exprUI, 1, 0, NULL);
      Expr *paintFunction = generateUIFunction("void", "paint", "int pos0, int pos1, int size0, int size1", exprUI, 1, 0, NULL);
      Expr **uiFunctions = new Expr *[2];

      uiFunctions[0] = paintFunction;
      uiFunctions[1] = clickFunction;

      Expr *layoutFunction = generateUIFunction("void", "Layout_", NULL, exprUI, 3, 2, uiFunctions);
      Expr **layoutExprs = new Expr *[1];

      layoutExprs[0] = layoutFunction;

      Expr *valueFunction = generateUIFunction("void", "UI_", NULL, exprUI, 1, 1, layoutExprs);

      aCount = 1;
      valueFunction->resolve(parser);
      Type type = getCurrent()->peekDeclaration();// = popType();
      getCurrent()->function->uiFunction = AS_FUNCTION_TYPE(type);
//      getCurrent()->declarationCount--;
      delete exprUI;
      expr->ui = NULL;
      expr->expressions = RESIZE_ARRAY(Expr *, expr->expressions, expr->count, expr->count + 2);
      expr->expressions[expr->count++] = valueFunction;
      uiParseCount = -1;
      GroupingExpr *uiExpr = (GroupingExpr *) parse("UI_ ui_;\n", 0, 0, NULL);
      expr->expressions[expr->count++] = uiExpr->expressions[0];
      uiExpr->expressions[0]->resolve(parser);
    }
    else {
      delete exprUI;
      expr->ui = NULL;
    }
  }
}

void AssignExpr::resolve(Parser &parser) {
  if (value)
    value->resolve(parser);

  if (varExp)
    varExp->resolve(parser);

  Type type1 = popType();
  Type type2 = varExp && value ? popType() : type1;

  opCode = getOpCode(type1, op);

  if (IS_VOID(type1))
    parser.error("Variable not found");
  else if (!type1.equals(type2)) {
    value = convertToType(type2, value, type1, parser);

    if (!value) {
      parser.error("Value must match the variable type");
    }
  }

  getCurrent()->pushType(type1);
}

void UIAttributeExpr::resolve(Parser &parser) {
  parser.error("Internal error, cannot be invoked..");
}

void UIDirectiveExpr::resolve(Parser &parser) {
  (*directiveFns[getParseStep()])(this, parser);
}
#if 0
void UIDirectiveExpr::resolve(Parser &parser) {
  ParseStep step = getParseStep();

  if (resolverUIRules[step].attrListFn)
    (this->*resolverUIRules[step].attrListFn)(expr);
////////////////////
/*  if (outFlag)
    for (int index = 0; index < attCount; index++)
      attributes[index]->resolve(parser);
  else
    for (int index = 0; index < attCount; index++)
      if (attributes[index]->_index != -1)
        valueStack.push(attributes[index]->name.getString(), attributes[index]->_index);

  int numAttrSets = childrenCount;

  if (!numAttrSets) {
    if (!outFlag) {
      int index = valueStack.get("out");

      if (index != -1) {
        Type &type = getCurrent()->enclosing->declarations[index].type;

        if (type.valueType == VAL_OBJ)
          switch (type.objType->type) {
          case OBJ_INSTANCE:
            viewIndex = getCurrent()->declarationCount;
            getCurrent()->addDeclaration({VAL_INT});
            break;

          case OBJ_STRING:
            viewIndex = getCurrent()->declarationCount;
            getCurrent()->addDeclaration({VAL_OBJ, &newInternal()->obj});
            break;
          }
      }
    }
  }
  else
  / *
   Point numZones;
   int offset = 0;
   std::array<long, NUM_DIRS> arrayDirFlags = {2L, 1L};
   ValueStack valueStack;* /
    for (int index = 0; index < numAttrSets; index++)
      children[index]->resolve(parser);

  //  attrSets = numAttrSets != 0 ? new ChildAttrSets(&offset, numZones, 0, arrayDirFlags, valueStack, expr, 0) : NULL;
  //  function->attrSets = attrSets;

  //  if (attrSets != NULL) {
  //    Sizer *topSizers[NUM_DIRS];
  / *
      for (int dir = 0; dir < NUM_DIRS; dir++) {
        int *zone = NULL;

        topSizers[dir] = new Maxer(-1);
  / *
        if (parent is ImplicitArrayDeclaration) {
  //					int zFlags = attrSets.zFlags[dir];

          zone = {0};//{zFlags > 0 ? zFlags != 1 ? 1 : 0 : ctz({-zFlags - 1})};
          topSizers[dir]->put(SizerType.maxer, -1, 0, 0, 0, false, {Path()});
          topSizers[dir]->children.add(new Adder(-1));
        }
  * /
        attrSets->parseCreateSizers(topSizers, dir, zone, Path(), Path());
        attrSets->parseAdjustPaths(topSizers, dir);
        numZones[dir] = zone != NULL ? zone[0] + 1 : 1;
      }
  * /
  //    subAttrsets = parent is ImplicitArrayDeclaration ? parseCreateSubSets(topSizers, new Path(), numZones) : createIntersection(-1, topSizers);
  //    subAttrsets = createIntersection(-1, topSizers);
  //  }

  if (!outFlag)
    for (int index = 0; index < attCount; index++)
      if (attributes[index]->_index != -1)
        valueStack.pop(attributes[index]->name.getString());*/
}
#endif
void BinaryExpr::resolve(Parser &parser) {
  //  if (left) {
  left->resolve(parser);
  //    type = left->type;
  //  } else
  //    type = {VAL_VOID};

  //  if (right)
  right->resolve(parser);

  Type type2 = popType();
  Type type1 = popType();

  Type type = type1;
  bool boolVal = false;

  opCode = getOpCode(type, op);

  switch (op.type) {
  case TOKEN_BANG_EQUAL:
  case TOKEN_GREATER_EQUAL:
  case TOKEN_LESS_EQUAL:
    break;
  }

  switch (op.type) {
  case TOKEN_PLUS:
    if (IS_OBJ(type1)) {
      right = convertToString(right, type2, parser);
      getCurrent()->pushType(stringType);
      return;
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

      getCurrent()->pushType(VAL_BOOL);
      return;
    }
    // no break statement, fall through

  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH: {
    switch (type1.valueType) {
    case VAL_INT:
      if (!type1.equals(type2)) {
        if (!IS_FLOAT(type2)) {
          getCurrent()->pushType(VAL_VOID);
          parser.error("Second operand must be numeric");
          return;
        }

        left = convertToFloat(left, type1, parser);
        opCode =
            (OpCode)((int)opCode + (int)OP_ADD_FLOAT - (int)OP_ADD_INT);
        getCurrent()->pushType(boolVal ? VAL_BOOL : VAL_FLOAT);
      }
      else
        getCurrent()->pushType(boolVal ? VAL_BOOL : VAL_INT);
      break;

    case VAL_FLOAT:
      if (!type1.equals(type2)) {
        if (!IS_INT(type2)) {
          getCurrent()->pushType(VAL_VOID);
          parser.error("Second operand must be numeric");
          return;
        }

        right = convertToFloat(right, type2, parser);
      }

      getCurrent()->pushType(boolVal ? VAL_BOOL : VAL_FLOAT);
      break;

    default:
      parser.error("First operand must be numeric");
      getCurrent()->pushType(type1);
      break;
    }
  }
  break;

  case TOKEN_XOR:
    getCurrent()->pushType(IS_BOOL(type1) ? VAL_BOOL : VAL_FLOAT);
    break;

  case TOKEN_OR:
  case TOKEN_AND:
  case TOKEN_GREATER_GREATER_GREATER:
  case TOKEN_GREATER_GREATER:
  case TOKEN_LESS_LESS:
    getCurrent()->pushType(VAL_INT);
    break;

  case TOKEN_OR_OR:
  case TOKEN_AND_AND:
    getCurrent()->pushType(VAL_BOOL);
    break;

    break;

  default:
    return; // Unreachable.
  }
}

static Token tok = buildToken(TOKEN_IDENTIFIER, "Capital", 7, -1);
void CallExpr::resolve(Parser &parser) {
  Compiler compiler;
  ObjFunction signature;

  signature.arity = count;
  signature.compiler = &compiler;
  compiler.function = &signature;
  compiler.function->name = copyString("Capital", 7);
  compiler.declarationCount = 0;

  getCurrent()->pushType({VAL_OBJ}); // Dummy callee for space purposes

  compiler.addDeclaration({VAL_VOID}, tok, NULL, false);

  for (int index = 0; index < count; index++) {
    arguments[index]->resolve(parser);
    compiler.addDeclaration(getCurrent()->typeStack.top(), tok, NULL, false);
  }

  for (int index = -1; index < count; index++)
    popType();

  pushSignature(&signature);
  callee->resolve(parser);
  popSignature();

  Type type = popType();
  //  Declaration *callerLocal = getCurrent()->peekDeclaration(argCount);

  switch (AS_OBJ_TYPE(type)) {
  case OBJ_FUNCTION: {
    callable = AS_FUNCTION_TYPE(type);

    if (newFlag) {
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

      getCurrent()->pushType((Type) {VAL_OBJ, &newInstance(callable)->obj});
    }
    else
      getCurrent()->pushType(callable->type);
    break;
  }
  case OBJ_FUNCTION_PTR: {
    ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;

    if (newFlag) {
      if (handler != NULL) {
        char buffer[256] = "";

        if (!IS_VOID(callable->type))
          sprintf(buffer, "%s _ret", callable->type.toString());

        handler = generateFunction("void", "ReturnHandler_", !IS_VOID(callable->type) ? buffer : NULL, handler, 1, 0, NULL);
        handler->resolve(parser);
        popType();
      }

;//        getCurrent()->addDeclaration((Type) {VAL_OBJ, &newInstance(callable)->obj});
    }
    else
      getCurrent()->pushType(callable->type.valueType);
    break;
  }
  default:
    parser.error("Non-callable object type");
    getCurrent()->pushType(VAL_VOID);
    break; // Non-callable object type.
  }
}

void ArrayElementExpr::resolve(Parser &parser) {
  callee->resolve(parser);

  Type type = popType();

  if (!count)
    if (isType(type)) {
      Type returnType = convertType(type);
      ObjArray *array = newArray();

      array->elementType = returnType;
      getCurrent()->pushType({VAL_OBJ, &array->obj});
    }
    else {
      parser.error("No index defined.");
      getCurrent()->pushType(VAL_VOID);
    }
  else
    if (isType(type)) {
      parser.error("A type cannot have an index.");
      getCurrent()->pushType(VAL_VOID);
    }
    else
      switch (AS_OBJ_TYPE(type)) {
      case OBJ_ARRAY: {
        ObjArray *array = AS_ARRAY_TYPE(type);

        getCurrent()->pushType(array->elementType);

        for (int index = 0; index < count; index++)
          indexes[index]->resolve(parser);

        for (int index = 0; index < count; index++) {
          Type argType = popType();

          argType = argType;
        }
        break;
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
        break;
      }
      default:
        parser.error("Non-indexable object type");
        getCurrent()->pushType(VAL_VOID);
        break; // Non-indexable object type.
      }
}

void DeclarationExpr::resolve(Parser &parser) {
//  getCurrent()->checkDeclaration(&name);

  if (initExpr != NULL) {
    initExpr->resolve(parser);
    Type type1 = popType();
  }

  getCurrent()->addDeclaration(type, name, NULL, false);
}

void FunctionExpr::resolve(Parser &parser) {/*
  getCurrent()->checkDeclaration(&name);
  getCurrent()->addDeclaration(VAL_OBJ, &name);

  Compiler &compiler = ((GroupingExpr *) body)->_compiler;

  compiler.beginScope(newFunction(type, copyString(name.start, name.length), count));

  for (int index = 0; index < count; index++)
    params[index]->resolve(parser);

  body->resolve(parser);
  compiler.function->fieldCount = compiler.getDeclarationCount();
  compiler.function->fields = ALLOCATE(Field, compiler.getDeclarationCount());

  for (int index = 0; index < compiler.getDeclarationCount(); index++) {
    Declaration *dec = &compiler.getDeclaration(index);

    compiler.function->fields[index].type = dec->type;
    compiler.function->fields[index].name = copyString(dec->name.start, dec->name.length);
  }

  compiler.endScope();
  function = compiler.function;*/
}

void GetExpr::resolve(Parser &parser) {
  pushSignature(NULL);
  object->resolve(parser);
  popSignature();

  Type objectType = popType();

  if (AS_OBJ_TYPE(objectType) != OBJ_INSTANCE)
    parser.errorAt(&name, "Only instances have properties.");
  else {
    ObjInstance *type = AS_INSTANCE_TYPE(objectType);

    for (int count = 0, i = 0; i < type->callable->compiler->declarationCount; i++) {
      Declaration *dec = &type->callable->compiler->declarations[i];

      if (dec->isField) {
        if (identifiersEqual(&name, &dec->name)) {
          index = count;
          getCurrent()->pushType(dec->type);
          return;
        }

        count++;
      }
    }

    parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
  }

  getCurrent()->pushType({VAL_VOID});
}

void GroupingExpr::resolve(Parser &parser) {
  TokenType type = name.type;
  bool parenFlag = type == TOKEN_RIGHT_PAREN;
  bool groupFlag = type == TOKEN_RIGHT_BRACE || parenFlag;
  Type parenType;

  if (groupFlag) {
    _compiler.beginScope();
    acceptGroupingExprUnits(this, parser);
    parenType = parenFlag ? popType() : (Type){VAL_VOID};
    popLevels = _compiler.declarationCount;
    _compiler.endScope();
  }
  else {
    acceptGroupingExprUnits(this, parser);
    parenType = parenFlag ? popType() : (Type){VAL_VOID};
  }

  if (type != TOKEN_EOF)
    getCurrent()->pushType(parenType);
}

void ArrayExpr::resolve(Parser &parser) {
  ObjArray *objArray = newArray();
  Type type = {VAL_OBJ, &objArray->obj};
  Compiler compiler;

  compiler.beginScope(newFunction({VAL_VOID}, NULL, 0));

  for (int index = 0; index < count; index++)
    expressions[index]->resolve(parser);

  for (int index = 0; index < count; index++) {
    Type subType = popType();

    objArray->elementType = subType;
  }

  compiler.endScope();
  compiler.function->type = type;
  function = compiler.function;
  getCurrent()->pushType(type);
}

// DECLARATIONS:
// Type(EXP_VARIABLE) Call(EXP_CALL(EXP_VARIABLE(-1) type-var-1*))
// Group(EXP_GROUPING) => function Type(EXP_VARIABLE) Assignment(-1) =>
// declaration Original parse: (group','
//   (var 2)
//   (call (var -1) (group',' , (var 2), (var -1)) (group',' , (var 2), (var
//   -1))) (group'}' )
// );
// (group','
//   (var 2) (= -1 (+ 2 3))
// );
void ListExpr::resolve(Parser &parser) {
  expressions[0]->resolve(parser);
  _declaration = NULL;

  Type type = popType();

  if (isType(type)) {
    Type returnType = convertType(type);
    Expr *subExpr = expressions[1];

    switch (subExpr->type) {
    case EXPR_ASSIGN: {
      AssignExpr *assignExpr = (AssignExpr *)subExpr;

      if (assignExpr->varExp && assignExpr->value) {
        listType = subExpr->type;
        assignExpr->varExp->_declaration = NULL;
        assignExpr->value->resolve(parser);

        Type internalType = {VAL_OBJ, &objInternalType};
        Type type1 = popType();

        //          if (type1.valueType == VAL_VOID)
        //            parser.error("Variable not found");
        //          else
        if (returnType.equals(internalType))
          returnType = type1;
        else if (!type1.equals(returnType)) {
          assignExpr->value = convertToType(returnType, assignExpr->value, type1, parser);

          if (assignExpr->value == NULL) {
            parser.error("Value must match the variable type");
          }
        }

        getCurrent()->checkDeclaration(returnType, assignExpr->varExp, NULL);

        if (count > 2)
          parser.error("Expect ';' or newline after variable declaration.");
      }
      else
        parser.error("Unrecognized declaration.");
      return;
    }
    case EXPR_CALL: {
      listType = subExpr->type;

      CallExpr *callExpr = (CallExpr *)subExpr;

      if (callExpr->newFlag)
        parser.error("Syntax error: new.");
      else if (callExpr->callee->type != EXPR_REFERENCE)
        parser.error("Function name must be a string.");
      else {
        ReferenceExpr *varExp = (ReferenceExpr *)callExpr->callee;
        Expr *bodyExpr = count > 2 ? expressions[2] : NULL;
        GroupingExpr *body;

        if (bodyExpr && bodyExpr->type == EXPR_GROUPING && ((GroupingExpr *) bodyExpr)->name.type == TOKEN_RIGHT_BRACE)
          body = (GroupingExpr *) bodyExpr;
        else {
          Expr **expList = bodyExpr ? RESIZE_ARRAY(Expr *, NULL, 0, 1) : NULL;

          if (expList)
            expList[0] = bodyExpr;

          body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), expList ? 1 : 0, expList, 0, NULL);
        }

        Compiler &compiler = body->_compiler;
        ObjFunction *function = newFunction(returnType, copyString(varExp->name.start, varExp->name.length), callExpr->count);

        _declaration = compiler.beginScope(function);
        bindFunction(compiler.prefix, function);

        for (int index = 0; index < callExpr->count; index++)
          if (callExpr->arguments[index]->type == EXPR_LIST) {
            ListExpr *param = (ListExpr *)callExpr->arguments[index];
            Expr *paramExpr = param->expressions[0];

            paramExpr->resolve(parser);

            Type type = popType();

            if (isType(type)) {
              paramExpr = param->expressions[1];

              if (paramExpr->type != EXPR_REFERENCE)
                parser.error("Parameter name must be a string.");
              else {
                getCurrent()->addDeclaration(convertType(type), ((ReferenceExpr *)paramExpr)->name, NULL, false);

                if (param->count > 2)
                  parser.error("Syntax error");
              }
            }
            else
              parser.error("Parameter type not found.");
          }
          else
            parser.error("Parameter consists of a type and a name.");

        const char *str = varExp->name.getString().c_str();
        char firstChar = str[0];
        bool handlerFlag = str[strlen(str) - 1] == '_';

        if (!handlerFlag && firstChar >= 'A' && firstChar <= 'Z') {
          char buffer[256] = "";
          char buf[256];

          if (!IS_VOID(returnType))
            sprintf(buffer, "%s _ret", returnType.toString());

          sprintf(buf, "void ReturnHandler_(%s);", buffer);

          Expr *handlerExpr = parse(buf, 0, 0, NULL);

          handlerExpr->resolve(parser);
        }

        getCurrent()->enclosing->checkDeclaration({VAL_OBJ, &function->obj}, varExp, function);
        function->bodyExpr = body;

        if (body)
          acceptGroupingExprUnits(body, parser);

        compiler.endScope();
      }
      return;
    }
    case EXPR_REFERENCE: {
      ReferenceExpr *varExpr = (ReferenceExpr *)subExpr;

      listType = subExpr->type;
      getCurrent()->checkDeclaration(returnType, varExpr, NULL);

      LiteralExpr *valueExpr = NULL;

      switch (returnType.valueType) {
      case VAL_VOID:
        parser.error("Cannot declare a void variable.");
        break;

      case VAL_INT:
        valueExpr = new LiteralExpr(VAL_INT, {.integer = 0});
        break;

      case VAL_BOOL:
        valueExpr = new LiteralExpr(VAL_BOOL, {.boolean = false});
        break;

      case VAL_FLOAT:
        valueExpr = new LiteralExpr(VAL_FLOAT, {.floating = 0.0});
        break;

      case VAL_OBJ:
        switch (AS_OBJ_TYPE(returnType)) {
        case OBJ_STRING:
          valueExpr = new LiteralExpr(VAL_OBJ, {.obj = &copyString("", 0)->obj});
          break;

        case OBJ_INTERNAL:
          valueExpr = new LiteralExpr(VAL_OBJ, {.obj = &newInternal()->obj});
          break;
        }
        break;
      }

      if (valueExpr) {
        expressions[1] = new AssignExpr(
            varExpr, buildToken(TOKEN_EQUAL, "=", 1, varExpr->name.line),
            valueExpr, OP_FALSE);
        listType = EXPR_ASSIGN;
      }

      if (count > 2)
        parser.error("Expect ';' or newline after variable declaration.");
      return;
    }
    default:
      break;
    }
  }
  else // parse array expression
    for (int index = 1; index < count; index++) {
      Expr *subExpr = expressions[index];

      subExpr->resolve(parser);

      if (IS_VOID(getCurrent()->peekDeclaration()))
        popType();
    }

  getCurrent()->pushType({VAL_VOID});
}

void LiteralExpr::resolve(Parser &parser) {
  if (type == VAL_OBJ)
    getCurrent()->pushType(stringType);
  else
    getCurrent()->pushType(type);
}

void LogicalExpr::resolve(Parser &parser) {
  left->resolve(parser);
  right->resolve(parser);

  Type type2 = popType();
  Type type1 = popType();

  if (!IS_BOOL(type1) || !IS_BOOL(type2))
    parser.error("Value must be boolean");

  getCurrent()->pushType(VAL_BOOL);
}

void OpcodeExpr::resolve(Parser &parser) {
  parser.compilerError("In OpcodeExpr!");
  right->resolve(parser);
}

static std::list<Expr *> uiExprs;
static std::list<Type> uiTypes;

void ReturnExpr::resolve(Parser &parser) {
  if (getCurrent()->function->isClass()) {
    char buf[128] = "{post(ReturnHandler_)}";

    if (value) {
      uiExprs.push_back(value);
      uiTypes.push_back({(ValueType)-1});
      strcpy(buf, "{void Ret_() {ReturnHandler_($EXPR)}; post(Ret_)}");
    }

    value = parse(buf, 0, 0, NULL);
  }

  // sync processing below

  if (value) {
    value->resolve(parser);

//  if (!getCurrent()->isClass())
//    Type type = removeDeclaration();
    // verify that type is the function return type if not an instance
    // else return void
  }

  getCurrent()->pushType(VAL_VOID);
}

void SetExpr::resolve(Parser &parser) {
  object->resolve(parser);
  value->resolve(parser);

  Type valueType = popType();
  Type objectType = popType();

  if (AS_OBJ_TYPE(objectType) != OBJ_INSTANCE)
    parser.errorAt(&name, "Only instances have properties.");
  else {
    ObjInstance *type = AS_INSTANCE_TYPE(objectType);

    for (int count = 0, i = 0; i < type->callable->compiler->declarationCount; i++) {
      Declaration *dec = &type->callable->compiler->declarations[i];

      if (dec->isField) {
        if (identifiersEqual(&name, &dec->name)) {
          index = count;
          getCurrent()->pushType(dec->type);
          return;
        }

        count++;
      }
    }

    parser.errorAt(&name, "Field '%.*s' not found.", name.length, name.start);
  }

  getCurrent()->pushType({VAL_VOID});
}

void StatementExpr::resolve(Parser &parser) {
  int oldStackSize = getCurrent()->typeStack.size();

  expr->resolve(parser);

  if (expr->type != EXPR_LIST || ((ListExpr *)expr)->listType == EXPR_LIST) {
    Type type = popType();

    if (oldStackSize != getCurrent()->typeStack.size())
      parser.compilerError("COMPILER ERROR: oldStackSize: %d stackSize: %d",
                           oldStackSize, getCurrent()->typeStack.size());

    if (!IS_VOID(type))
      expr = new OpcodeExpr(OP_POP, expr);

    getCurrent()->pushType(VAL_VOID);
  }
}

void SuperExpr::resolve(Parser &parser) {}

void TernaryExpr::resolve(Parser &parser) {
  left->resolve(parser);

  Type type = popType();

  if (IS_VOID(type))
    parser.error("Value must not be void");

  middle->resolve(parser);

  if (right) {
    right->resolve(parser);
    popType();
  }
}

void ThisExpr::resolve(Parser &parser) {}

void TypeExpr::resolve(Parser &parser) {
  //  getCurrent()->addDeclaration({VAL_OBJ, (ObjPrimitiveType) {{OBJ_PRIMITIVE_TYPE},
  //  type}});
}

void UnaryExpr::resolve(Parser &parser) {
  right->resolve(parser);

  Type type = popType();

  if (IS_VOID(type))
    parser.error("Value must not be void");

  switch (op.type) {
  case TOKEN_PRINT:
    right = convertToString(right, type, parser);
    getCurrent()->pushType(VAL_VOID);
    break;

  case TOKEN_NEW: {
    bool errorFlag = true;

    switch (AS_OBJ_TYPE(type)) {
    case OBJ_FUNCTION:
      errorFlag = false;
      break;
    }

    if (errorFlag)
      parser.error("Operator new must target a callable function");

    getCurrent()->pushType(type);
    break;
  }

  case TOKEN_PERCENT:
    right = convertToFloat(right, type, parser);
    getCurrent()->pushType({VAL_FLOAT});
    break;

  default:
    getCurrent()->pushType(type);
    break;
  }
}

void ReferenceExpr::resolve(Parser &parser) {
  if (index != -1) {
    getCurrent()->pushType({VAL_OBJ, primitives[index]});
    index = -1;
    _declaration = NULL;
  }
  else
    getCurrent()->resolveReferenceExpr(this);
}

void SwapExpr::resolve(Parser &parser) {
  Type type = uiTypes.front();

  _expr = uiExprs.front();
  uiExprs.pop_front();
  uiTypes.pop_front();

  if (type.valueType < VAL_VOID)
    _expr->resolve(parser);
  else
    getCurrent()->pushType(type);
}

void NativeExpr::resolve(Parser &parser) {
  getCurrent()->pushType(VAL_VOID);
}

Type popType() {
  return getCurrent()->popType();
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

Expr *parse(const char *source, int index, int replace, Expr *body) {
  Scanner scanner((new std::string(source))->c_str());
  Parser parser(scanner);
  GroupingExpr *group = (GroupingExpr *) parser.parse() ? (GroupingExpr *) parser.expr : NULL;

  if (body == NULL)
    body = group;
  else
    if (group) {
      if (replace || group->count) {
        if (body->type != EXPR_GROUPING) {
          Expr **newExprList = RESIZE_ARRAY(Expr *, NULL, 0, 1);

          newExprList[0] = body;
          body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 1, newExprList, 0, NULL);
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

  return body;
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
    expr->previous->resolve(parser);

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    expr->lastChild->resolve(parser);
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
        attExpr->handler->resolve(parser);

        Type type = popType();

        if (!IS_VOID(type)) {
          if (attExpr->_uiIndex == ATTRIBUTE_OUT)
            switch (AS_OBJ_TYPE(type)) {
              case OBJ_FUNCTION:
                break;

              case OBJ_INSTANCE:
                getCurrent()->function->instanceIndexes->set(getCurrent()->vCount);
                expr->_eventFlags |= ((ObjFunction *) AS_INSTANCE_TYPE(type)->callable)->uiFunction->eventFlags;
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
    expr->previous->resolve(parser);

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
    expr->lastChild->resolve(parser);

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
      Type outType = getCurrent()->enclosing->getDeclaration(getCurrent()->enclosing->resolveReference2(&token)).type;
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
    expr->previous->resolve(parser);

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    expr->lastChild->resolve(parser);
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
    expr->previous->resolve(parser);

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
    Type outType = getCurrent()->enclosing->enclosing->getDeclaration(getCurrent()->enclosing->enclosing->resolveReference2(&token)).type;
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
    expr->lastChild->resolve(parser);
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
      expr->lastChild->resolve(parser);
      parent = oldParent;
    }
    else {
      const char *name = getValueVariableName(expr, ATTRIBUTE_OUT);
      Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
      int index = getCurrent()->enclosing->enclosing->resolveReference2(&token);
      Type outType = getCurrent()->enclosing->enclosing->getDeclaration(index).type;

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

    expr->previous->resolve(parser);

    if (expr->_eventFlags) {
      --nTabs;
      insertTabs();
      (*ss) << "}\n";
    }
  }
}
