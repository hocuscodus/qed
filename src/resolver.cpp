/*
 * The QED Programming Language
 * Copyright (C) 2022  Hocus Codus Software inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http: *www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string.h>
#include <list>
#include <set>
#include <sstream>
#include "resolver.hpp"
#include "memory.h"
#include "qni.hpp"

typedef void (Resolver::*DirectiveFn)(UIDirectiveExpr *expr);

#define UI_PARSE_DEF( identifier, directiveFn )  { directiveFn }
DirectiveFn directiveFns[] = { UI_PARSES_DEF };
#undef UI_PARSE_DEF

extern ParseExpRule expRules[];

/*
#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif
*/
static Compiler *current = NULL;

static Obj objInternalType = {OBJ_INTERNAL};
static Obj *primitives[] = {
  &newPrimitive("void", {VAL_VOID})->obj,
  &newPrimitive("bool", {VAL_BOOL})->obj,
  &newPrimitive("int", {VAL_INT})->obj,
  &newPrimitive("float", {VAL_FLOAT})->obj,
  &newPrimitive("String", stringType)->obj,
  &newPrimitive("var", {VAL_OBJ, &objInternalType})->obj,
};

static bool identifiersEqual2(Token *a, ObjString *b) {
  return a->length != 0 && a->length == b->length &&
         memcmp(a->start, b->chars, a->length) == 0;
}

static bool isType(Type &type) {
  if (type.valueType == VAL_OBJ)
    switch (type.objType->type) {
    case OBJ_FUNCTION: {
      ObjString *name = ((ObjNamed *)type.objType)->name;

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
  return type.objType->type == OBJ_PRIMITIVE ? ((ObjPrimitive *)type.objType)->type : type;
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

static void resolveVariableExpr(VariableExpr *expr) {
  expr->index = current->resolveLocal(&expr->name);

  switch (expr->index) {
  case -2:
    expr->index = -1;
    current->addLocal(VAL_VOID);
    break;

  case -1:
    if ((expr->index = current->resolveUpvalue(&expr->name)) != -1) {
      expr->upvalueFlag = true;
      current->addLocal(current->function->upvalues[expr->index].type);
    }
    else {
      current->parser.error("Variable '%.*s' must be defined", expr->name.length, expr->name.start);
      current->addLocal(VAL_VOID);
    }
    break;

  default:
    current->addLocal(current->locals[expr->index].type);
    break;
  }
}

Resolver::Resolver(Parser &parser, Expr *exp) : ExprVisitor(), parser(parser) {
  this->exp = exp;
  uiParseCount = -1;
}

static OpCode getOpCode(Type type, Token token) {
  OpCode opCode = OP_FALSE;

  switch (token.type) {
  case TOKEN_PLUS:
  case TOKEN_PLUS_PLUS:
  case TOKEN_PLUS_EQUAL:
    opCode = type.valueType == VAL_OBJ   ? OP_ADD_STRING
                    : type.valueType == VAL_INT ? OP_ADD_INT
                                                : OP_ADD_FLOAT;
    break;
  case TOKEN_MINUS:
  case TOKEN_MINUS_MINUS:
  case TOKEN_MINUS_EQUAL:
    opCode = type.valueType == VAL_INT ? OP_SUBTRACT_INT : OP_SUBTRACT_FLOAT;
    break;
  case TOKEN_STAR:
  case TOKEN_STAR_EQUAL:
    opCode = type.valueType == VAL_INT ? OP_MULTIPLY_INT : OP_MULTIPLY_FLOAT;
    break;
  case TOKEN_SLASH:
  case TOKEN_SLASH_EQUAL:
    opCode = type.valueType == VAL_INT ? OP_DIVIDE_INT : OP_DIVIDE_FLOAT;
    break;
  case TOKEN_BANG_EQUAL:
  case TOKEN_EQUAL_EQUAL:
    opCode = type.valueType == VAL_OBJ   ? OP_EQUAL_STRING
                    : type.valueType == VAL_INT ? OP_EQUAL_INT
                                                : OP_EQUAL_FLOAT;
    break;
  case TOKEN_GREATER:
  case TOKEN_LESS_EQUAL:
    opCode = (type.valueType == VAL_OBJ   ? OP_GREATER_STRING
                    : type.valueType == VAL_INT ? OP_GREATER_INT
                                                : OP_GREATER_FLOAT);
    break;
  case TOKEN_GREATER_EQUAL:
  case TOKEN_LESS:
    opCode = (type.valueType == VAL_OBJ   ? OP_LESS_STRING
                    : type.valueType == VAL_INT ? OP_LESS_INT
                                                : OP_LESS_FLOAT);
    break;
  case TOKEN_OR:
  case TOKEN_OR_EQUAL:
    opCode = type.valueType == VAL_INT ? OP_BITWISE_OR : OP_LOGICAL_OR;
    break;
  case TOKEN_OR_OR:
    opCode = OP_LOGICAL_OR;
    break;
  case TOKEN_AND:
  case TOKEN_AND_EQUAL:
    opCode = type.valueType == VAL_INT ? OP_BITWISE_AND : OP_LOGICAL_AND;
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

void Resolver::visitAssignExpr(AssignExpr *expr) {
  //  accept<int>(expr->index, 0);
  if (expr->value)
    accept<int>(expr->value, 0);

  accept<int>(expr->varExp, 0);

  Type type1 = removeLocal();
  Type type2 = expr->value ? removeLocal() : type1;

  expr->opCode = getOpCode(type1, expr->op);

  if (type1.valueType == VAL_VOID)
    parser.error("Variable not found");
  else if (!type1.equals(type2)) {
    expr->value = convertToType(type2, expr->value, type1, parser);

    if (expr->value == NULL) {
      parser.error("Value must match the variable type");
    }
  }

  current->addLocal(type2);
}

void Resolver::visitUIAttributeExpr(UIAttributeExpr *expr) {
  parser.error("Internal error, cannot be invoked..");
}

void Resolver::visitUIDirectiveExpr(UIDirectiveExpr *expr) {
  (this->*directiveFns[getParseStep()])(expr);
}
#if 0
void Resolver::visitDirectiveExpr(UIDirectiveExpr *expr) {
  ParseStep step = getParseStep();

  if (resolverUIRules[step].attrListFn)
    (this->*resolverUIRules[step].attrListFn)(expr);
////////////////////
/*  if (outFlag)
    for (int index = 0; index < expr->attCount; index++)
      accept<int>(expr->attributes[index], 0);
  else
    for (int index = 0; index < expr->attCount; index++)
      if (expr->attributes[index]->_index != -1)
        valueStack.push(expr->attributes[index]->name.getString(), expr->attributes[index]->_index);

  int numAttrSets = expr->childrenCount;

  if (!numAttrSets) {
    if (!outFlag) {
      int index = valueStack.get("out");

      if (index != -1) {
        Type &type = current->enclosing->locals[index].type;

        if (type.valueType == VAL_OBJ)
          switch (type.objType->type) {
          case OBJ_INSTANCE:
            expr->viewIndex = current->localCount;
            current->addLocal({VAL_INT});
            break;

          case OBJ_STRING:
            expr->viewIndex = current->localCount;
            current->addLocal({VAL_OBJ, &newInternal()->obj});
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
      accept<int>(expr->children[index], 0);

  //  expr->attrSets = numAttrSets != 0 ? new ChildAttrSets(&offset, numZones, 0, arrayDirFlags, valueStack, expr, 0) : NULL;
  //  function->attrSets = expr->attrSets;

  //  if (expr->attrSets != NULL) {
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
        expr->attrSets->parseCreateSizers(topSizers, dir, zone, Path(), Path());
        expr->attrSets->parseAdjustPaths(topSizers, dir);
        numZones[dir] = zone != NULL ? zone[0] + 1 : 1;
      }
  * /
  //    subAttrsets = parent is ImplicitArrayDeclaration ? parseCreateSubSets(topSizers, new Path(), numZones) : createIntersection(-1, topSizers);
  //    subAttrsets = createIntersection(-1, topSizers);
  //  }

  if (!outFlag)
    for (int index = 0; index < expr->attCount; index++)
      if (expr->attributes[index]->_index != -1)
        valueStack.pop(expr->attributes[index]->name.getString());*/
}
#endif
void Resolver::visitBinaryExpr(BinaryExpr *expr) {
  //  if (expr->left) {
  accept<int>(expr->left, 0);
  //    expr->type = expr->left->type;
  //  } else
  //    expr->type = {VAL_VOID};

  //  if (expr->right)
  accept<int>(expr->right, 0);

  Type type2 = removeLocal();
  Type type1 = removeLocal();

  Type type = type1;
  bool boolVal = false;

  expr->opCode = getOpCode(type, expr->op);

  switch (expr->op.type) {
  case TOKEN_BANG_EQUAL:
  case TOKEN_GREATER_EQUAL:
  case TOKEN_LESS_EQUAL:
    expr->notFlag = true;
    break;
  }

  switch (expr->op.type) {
  case TOKEN_PLUS:
    if (type1.valueType == VAL_OBJ) {
      expr->right = convertToString(expr->right, type2, parser);
      current->addLocal(stringType);
      return;
    }
    // no break statement, fall through

  case TOKEN_BANG_EQUAL:
  case TOKEN_EQUAL_EQUAL:
  case TOKEN_GREATER:
  case TOKEN_GREATER_EQUAL:
  case TOKEN_LESS:
  case TOKEN_LESS_EQUAL:
    boolVal = expr->op.type != TOKEN_PLUS;
    if (type1.valueType == VAL_OBJ) {
      if (type2.valueType != VAL_OBJ)
        parser.error("Second operand must be a string");

      current->addLocal(VAL_BOOL);
      return;
    }
    // no break statement, fall through

  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH: {
    switch (type1.valueType) {
    case VAL_INT:
      if (!type1.equals(type2)) {
        if (type2.valueType != VAL_FLOAT) {
          current->addLocal(VAL_VOID);
          parser.error("Second operand must be numeric");
          return;
        }

        expr->left = convertToFloat(expr->left, type1, parser);
        expr->opCode =
            (OpCode)((int)expr->opCode + (int)OP_ADD_FLOAT - (int)OP_ADD_INT);
        current->addLocal(boolVal ? VAL_BOOL : VAL_FLOAT);
      }
      else
        current->addLocal(boolVal ? VAL_BOOL : VAL_INT);
      break;

    case VAL_FLOAT:
      if (!type1.equals(type2)) {
        if (type2.valueType != VAL_INT) {
          current->addLocal(VAL_VOID);
          parser.error("Second operand must be numeric");
          return;
        }

        expr->right = convertToFloat(expr->right, type2, parser);
      }

      current->addLocal(boolVal ? VAL_BOOL : VAL_FLOAT);
      break;

    default:
      parser.error("First operand must be numeric");
      current->addLocal(type1);
      break;
    }
  }
  break;

  case TOKEN_XOR:
    current->addLocal(type1.valueType == VAL_BOOL ? VAL_BOOL : VAL_FLOAT);
    break;

  case TOKEN_OR:
  case TOKEN_AND:
  case TOKEN_GREATER_GREATER_GREATER:
  case TOKEN_GREATER_GREATER:
  case TOKEN_LESS_LESS:
    current->addLocal(VAL_INT);
    break;

  case TOKEN_OR_OR:
  case TOKEN_AND_AND:
    current->addLocal(VAL_BOOL);
    break;

    break;

  default:
    return; // Unreachable.
  }
}

static Expr *generateUIFunction(const char *type, const char *name, char *args, Expr *uiExpr, int count, int restLength, Expr **rest) {
    VariableExpr *nameExpr = new VariableExpr(buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), -1, false);
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
      bodyExprs[index] = uiExpr;

    for (int index = 0; index < restLength; index++)
      bodyExprs[count + index] = rest[index];

    int typeIndex = -1;

    for (int index = 0; typeIndex == -1 && index < sizeof(primitives) / sizeof(Obj *); index++)
      if (!strcmp(((ObjPrimitive *) primitives[index])->name->chars, type))
        typeIndex = index;

    functionExprs[0] = new VariableExpr(buildToken(TOKEN_IDENTIFIER, type, strlen(type), -1), typeIndex, false);
    functionExprs[1] = new CallExpr(nameExpr, buildToken(TOKEN_RIGHT_PAREN, ")", 1, -1), nbParms, parms, false, NULL);
    functionExprs[2] = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), count + restLength, bodyExprs, 0, NULL);

    return new ListExpr(3, functionExprs, EXPR_LIST, NULL);
}

void Resolver::visitCallExpr(CallExpr *expr) {
  Field fields[expr->count];
  ObjCallable signature;

  signature.arity = expr->count;
  signature.fields = fields;

  for (int index = 0; index < expr->count; index++) {
    accept<int>(expr->arguments[index]);
    fields[index].type = removeLocal();
  }

  pushSignature(&signature);
  accept<int>(expr->callee);
  popSignature();

  Type type = removeLocal();
  //  Local *callerLocal = current->peekLocal(expr->argCount);

  if (type.valueType == VAL_OBJ) {
    switch (type.objType->type) {
    case OBJ_FUNCTION: {
      ObjCallable *callable = (ObjCallable *)type.objType;

      if (expr->newFlag) {
        if (expr->handler != NULL) {
          char buffer[256] = "";

          if (callable->type.valueType != VAL_VOID)
            sprintf(buffer, "%s _ret", callable->type.toString());

          expr->handler = generateUIFunction("void", "ReturnHandler_", callable->type.valueType != VAL_VOID ? buffer : NULL, expr->handler, 1, 0, NULL);
          accept<int>(expr->handler);
          removeLocal();
        }

        current->addLocal((Type) {VAL_OBJ, &newInstance(callable)->obj});
      }
      else
        current->addLocal(callable->type);
      break;
    }
    case OBJ_FUNCTION_PTR: {
      ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;

      if (expr->newFlag) {
        if (expr->handler != NULL) {
          char buffer[256] = "";

          if (callable->type.valueType != VAL_VOID)
            sprintf(buffer, "%s _ret", callable->type.toString());

          expr->handler = generateUIFunction("void", "ReturnHandler_", callable->type.valueType != VAL_VOID ? buffer : NULL, expr->handler, 1, 0, NULL);
          accept<int>(expr->handler);
          removeLocal();
        }

;//        current->addLocal((Type) {VAL_OBJ, &newInstance(callable)->obj});
      }
      else
        current->addLocal(callable->type.valueType);
      break;
    }
    default:
      parser.error("Non-callable object type");
      current->addLocal(VAL_VOID);
      break; // Non-callable object type.
    }
  }
  else {
    parser.error("Non-callable object type");
    current->addLocal(VAL_VOID);
    //    return INTERPRET_RUNTIME_ERROR;
  }
}

void Resolver::visitArrayElementExpr(ArrayElementExpr *expr) {
  accept<int>(expr->callee);

  Type type = removeLocal();

  if (!expr->count)
    if (isType(type)) {
      Type returnType = convertType(type);
      ObjArray *array = newArray();

      array->elementType = returnType;
      current->addLocal({VAL_OBJ, &array->obj});
    }
    else {
      parser.error("No index defined.");
      current->addLocal(VAL_VOID);
    }
  else
    if (isType(type)) {
      parser.error("A type cannot have an index.");
      current->addLocal(VAL_VOID);
    }
    else
      if (type.valueType == VAL_OBJ) {
        switch (type.objType->type) {
        case OBJ_ARRAY: {
          ObjArray *array = (ObjArray *)type.objType;

          current->addLocal(array->elementType);

          for (int index = 0; index < expr->count; index++) {
            accept<int>(expr->indexes[index]);

            Type argType = removeLocal();

            argType = argType;
          }
          break;
        }
        case OBJ_STRING: {/*
          ObjString *string = (ObjString *)type.objType;

          if (expr->count != string->arity)
            parser.error("Expected %d arguments but got %d.", string->arity, expr->count);

          current->addLocal(string->type.valueType);

          for (int index = 0; index < expr->count; index++) {
            accept<int>(expr->indexes[index]);
            Type argType = removeLocal();

            argType = argType;
          }*/
          break;
        }
        default:
          parser.error("Non-indexable object type");
          current->addLocal(VAL_VOID);
          break; // Non-indexable object type.
        }
      }
      else {
        parser.error("Non-indexable object type");
        //    return INTERPRET_RUNTIME_ERROR;
      }
}

void Resolver::visitDeclarationExpr(DeclarationExpr *expr) {
  checkDeclaration(&expr->name);

  if (expr->initExpr != NULL) {
    accept<int>(expr->initExpr, 0);
    Type type1 = removeLocal();
  }

  current->addLocal(expr->type);
  current->setLocalName(&expr->name);
}

void Resolver::visitFunctionExpr(FunctionExpr *expr) {
  checkDeclaration(&expr->name);
  current->addLocal(VAL_OBJ);
  current->setLocalName(&expr->name);

  Compiler compiler(parser, current);

  compiler.function->type = expr->type;
  compiler.function->name = copyString(expr->name.start, expr->name.length);
  compiler.function->arity = expr->count;
  compiler.beginScope();
  current = &compiler;

  for (int index = 0; index < expr->count; index++)
    accept<int>(expr->params[index]);

  accept<int>(expr->body, 0);
  compiler.function->fieldCount = compiler.localCount;
  compiler.function->fields = ALLOCATE(Field, compiler.localCount);

  for (int index = 0; index < compiler.localCount; index++) {
    Local *local = &compiler.locals[index];

    compiler.function->fields[index].type = local->type;
    compiler.function->fields[index].name = copyString(local->name.start, local->name.length);
  }

  current = current->enclosing;
  current->setLocalObjType(compiler.function);
  expr->function = compiler.function;
}

void Resolver::visitGetExpr(GetExpr *expr) {
  accept<int>(expr->object);

  Type objectType = removeLocal();

  if (objectType.valueType != VAL_OBJ || objectType.objType->type != OBJ_INSTANCE)
    parser.errorAt(&expr->name, "Only instances have properties.");
  else {
    ObjFunction *type = (ObjFunction *)objectType.objType;

    for (int i = 0; expr->index == -1 && i < type->fieldCount; i++) {
      Field *field = &type->fields[i];

      if (identifiersEqual2(&expr->name, field->name)) // && local->depth != -1)
        expr->index = i;
    }

    if (expr->index == -1)
      parser.errorAt(&expr->name, "Field '%.*s' not found.", expr->name.length,
                     expr->name.start);
    else {
      current->addLocal(type->fields[expr->index].type);
      return;
    }
  }

  current->addLocal({VAL_VOID});
}

void Resolver::visitGroupingExpr(GroupingExpr *expr) {
  TokenType type = expr->name.type;
  bool parenFlag = type == TOKEN_RIGHT_PAREN;
  bool groupFlag = type == TOKEN_RIGHT_BRACE || parenFlag;

  if (groupFlag)
    current->beginScope();

  acceptGroupingExprUnits(expr);

  Type parenType = parenFlag ? removeLocal() : (Type){VAL_VOID};

  if (groupFlag)
    expr->popLevels = current->endScope();

  if (type != TOKEN_EOF)
    current->addLocal(parenType);
}

void Resolver::visitArrayExpr(ArrayExpr *expr) {
  ObjArray *objArray = newArray();
  Type type = {VAL_OBJ, &objArray->obj};
  Compiler compiler(parser, current);

  compiler.beginScope();
  current = &compiler;

  for (int index = 0; index < expr->count; index++) {
    acceptSubExpr(expr->expressions[index]);

    Type subType = removeLocal();

    objArray->elementType = subType;
  }

  current = current->enclosing;
  compiler.function->type = type;
  current->setLocalObjType(compiler.function);
  expr->function = compiler.function;
  current->addLocal(type);
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
void Resolver::visitListExpr(ListExpr *expr) {
  accept<int>(expr->expressions[0], 0);

  Type type = removeLocal();

  if (isType(type)) {
    Type returnType = convertType(type);
    Expr *subExpr = expr->expressions[1];

    switch (subExpr->type) {
    case EXPR_ASSIGN: {
      AssignExpr *assignExpr = (AssignExpr *)subExpr;

      expr->listType = subExpr->type;
      checkDeclaration(&assignExpr->varExp->name);

      if (assignExpr->value != NULL) {
        accept<int>(assignExpr->value, 0);
        Type internalType = {VAL_OBJ, &objInternalType};
        Type type1 = removeLocal();

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
      }

      current->addLocal(returnType);
      current->setLocalName(&assignExpr->varExp->name);

      if (expr->count > 2)
        parser.error("Expect ';' or newline after variable declaration.");
      return;
    }
    case EXPR_CALL: {
      expr->listType = subExpr->type;

      CallExpr *callExpr = (CallExpr *)subExpr;

      if (callExpr->newFlag)
        parser.error("Syntax error: new.");
      else if (callExpr->callee->type != EXPR_VARIABLE)
        parser.error("Function name must be a string.");
      else {
        VariableExpr *varExp = (VariableExpr *)callExpr->callee;

        checkDeclaration(&varExp->name);
        current->addLocal(VAL_OBJ);
        current->setLocalName(&varExp->name);

        Compiler compiler(parser, current);

        compiler.function->type = returnType;
        compiler.function->name = copyString(varExp->name.start, varExp->name.length);
        compiler.function->arity = callExpr->count;
        compiler.beginScope();
        current = &compiler;

        bindFunction(compiler.prefix, compiler.function);

        for (int index = 0; index < callExpr->count; index++)
          if (callExpr->arguments[index]->type == EXPR_LIST) {
            ListExpr *param = (ListExpr *)callExpr->arguments[index];
            Expr *paramExpr = param->expressions[0];

            accept<int>(paramExpr, 0);

            Type type = removeLocal();

            if (isType(type)) {
              paramExpr = param->expressions[1];

              if (paramExpr->type != EXPR_VARIABLE)
                parser.error("Parameter name must be a string.");
              else {
                current->addLocal(convertType(type));
                current->setLocalName(&((VariableExpr *)paramExpr)->name);

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

          if (returnType.valueType != VAL_VOID)
            sprintf(buffer, "%s _ret", returnType.toString());

          sprintf(buf, "void ReturnHandler_(%s);", buffer);

          Expr *handlerExpr = parse(buf, 0, 0, NULL);

          accept<int>(handlerExpr, 0);
        }

        compiler.function->bodyExpr = expr->count > 2 ? expr->expressions[2] : NULL;

        if (compiler.function->bodyExpr)
          if (compiler.function->bodyExpr->type == EXPR_GROUPING && ((GroupingExpr *)compiler.function->bodyExpr)->name.type == TOKEN_RIGHT_BRACE)
            acceptGroupingExprUnits((GroupingExpr *)compiler.function->bodyExpr);
          else {
            accept<int>(compiler.function->bodyExpr, 0);
            removeLocal();
          }

        compiler.function->fieldCount = compiler.localCount;
        compiler.function->fields = ALLOCATE(Field, compiler.localCount);

        for (int index = 0; index < compiler.localCount; index++) {
          Local *local = &compiler.locals[index];

          compiler.function->fields[index].type = local->type;
          compiler.function->fields[index].name = copyString(local->name.start, local->name.length);
        }

        current = current->enclosing;
        current->setLocalObjType(compiler.function);
        expr->function = compiler.function;
      }
      return;
    }
    case EXPR_VARIABLE: {
      VariableExpr *varExpr = (VariableExpr *)subExpr;
/*
      resolveVariableExpr(varExpr);
      if (expr->index != -1 && !expr->upvalueFlag)// && current->locals[expr->index].depth == current->scopeDepth)
        if (current->locals[expr->index].depth == current->scopeDepth)
        parser.error("Already a variable with this name in this scope.");*/
      checkDeclaration(&varExpr->name);
      current->addLocal(returnType);
      current->setLocalName(&varExpr->name);

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
        switch (returnType.objType->type) {
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
        expr->expressions[1] = new AssignExpr(
            varExpr, buildToken(TOKEN_EQUAL, "=", 1, varExpr->name.line),
            valueExpr, OP_FALSE, false);
        expr->listType = EXPR_ASSIGN;
      }

      if (expr->count > 2)
        parser.error("Expect ';' or newline after variable declaration.");
      return;
    }
    default:
      break;
    }
  }
  else // parse array expression
    for (int index = 0; index < expr->count; index++) {
      Expr *subExpr = expr->expressions[index];

      accept<int>(subExpr, 0);

      if (current->peekLocal(0)->type.valueType == VAL_VOID)
        removeLocal();
    }

  current->addLocal({VAL_VOID});
}

void Resolver::visitLiteralExpr(LiteralExpr *expr) {
  if (expr->type == VAL_OBJ)
    current->addLocal(stringType);
  else
    current->addLocal(expr->type);
}

void Resolver::visitLogicalExpr(LogicalExpr *expr) {
  accept(expr->left, 0);
  accept(expr->right, 0);

  Type type2 = removeLocal();
  Type type1 = removeLocal();

  if (type1.valueType != VAL_BOOL || type2.valueType != VAL_BOOL)
    parser.error("Value must be boolean");

  current->addLocal(VAL_BOOL);
}

void Resolver::visitOpcodeExpr(OpcodeExpr *expr) {
  parser.compilerError("In OpcodeExpr!");
  expr->right->accept(this);
}

static std::list<Expr *> uiExprs;

void Resolver::visitReturnExpr(ReturnExpr *expr) {
//  if (isInstance) {
//    char buf[128] = "{void Ret_() {ReturnHandler_()}; post(Ret_)}";
    char buf[128] = "{post(ReturnHandler_)}";

    if (expr->value) {
      uiExprs.push_back(expr->value);
      strcpy(buf, "{void Ret_() {ReturnHandler_($EXPR)}; post(Ret_)}");
    }

    expr->value = parse(buf, 0, 0, NULL);
//  }

  // sync processing below

  if (expr->value) {
    accept<int>(expr->value, 0);

//    Type type = removeLocal();
    // verify that type is the function return type if not an instance
    // else return void
  }

  current->addLocal(VAL_VOID);
}

void Resolver::visitSetExpr(SetExpr *expr) {
  accept<int>(expr->object);
  accept<int>(expr->value);

  Type valueType = removeLocal();
  Type objectType = removeLocal();

  if (objectType.valueType != VAL_OBJ || objectType.objType->type != OBJ_INSTANCE)
    parser.errorAt(&expr->name, "Only instances have properties.");
  else {
    ObjFunction *type = (ObjFunction *)objectType.objType;

    for (int i = 0; expr->index == -1 && i < type->fieldCount; i++) {
      Field *field = &type->fields[i];

      if (identifiersEqual2(&expr->name, field->name)) // && local->depth != -1)
        expr->index = i;
    }

    if (expr->index == -1)
      parser.errorAt(&expr->name, "Field '%.*s' not found.", expr->name.length,
                     expr->name.start);
    else {
      current->addLocal(type->fields[expr->index].type);
      return;
    }
  }

  current->addLocal({VAL_VOID});
}

void Resolver::visitStatementExpr(StatementExpr *expr) {
  int oldLocalCount = current->localCount;

  acceptSubExpr(expr->expr);

  if (expr->expr->type != EXPR_LIST || ((ListExpr *)expr->expr)->listType == EXPR_LIST) {
    Type type = removeLocal();

    if (oldLocalCount != current->localCount)
      parser.compilerError("COMPILER ERROR: oldLocalCount: %d localCount: %d",
                           oldLocalCount, current->localCount);

    if (type.valueType != VAL_VOID)
      expr->expr = new OpcodeExpr(OP_POP, expr->expr);

    current->addLocal(VAL_VOID);
  }
}

void Resolver::visitSuperExpr(SuperExpr *expr) {}

void Resolver::visitTernaryExpr(TernaryExpr *expr) {
  expr->left->accept(this);

  Type type = removeLocal();

  if (type.valueType == VAL_VOID)
    parser.error("Value must not be void");

  expr->middle->accept(this);

  if (expr->right) {
    expr->right->accept(this);
    removeLocal();
  }
}

void Resolver::visitThisExpr(ThisExpr *expr) {}

void Resolver::visitTypeExpr(TypeExpr *expr) {
  //  current->addLocal({VAL_OBJ, (ObjPrimitiveType) {{OBJ_PRIMITIVE_TYPE},
  //  expr->type}});
}

void Resolver::visitUnaryExpr(UnaryExpr *expr) {
  accept<int>(expr->right, 0);

  Type type = removeLocal();

  if (type.valueType == VAL_VOID)
    parser.error("Value must not be void");

  switch (expr->op.type) {
  case TOKEN_PRINT:
    expr->right = convertToString(expr->right, type, parser);
    current->addLocal(VAL_VOID);
    break;

  case TOKEN_NEW: {
    bool errorFlag = true;

    if (type.valueType == VAL_OBJ)
      switch (type.objType->type) {
      case OBJ_FUNCTION:
        errorFlag = false;
        break;
      }

    if (errorFlag)
      parser.error("Operator new must target a callable function");

    current->localCount++;
    break;
  }

  case TOKEN_PERCENT:
    expr->right = convertToFloat(expr->right, type, parser);
    current->addLocal({VAL_FLOAT});
    break;

  default:
    current->addLocal(type);
    break;
  }
}

void Resolver::visitVariableExpr(VariableExpr *expr) {
  if (expr->index != -1)
    current->addLocal({VAL_OBJ, primitives[expr->index]});
  else
    resolveVariableExpr(expr);
}

void Resolver::visitSwapExpr(SwapExpr *expr) {
  expr->_expr = uiExprs.front();
  uiExprs.pop_front();
  accept<int>(expr->_expr);
}

void Resolver::checkDeclaration(Token *name) {
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];

    if (local->depth != -1 && local->depth < current->scopeDepth)
      break;

    if (identifiersEqual(name, &local->name))
;//      parser.error("Already a variable '%.*s' with this name in this scope.", name->length, name->start);
  }
}

Type Resolver::removeLocal() {
  Type type = current->removeLocal();
  /*
    if (type.valueType == VAL_VOID && type.objType != NULL) {
      VariableExpr *variableExpr = (VariableExpr *) type.objType;
      int index = current->resolveLocal(&variableExpr->name);

      if (index != (int8_t) -128) {
        Local *local = &current->locals[current->localCount + index];

        return local->type;
      }
      else if ((index = current->resolveUpvalue(&variableExpr->name)) != -1)
        return {VAL_VOID}; // TODO: change this...
      else {
        parser.error("Variable must be defined");
        return {VAL_VOID};
      }
    }
  */
  return type; /*
   if (type == VAL_VOID)
   accept<int>(expr->right, 0);
     expr->index = current->resolveLocal(&previous);

     if (expr->index != (int8_t) -128) {
       Local *local = &current->locals[current->localCount + expr->index];

       current->addLocal(local->type, local->objType);
     } else if ((expr->index = current->resolveUpvalue(&expr->name)) != -1)
       expr->upvalueFlag = true;
     else {
       parser.error("Variable must be defined");
       current->addLocal(VAL_VOID);
     }
 */
}

bool Resolver::resolve(Compiler *compiler) {
  Compiler *oldCurrent = current;

  current = compiler;
  accept(exp, 0);
  current = oldCurrent;
  return !parser.hadError;
}

int aCount;
std::stringstream s;
std::stringstream *ss = &s;

int nTabs = 0;

static void insertTabs() {
  for (int index = 0; index < nTabs; index++)
    (*ss) << "  ";
}

static const char *getGroupName(UIDirectiveExpr *expr, int dir);

void Resolver::acceptGroupingExprUnits(GroupingExpr *expr) {
  TokenType type = expr->name.type;
  bool parenFlag = type == TOKEN_RIGHT_PAREN;

  for (int index = 0; index < expr->count; index++) {
    Expr *subExpr = expr->expressions[index];

    if (subExpr->type == EXPR_UIDIRECTIVE) {
      if (++uiParseCount >= 1)
        ss->str("");

      if (getParseStep() == PARSE_EVENTS) {
        insertTabs();
        (*ss) << "bool flag = false\n";
      }
    }

    acceptSubExpr(subExpr);

    if (subExpr->type == EXPR_UIDIRECTIVE) {
      UIDirectiveExpr *exprUI = (UIDirectiveExpr *) subExpr;

      if (uiParseCount) {
        if (uiParseCount == 3) {
          nTabs++;
          insertTabs();
          (*ss) << "var size = (" << getGroupName(exprUI, 0) << " << 16) | " << getGroupName(exprUI, 1) << "\n";
          nTabs--;
        }
#ifdef DEBUG_PRINT_CODE
        printf("CODE %d\n%s", uiParseCount, ss->str().c_str());
#endif
        expr = (GroupingExpr *) parse(ss->str().c_str(), index, 1, expr);
      }
      else {
        current->function->eventFlags = exprUI->_eventFlags;
        expr->expressions = RESIZE_ARRAY(Expr *, expr->expressions, expr->count, expr->count + uiExprs.size() - 1);
        memmove(&expr->expressions[index + uiExprs.size()], &expr->expressions[index + 1], (--expr->count - index) * sizeof(Expr *));

        for (Expr *uiExpr : uiExprs)
          expr->expressions[index++] = uiExpr;

        expr->count += uiExprs.size();
        uiExprs.clear();
      }

      index--;
    }
    else
      if (current->peekLocal(0)->type.valueType == VAL_VOID && (!parenFlag || index < expr->count - 1))
        removeLocal();
  }

  if (expr->ui != NULL) {
    UIDirectiveExpr *exprUI = (UIDirectiveExpr *) expr->ui;

    if (exprUI->previous || exprUI->lastChild) {
      // Perform the UI AST magic
      Expr *clickFunction = generateUIFunction("void", "onEvent", "int event, int pos0, int pos1, int size0, int size1", expr->ui, 1, 0, NULL);
      Expr *paintFunction = generateUIFunction("void", "paint", "int pos0, int pos1, int size0, int size1", expr->ui, 1, 0, NULL);
      Expr **uiFunctions = new Expr *[2];

      uiFunctions[0] = paintFunction;
      uiFunctions[1] = clickFunction;

      Expr *layoutFunction = generateUIFunction("void", "Layout_", NULL, expr->ui, 3, 2, uiFunctions);
      Expr **layoutExprs = new Expr *[1];

      layoutExprs[0] = layoutFunction;

      Expr *valueFunction = generateUIFunction("void", "UI_", NULL, expr->ui, 1, 1, layoutExprs);

      aCount = 1;
      accept<int>(valueFunction, 0);
      Type type = removeLocal();
      current->function->uiFunction = type.valueType == VAL_OBJ ? (ObjFunction *) type.objType : NULL;
      delete expr->ui;
      expr->ui = valueFunction;
      uiParseCount = -1;
    }
    else {
      delete expr->ui;
      expr->ui = NULL;
    }
  }
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
*/
void Resolver::acceptSubExpr(Expr *expr) {
  accept<int>(expr, 0);
/*
  if (expr->type == EXPR_VARIABLE) {
    Type type = removeLocal();

    if (type.valueType == VAL_OBJ && type.objType->type == OBJ_FUNCTION) {
      ObjFunction *function = (ObjFunction *) type.objType;
      Type parms[function->arity];

      for (int index = 0; index < function->arity; index++)
        parms[index] = function->fields[index].type;

      type.objType = &newFunctionPtr(function->type, function->arity, parms)->obj;
    }

    current->addLocal(type);
  }*/
}

Expr *Resolver::parse(const char *source, int index, int replace, Expr *body) {
  Scanner scanner((new std::string(source))->c_str());
  Parser parser(scanner);
  printf(source);
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

ParseStep Resolver::getParseStep() {
  return uiParseCount <= PARSE_AREAS ? (ParseStep) uiParseCount : uiParseCount <= PARSE_AREAS + NUM_DIRS ? PARSE_LAYOUT : (ParseStep) (uiParseCount - NUM_DIRS + 1);
}

int Resolver::getParseDir() {
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

void Resolver::processAttrs(UIDirectiveExpr *expr) {
  if (expr->previous)
    accept<int>(expr->previous);

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    accept<int>(expr->lastChild);
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
        accept<int>(attExpr->handler, 0);

        Type type = removeLocal();

        if (type.valueType != VAL_VOID) {
          if (attExpr->_uiIndex == ATTRIBUTE_OUT) {
            if (type.valueType != VAL_OBJ || (type.objType->type != OBJ_INSTANCE && type.objType->type != OBJ_FUNCTION)) {
              attExpr->handler = convertToString(attExpr->handler, type, parser);
              type = stringType;
            }
          }

          if (type.valueType == VAL_OBJ && type.objType && type.objType->type == OBJ_INSTANCE) {
            current->function->instanceIndexes->set(current->localCount);
            expr->_eventFlags |= ((ObjFunction *) ((ObjInstance *) type.objType)->callable)->uiFunction->eventFlags;
          }

          char *name = generateInternalVarName("v", current->localCount);
          DeclarationExpr *decExpr = new DeclarationExpr(type, buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), attExpr->handler);

          attExpr->handler = NULL;
          attExpr->_index = current->localCount;
          uiExprs.push_back(decExpr);
          current->addLocal(type);
          current->setLocalName(&decExpr->name);
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
void Resolver::pushAreas(UIDirectiveExpr *expr) {
  if (expr->previous)
    accept<int>(expr->previous);

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
    accept<int>(expr->lastChild);

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
      Type outType = current->enclosing->locals[current->enclosing->resolveLocal(&token)].type;
      char *callee = NULL;

      if (outType.valueType == VAL_OBJ && outType.objType) {
        switch (outType.objType->type) {
          case OBJ_INSTANCE:
            callee = "getInstanceSize";
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

void Resolver::recalcLayout(UIDirectiveExpr *expr) {
  int dir = getParseDir();

  if (expr->previous)
    accept<int>(expr->previous);

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    accept<int>(expr->lastChild);
    parent = oldParent;
  }

  if (hasAreas(expr)) {
    UIDirectiveExpr *previous = getPrevious(expr);

    if (expr->viewIndex || previous)
      expr->_layoutIndexes[dir] = aCount++;

    if (expr->viewIndex)
      (*ss) << "  var " << getUnitName(expr, dir) << " = a" << expr->viewIndex << (dir ? " & 0xFFFF" : " >> 16") << "\n";

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

int Resolver::adjustLayout(UIDirectiveExpr *expr) {
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

void Resolver::paint(UIDirectiveExpr *expr) {
  if (expr->previous)
    accept<int>(expr->previous);

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
    int posDiffDirs = adjustLayout(expr);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      if (posDiffDirs & (1 << dir)) {
        insertTabs();
        (*ss) << "int pos" << dir << " = pos" << dir << " + posDiff" << dir << "\n";
      }
  }

  char *name = (char *) getValueVariableName(expr, ATTRIBUTE_OUT);
  char *callee = NULL;

  if (name) {
    Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
    Type outType = current->enclosing->enclosing->locals[current->enclosing->enclosing->resolveLocal(&token)].type;

    if (outType.valueType == VAL_OBJ && outType.objType) {
      switch (outType.objType->type) {
        case OBJ_INSTANCE:
          callee = "displayInstance";
          break;

        case OBJ_STRING:
          callee = "displayText";
          break;

        case OBJ_FUNCTION:
          callee = ((ObjFunction *) outType.objType)->name->chars;
          name[0] = 0;
          break;
      }

      if (callee) {
        if (expr->lastChild) {
          insertTabs();
          (*ss) << "saveContext()\n";
        }

        insertTabs();
        (*ss) << callee << "(" << name << (name[0] ? ", " : "");
        insertPoint("pos");
        (*ss) << ", ";
        insertPoint("size");
        (*ss) << ")\n";
      }
    }
    else
      parser.error("Out value having an illegal type");
  }

  if (expr->lastChild) {
    UIDirectiveExpr *oldParent = parent;

    parent = expr;
    accept<int>(expr->lastChild);
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

void Resolver::onEvent(UIDirectiveExpr *expr) {
  if (expr->_eventFlags) {
    insertTabs();
    (*ss) << "if ((" << expr->_eventFlags << " & (1 << event)) != 0) {\n";
    nTabs++;

    int posDiffDirs = adjustLayout(expr);
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
      accept<int>(expr->lastChild);
      parent = oldParent;
    }
    else {
      const char *name = getValueVariableName(expr, ATTRIBUTE_OUT);
      Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
      int index = current->enclosing->enclosing->resolveLocal(&token);
      Type outType = current->enclosing->enclosing->locals[index].type;

      switch (outType.objType->type) {
        case OBJ_INSTANCE:
          insertTabs();
          (*ss) << "flag = onInstanceEvent(" << name << ", event, ";
          insertPoint("pos");
          (*ss) << ", ";
          insertPoint("size");
          (*ss) << ")\n";
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
                (*ss) << "$EXPR\n";
                uiExprs.push_back(attExpr->handler);
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

    accept<int>(expr->previous);

    if (expr->_eventFlags) {
      --nTabs;
      insertTabs();
      (*ss) << "}\n";
    }
  }
}
