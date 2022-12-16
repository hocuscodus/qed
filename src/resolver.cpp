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
#include "resolver.hpp"
#include "memory.h"
#include "qni.hpp"

#define NUM_DIRS 2

typedef void (Resolver::*FttrListFn)(AttributeListExpr *expr);
typedef void (Resolver::*AttrFn)(AttributeExpr *expr);

typedef struct {
  FttrListFn attrListFn;
  AttrFn attrFn;
} ResolverUIRule;

#define UI_PARSE_DEF( identifier, push, attr )  { push, attr }
ResolverUIRule resolverUIRules[] = { UI_PARSES_DEF };
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
    &newPrimitive("var", {VAL_OBJ, &objInternalType})->obj};

static bool identifiersEqual2(Token *a, ObjString *b)
{
  return a->length != 0 && a->length == b->length &&
         memcmp(a->start, b->chars, a->length) == 0;
}

static bool isType(Type &type)
{
  if (type.valueType == VAL_OBJ)
    switch (type.objType->type)
    {
    case OBJ_FUNCTION:
    case OBJ_NATIVE:
    {
      ObjString *name = ((ObjNamed *)type.objType)->name;

      return name != NULL && name->chars[0] >= 'A' && name->chars[0] <= 'Z';
    }

    case OBJ_PRIMITIVE:
    case OBJ_FUNCTION_PTR:
      return true;
    }

  return false;
}

static Type convertType(Type &type)
{
  return type.objType->type == OBJ_PRIMITIVE
             ? ((ObjPrimitive *)type.objType)->type
             : type;
}

static Expr *convertToString(Expr *expr, Type &type, Parser &parser)
{
  switch (type.valueType)
  {
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

static Expr *convertToInt(Expr *expr, Type &type, Parser &parser)
{
  switch (type.valueType)
  {
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

static Expr *convertToFloat(Expr *expr, Type &type, Parser &parser)
{
  switch (type.valueType)
  {
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

static void resolveVariableExpr(VariableExpr *expr)
{
  if (expr->index != -1) {
    current->addLocal({VAL_OBJ, primitives[expr->index]});
    return;
  }

  expr->index = current->resolveLocal(&expr->name);

  if (expr->index != (int8_t)-1) {
    Local *local = &current->locals[expr->index];

    current->addLocal(local->type);
  }
  else 
    if ((expr->index = current->resolveUpvalue(&expr->name)) != -1) {
      expr->upvalueFlag = true;
      current->addLocal(current->function->upvalues[expr->index].type);
    }
}

Resolver::Resolver(Parser &parser, Expr *exp) : ExprVisitor(), parser(parser) {
  this->exp = exp;
}

void Resolver::visitAssignExpr(AssignExpr *expr)
{
  //  accept<int>(expr->index, 0);
  accept<int>(expr->value, 0);
  accept<int>(expr->varExp, 0);

  Type type1 = removeLocal();
  Type type2 = removeLocal();

  if (type1.valueType == VAL_VOID)
    parser.error("Variable not found");
  else if (!type1.equals(type2))
    parser.error("Value must match the variable type");

  current->addLocal(type2);
}

void Resolver::visitAttributeExpr(AttributeExpr *expr) {
  int step = getParseStep();

  if (resolverUIRules[step].attrFn)
    (this->*resolverUIRules[step].attrFn)(expr);
}

void Resolver::visitAttributeListExpr(AttributeListExpr *expr) {
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
          case OBJ_COMPILER_INSTANCE:
            expr->_viewIndex = current->localCount;
            current->addLocal({VAL_INT});
            break;

          case OBJ_STRING:
            expr->_viewIndex = current->localCount;
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

void Resolver::visitBinaryExpr(BinaryExpr *expr)
{
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

  switch (expr->op.type)
  {
  case TOKEN_PLUS:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_ADD_STRING
                    : type.valueType == VAL_INT ? OP_ADD_INT
                                                : OP_ADD_FLOAT);
    break;
  case TOKEN_MINUS:
    expr->opCode =
        (type.valueType == VAL_INT ? OP_SUBTRACT_INT : OP_SUBTRACT_FLOAT);
    break;
  case TOKEN_STAR:
    expr->opCode =
        (type.valueType == VAL_INT ? OP_MULTIPLY_INT : OP_MULTIPLY_FLOAT);
    break;
  case TOKEN_SLASH:
    expr->opCode =
        (type.valueType == VAL_INT ? OP_DIVIDE_INT : OP_DIVIDE_FLOAT);
    break;
  case TOKEN_BANG_EQUAL:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_EQUAL_STRING
                    : type.valueType == VAL_INT ? OP_EQUAL_INT
                                                : OP_EQUAL_FLOAT);
    expr->notFlag = true;
    break;
  case TOKEN_EQUAL_EQUAL:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_EQUAL_STRING
                    : type.valueType == VAL_INT ? OP_EQUAL_INT
                                                : OP_EQUAL_FLOAT);
    break;
  case TOKEN_GREATER:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_GREATER_STRING
                    : type.valueType == VAL_INT ? OP_GREATER_INT
                                                : OP_GREATER_FLOAT);
    break;
  case TOKEN_GREATER_EQUAL:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_LESS_STRING
                    : type.valueType == VAL_INT ? OP_LESS_INT
                                                : OP_LESS_FLOAT);
    expr->notFlag = true;
    break;
  case TOKEN_LESS:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_LESS_STRING
                    : type.valueType == VAL_INT ? OP_LESS_INT
                                                : OP_LESS_FLOAT);
    break;
  case TOKEN_LESS_EQUAL:
    expr->opCode = (type.valueType == VAL_OBJ   ? OP_GREATER_STRING
                    : type.valueType == VAL_INT ? OP_GREATER_INT
                                                : OP_GREATER_FLOAT);
    expr->notFlag = true;
    break;
  case TOKEN_OR:
    expr->opCode = OP_BITWISE_OR;
    break;
  case TOKEN_OR_OR:
    expr->opCode = OP_LOGICAL_OR;
    break;
  case TOKEN_OR_EQUAL:
    expr->opCode = type.valueType == VAL_INT ? OP_BITWISE_OR : OP_LOGICAL_OR;
    break;
  case TOKEN_AND:
    expr->opCode = OP_BITWISE_AND;
    break;
  case TOKEN_AND_AND:
    expr->opCode = OP_LOGICAL_AND;
    break;
  case TOKEN_AND_EQUAL:
    expr->opCode = type.valueType == VAL_INT ? OP_BITWISE_AND : OP_LOGICAL_AND;
    break;
  case TOKEN_XOR:
    expr->opCode = OP_BITWISE_XOR;
    break;
  case TOKEN_XOR_EQUAL:
    expr->opCode = OP_BITWISE_XOR;
    break;
  case TOKEN_GREATER_GREATER:
    expr->opCode = OP_SHIFT_RIGHT;
    break;
  case TOKEN_GREATER_GREATER_GREATER:
    expr->opCode = OP_SHIFT_URIGHT;
    break;
  case TOKEN_LESS_LESS:
    expr->opCode = OP_SHIFT_LEFT;
    break;
  default:
    return; // Unreachable.
  }

  switch (expr->op.type)
  {
  case TOKEN_PLUS:
    if (type1.valueType == VAL_OBJ) {
      expr->right = convertToString(expr->right, type2, parser);
      current->addLocal(VAL_OBJ);
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
    if (type1.valueType == VAL_OBJ)
    {
      if (type2.valueType != VAL_OBJ)
        parser.error("Second operand must be a string");

      current->addLocal(VAL_BOOL);
      return;
    }
    // no break statement, fall through

  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  {
    switch (type1.valueType)
    {
    case VAL_INT:
      if (!type1.equals(type2))
      {
        if (type2.valueType != VAL_FLOAT)
        {
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
      if (!type1.equals(type2))
      {
        if (type2.valueType != VAL_INT)
        {
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

void Resolver::visitCallExpr(CallExpr *expr)
{
  accept<int>(expr->callee);

  Type type = removeLocal();
  //  Local *callerLocal = current->peekLocal(expr->argCount);

  if (type.valueType == VAL_OBJ)
  {
    switch (type.objType->type)
    {
    case OBJ_FUNCTION:
    case OBJ_NATIVE: {
      ObjCallable *callable = (ObjCallable *)type.objType;

      if (expr->count != callable->arity)
        parser.error("Expected %d arguments but got %d.", callable->arity, expr->count);

      if (expr->newFlag)
//        current->localCount++;
        current->addLocal((Type) {VAL_OBJ, &newCompilerInstance(callable)->obj});
      else
        current->addLocal(callable->type);

      for (int index = 0; index < expr->count; index++) {
        expr->arguments[index]->accept(this);

        Type argType = removeLocal();

        argType = argType;
      }

      if (expr->handler != NULL)
      {
        Expr **expList = NULL;
        Token groupToken = buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1);

        expList = RESIZE_ARRAY(Expr *, expList, 0, 1);
        expList[0] = expr->handler;

        expr->groupingExpr = new GroupingExpr(groupToken, 1, expList, 0, NULL, NULL);
        current->beginScope();
        current->addLocal(VAL_INT);
        current->addLocal(type);

        if (callable->type.valueType != VAL_VOID)
          current->addLocal(callable->type);

        acceptGroupingExprUnits(expr->groupingExpr);
        expr->groupingExpr->popLevels = current->endScope();
        //        current->addLocal(VAL_VOID);
      }
      break;
    }
    case OBJ_FUNCTION_PTR:
    {
      ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;

      if (expr->count != callable->arity)
        parser.error("Expected %d arguments but got %d.", callable->arity, expr->count);

      if (expr->newFlag)
        current->localCount++;
      else
        current->addLocal(callable->type.valueType);

      for (int index = 0; index < expr->count; index++)
      {
        expr->arguments[index]->accept(this);
        Type argType = removeLocal();

        argType = argType;
      }

      if (expr->handler != NULL)
      {
        Expr **expList = NULL;
        Token groupToken = buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1);

        expList = RESIZE_ARRAY(Expr *, expList, 0, 1);
        expList[0] = expr->handler;

        expr->groupingExpr = new GroupingExpr(groupToken, 1, expList, 0, NULL, NULL);
        current->beginScope();
        current->addLocal(VAL_INT);
        current->addLocal(type);

        if (callable->type.valueType != VAL_VOID)
          current->addLocal(callable->type);

        acceptGroupingExprUnits(expr->groupingExpr);
        expr->groupingExpr->popLevels = current->endScope();
        //        current->addLocal(VAL_VOID);
      }
      break;
    }
    default:
      parser.error("Non-callable object type");
      current->addLocal(VAL_VOID);
      break; // Non-callable object type.
    }
  }
  else
  {
    parser.error("Non-callable object type");
    //    return INTERPRET_RUNTIME_ERROR;
  }
}

void Resolver::visitDeclarationExpr(DeclarationExpr *expr)
{
  checkDeclaration(&expr->name);

  if (expr->initExpr != NULL)
  {
    accept<int>(expr->initExpr, 0);
    Type type1 = removeLocal();
  }

  current->addLocal(expr->type);
  current->setLocalName(&expr->name);
}

void Resolver::visitFunctionExpr(FunctionExpr *expr)
{
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
    compiler.function->fields[index].name =
        copyString(local->name.start, local->name.length);
  }

  current = current->enclosing;
  current->setLocalObjType(compiler.function);
  expr->function = compiler.function;
}

void Resolver::visitGetExpr(GetExpr *expr)
{
  accept<int>(expr->object);

  Type objectType = removeLocal();

  if (objectType.valueType != VAL_OBJ || objectType.objType->type != OBJ_COMPILER_INSTANCE)
    parser.errorAt(&expr->name, "Only instances have properties.");
  else
  {
    ObjFunction *type = (ObjFunction *)objectType.objType;

    for (int i = 0; expr->index == -1 && i < type->fieldCount; i++)
    {
      Field *field = &type->fields[i];

      if (identifiersEqual2(&expr->name, field->name)) // && local->depth != -1)
        expr->index = i;
    }

    if (expr->index == -1)
      parser.errorAt(&expr->name, "Field '%.*s' not found.", expr->name.length,
                     expr->name.start);
    else
    {
      current->addLocal(type->fields[expr->index].type);
      return;
    }
  }

  current->addLocal({VAL_VOID});
}

void Resolver::visitGroupingExpr(GroupingExpr *expr)
{
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
void Resolver::visitListExpr(ListExpr *expr)
{
  Expr *subExpr = expr->expressions[0];

  accept<int>(subExpr, 0);
  Type type = removeLocal();

  if (isType(type)) {
    // parse declaration
    Type returnType = convertType(type);

    subExpr = expr->expressions[1];

    switch (subExpr->type)
    {
    case EXPR_ASSIGN:
    {
      expr->listType = subExpr->type;

      AssignExpr *assignExpr = (AssignExpr *)subExpr;

      resolveVariableExpr(assignExpr->varExp);

      //        checkDeclaration(&expr->varExp->name);
      if (assignExpr->varExp->index != -1 && !assignExpr->varExp->upvalueFlag)
        parser.error("Already a variable with this name in this scope.");

      if (assignExpr->value != NULL)
      {
        accept<int>(assignExpr->value, 0);
        Type internalType = {VAL_OBJ, &objInternalType};
        Type type1 = removeLocal();

        //          if (type1.valueType == VAL_VOID)
        //            parser.error("Variable not found");
        //          else
        if (returnType.equals(internalType))
          returnType = type1;
        else if (!type1.equals(returnType))
          parser.error("Value must match the variable type");
      }

      current->addLocal(returnType);
      current->setLocalName(&assignExpr->varExp->name);

      if (expr->count > 2)
        parser.error("Expect ';' or newline after variable declaration.");
      return;
    }
    case EXPR_CALL:
    {
      Expr *bodyExpr = expr->count > 2 ? expr->expressions[2] : NULL;

      expr->listType = subExpr->type;

      CallExpr *callExpr = (CallExpr *)subExpr;

      if (callExpr->newFlag)
        parser.error("Syntax error: new.");
      else if (callExpr->callee->type != EXPR_VARIABLE)
        parser.error("Function name must be a string.");
      else {
        VariableExpr *varExp = (VariableExpr *)callExpr->callee;

        resolveVariableExpr(varExp);

        //          checkDeclaration(&varExp->name);
        if (varExp->index != -1 && !varExp->upvalueFlag)
          parser.error("Already a variable with this name in this scope.");

        if (!varExp->name.equal("UI$")) {
          current->addLocal(VAL_OBJ);
          current->setLocalName(&varExp->name);
        }

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

            if (isType(type))
            {
              paramExpr = param->expressions[1];

              if (paramExpr->type != EXPR_VARIABLE)
                parser.error("Parameter name must be a string.");
              else
              {
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

        if (bodyExpr)
          if (bodyExpr->type == EXPR_GROUPING && ((GroupingExpr *)bodyExpr)->name.type == TOKEN_RIGHT_BRACE)
            acceptGroupingExprUnits((GroupingExpr *)bodyExpr);
          else
          {
            accept<int>(bodyExpr, 0);
            removeLocal();
          }

        compiler.function->fieldCount = compiler.localCount;
        compiler.function->fields = ALLOCATE(Field, compiler.localCount);

        for (int index = 0; index < compiler.localCount; index++)
        {
          Local *local = &compiler.locals[index];

          compiler.function->fields[index].type = local->type;
          compiler.function->fields[index].name = copyString(local->name.start, local->name.length);
        }

        current = current->enclosing;
        if (varExp->name.equal("UI$"))
          current->function->uiFunction = compiler.function;
        else
          current->setLocalObjType(compiler.function);

        expr->function = compiler.function;
      }
      break;
    }
    case EXPR_VARIABLE:
    {
      VariableExpr *varExpr = (VariableExpr *)subExpr;

      resolveVariableExpr(varExpr);

      //        checkDeclaration(&expr->varExp->name);
      if (varExpr->index != -1 && !varExpr->upvalueFlag)
        parser.error("Already a variable with this name in this scope.");

      current->addLocal(returnType);
      current->setLocalName(&varExpr->name);

      LiteralExpr *valueExpr = NULL;

      switch (returnType.valueType)
      {
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
        switch (returnType.objType->type)
        {
        case OBJ_STRING:
          valueExpr = new LiteralExpr(VAL_OBJ, {.obj = &copyString("", 0)->obj});
          break;

        case OBJ_INTERNAL:
          valueExpr = new LiteralExpr(VAL_OBJ, {.obj = &newInternal()->obj});
          break;
        }
        break;
      }

      if (valueExpr)
      {
        expr->expressions[1] = new AssignExpr(
            varExpr, buildToken(TOKEN_EQUAL, "=", 1, varExpr->name.line),
            valueExpr);
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
    for (int index = 0; index < expr->count; index++)
    {
      Expr *subExpr = expr->expressions[index];

      accept<int>(subExpr, 0);

      if (current->peekLocal(0)->type.valueType == VAL_VOID)
        removeLocal();
    }

  current->addLocal({VAL_VOID});
}

void Resolver::visitLiteralExpr(LiteralExpr *expr)
{
  current->addLocal(expr->type);
}

void Resolver::visitLogicalExpr(LogicalExpr *expr)
{
  accept(expr->left, 0);
  accept(expr->right, 0);

  Type type2 = removeLocal();
  Type type1 = removeLocal();

  if (type1.valueType != VAL_BOOL || type2.valueType != VAL_BOOL)
    parser.error("Value must be boolean");

  current->addLocal(VAL_BOOL);
}

void Resolver::visitOpcodeExpr(OpcodeExpr *expr)
{
  parser.compilerError("In OpcodeExpr!");
  expr->right->accept(this);
}

void Resolver::visitReturnExpr(ReturnExpr *expr)
{
  if (expr->value != NULL)
  {
    accept<int>(expr->value, 0);

    Type type = removeLocal();
    // verify that type is the function return type
  }

  current->addLocal(VAL_VOID);
}

void Resolver::visitSetExpr(SetExpr *expr)
{
  accept<int>(expr->object);
  accept<int>(expr->value);

  Type valueType = removeLocal();
  Type objectType = removeLocal();

  if (objectType.valueType != VAL_OBJ || objectType.objType->type != OBJ_COMPILER_INSTANCE)
    parser.errorAt(&expr->name, "Only instances have properties.");
  else
  {
    ObjFunction *type = (ObjFunction *)objectType.objType;

    for (int i = 0; expr->index == -1 && i < type->fieldCount; i++)
    {
      Field *field = &type->fields[i];

      if (identifiersEqual2(&expr->name, field->name)) // && local->depth != -1)
        expr->index = i;
    }

    if (expr->index == -1)
      parser.errorAt(&expr->name, "Field '%.*s' not found.", expr->name.length,
                     expr->name.start);
    else
    {
      current->addLocal(type->fields[expr->index].type);
      return;
    }
  }

  current->addLocal({VAL_VOID});
}

void Resolver::visitStatementExpr(StatementExpr *expr)
{
  int oldLocalCount = current->localCount;

  acceptSubExpr(expr->expr);

  if ((!current->function->name || strcmp(current->function->name->chars, "UI$")) && (expr->expr->type != EXPR_LIST || ((ListExpr *)expr->expr)->listType == EXPR_LIST)) {
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

void Resolver::visitTernaryExpr(TernaryExpr *expr)
{
  expr->left->accept(this);

  Type type = removeLocal();

  if (type.valueType == VAL_VOID)
    parser.error("Value must not be void");

  expr->middle->accept(this);

  if (expr->right)
  {
    expr->right->accept(this);
    removeLocal();
  }
}

void Resolver::visitThisExpr(ThisExpr *expr) {}

void Resolver::visitTypeExpr(TypeExpr *expr)
{
  //  current->addLocal({VAL_OBJ, (ObjPrimitiveType) {{OBJ_PRIMITIVE_TYPE},
  //  expr->type}});
}

void Resolver::visitUnaryExpr(UnaryExpr *expr)
{
  accept<int>(expr->right, 0);

  Type type = removeLocal();

  if (type.valueType == VAL_VOID)
    parser.error("Value must not be void");

  switch (expr->op.type)
  {
  case TOKEN_PRINT:
    expr->right = convertToString(expr->right, type, parser);
    current->addLocal(VAL_VOID);
    break;

  case TOKEN_NEW:
  {
    bool errorFlag = true;

    if (type.valueType == VAL_OBJ)
      switch (type.objType->type) {
      case OBJ_FUNCTION:
      case OBJ_NATIVE:
        errorFlag = false;
        break;
      }

    if (errorFlag)
      parser.error("Operator new must target a callable function");

    current->localCount++;
    break;
  }
  default:
    current->addLocal(type);
    break;
  }
}

void Resolver::visitVariableExpr(VariableExpr *expr)
{
  resolveVariableExpr(expr);

  if (expr->index == -1)
  {
    parser.error("Variable must be defined");
    current->addLocal(VAL_VOID);
  }
}

void Resolver::checkDeclaration(Token *name)
{
  for (int i = current->localCount - 1; i >= 0; i--)
  {
    Local *local = &current->locals[i];

    if (local->depth != -1 && local->depth < current->scopeDepth)
      break;

    if (identifiersEqual(name, &local->name))
      parser.error("Already a variable with this name in this scope.");
  }
}

Type Resolver::removeLocal()
{
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

bool Resolver::resolve(Compiler *compiler)
{
  Compiler *oldCurrent = current;

  current = compiler;
  accept(exp, 0);
  current = oldCurrent;
  return !parser.hadError;
}

static std::list<Expr *> uiExprs;

static Expr *generateUIFunction(const char *name, Expr *uiExpr, int count, int restLength, Expr **rest) {
    // Perform the UI AST magic
    VariableExpr *nameExpr = new VariableExpr(buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), -1, false);
    Expr **bodyExprs = new Expr *[count + restLength];
    Expr **functionExprs = new Expr *[3];

    for (int index = 0; index < count; index++)
      bodyExprs[index] = uiExpr;

    for (int index = 0; index < restLength; index++)
      bodyExprs[count + index] = rest[index];

    functionExprs[0] = new VariableExpr(buildToken(TOKEN_IDENTIFIER, "void", 4, -1), VAL_VOID, false);
    functionExprs[1] = new CallExpr(nameExpr, buildToken(TOKEN_RIGHT_PAREN, ")", 1, -1), 0, NULL, false, NULL, NULL);
    functionExprs[2] = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), count + restLength, bodyExprs, 0, NULL, NULL);

    return new ListExpr(3, functionExprs, EXPR_LIST, NULL);
}

void Resolver::acceptGroupingExprUnits(GroupingExpr *expr)
{
  TokenType type = expr->name.type;
  bool parenFlag = type == TOKEN_RIGHT_PAREN;

  for (int index = 0; index < expr->count; index++) {
    Expr *subExpr = expr->expressions[index];

    acceptSubExpr(subExpr);

    if (subExpr->type == EXPR_ATTRIBUTELIST) {
      int size = uiExprs.size();
      Expr **newExpressions = new Expr *[size + expr->count - 1];

      memcpy(newExpressions, expr->expressions, index * sizeof(Expr *));
      memcpy(&newExpressions[index + size], &expr->expressions[index + 1], (--expr->count - index) * sizeof(Expr *));

      for (Expr *expr : uiExprs)
        newExpressions[index++] = expr;

      expr->count += size;
      index--;
      uiExprs.clear();
      delete[] expr->expressions;
      expr->expressions = newExpressions;
      uiParseCount++;
    }
    else
      if (current->peekLocal(0)->type.valueType == VAL_VOID && (!parenFlag || index < expr->count - 1))
        removeLocal();
  }

  if (expr->ui != NULL)
    if (((AttributeListExpr *) expr->ui)->childrenCount) {
      // Perform the UI AST magic
      Expr *paintFunction = generateUIFunction("paint", expr->ui, 1, 0, NULL);
      Expr **uiFunctions = new Expr *[1];

      uiFunctions[0] = paintFunction;

      Expr *layoutFunction = generateUIFunction("Layout", expr->ui, 3, 0, uiFunctions);
      Expr *valueFunction = generateUIFunction("UI$", expr->ui, 1, 1, new Expr *[] {layoutFunction});

      uiParseCount = 0;
      accept<int>(valueFunction, 0);
      delete expr->ui;
      expr->ui = valueFunction;
    }
    else {
      delete expr->ui;
      expr->ui = NULL;
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
void Resolver::acceptSubExpr(Expr *expr)
{
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

ParseStep Resolver::getParseStep() {
  return uiParseCount <= PARSE_AREAS ? (ParseStep) uiParseCount : uiParseCount <= PARSE_AREAS + NUM_DIRS ? PARSE_LAYOUT : (ParseStep) (uiParseCount - NUM_DIRS + 1);
}

int Resolver::getParseDir() {
  return uiParseCount - PARSE_LAYOUT;
}

void Resolver::parseChildren(AttributeListExpr *expr) {
  for (int index = 0; index < expr->childrenCount; index++)
    accept<int>(expr->children[index], 0);
}

void Resolver::processAttrs(AttributeListExpr *expr) {
  for (int index = 0; index < expr->attCount; index++)
    accept<int>(expr->attributes[index], 0);

  parseChildren(expr);
}

static std::set<std::string> outAttrs({"out", "bgcol", "textcol", "fontSize", "width", "height", "alignx", "aligny", "zoomwidth", "zoomheight"});

static char *generateInternalVarName(const char *prefix, int suffix) {
  char buffer[20];

  sprintf(buffer, "%s%d", prefix, suffix);

  char *str = new char[strlen(buffer) + 1];

  strcpy(str, buffer);
  return str;
}

void Resolver::evalValue(AttributeExpr *expr) {
  if (outAttrs.find(expr->name.getString()) != outAttrs.end())
    if (expr->handler) {
      accept<int>(expr->handler, 0);

      Type type = removeLocal();

      if (type.valueType != VAL_VOID) {
        if (!expr->name.getString().compare("out")) {
          if (type.valueType != VAL_OBJ || (type.objType->type != OBJ_COMPILER_INSTANCE && type.objType->type != OBJ_FUNCTION)) {
            expr->handler = convertToString(expr->handler, type, parser);
            type = stringType;
          }
        }

        if (type.valueType == VAL_OBJ && type.objType && type.objType->type == OBJ_COMPILER_INSTANCE)
          current->function->instanceIndexes->set(current->localCount);

        char *name = generateInternalVarName("v", current->localCount);
        DeclarationExpr *decExpr = new DeclarationExpr(type, buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), expr->handler);

        expr->handler = NULL;
        expr->_index = current->localCount;
        uiExprs.push_back(decExpr);
        current->addLocal(type);
        current->setLocalName(&decExpr->name);
      }
      else
        parser.error("Value must not be void");
//          attExprs.push_back(expr);
    }
    else
      ;
}

int outAttrIndex = -1;
//ValueStack<int> valueStack(-1);
ValueStack<ValueStackElement> valueStack2;
//std::list<AttributeExpr *> attExprs;

void Resolver::pushAreas(AttributeListExpr *expr) {
  int oldOutAttrIndex = outAttrIndex;

  for (int index = 0; index < expr->attCount; index++)
    if (expr->attributes[index]->_index != -1 && expr->attributes[index]->name.equal("out")) {
      outAttrIndex = expr->attributes[index]->_index;
      break;
    }

  if (!expr->childrenCount && outAttrIndex != -1) {
    char *name = generateInternalVarName("v", outAttrIndex);
    VariableExpr *outVar = new VariableExpr(buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1), -1, false);

    resolveVariableExpr(outVar);

    Type outType = removeLocal();
    char *callee = NULL;

    if (outType.valueType == VAL_OBJ && outType.objType) {
      switch (outType.objType->type) {
        case OBJ_COMPILER_INSTANCE:
          callee = "getInstanceSize";
          break;

        case OBJ_STRING:
          callee = "getTextSize";
          break;

        case OBJ_FUNCTION:
          break;

        default:
          parser.error("Out value having an illegal type");
          break;
      }

      if (callee) {
        Expr **expList = NULL;
        Expr *handler = NULL;
        VariableExpr *calleeExpr = new VariableExpr(buildToken(TOKEN_IDENTIFIER, callee, strlen(callee), -1), -1, false);

        resolveVariableExpr(calleeExpr);
        removeLocal();
        expList = RESIZE_ARRAY(Expr *, expList, 0, 1);
        expList[0] = outVar;
        Expr *callExp = new CallExpr(calleeExpr, buildToken(TOKEN_RIGHT_PAREN, ")", 1, -1), 1, expList, false, NULL, NULL);

        expr->_viewIndex = current->localCount;
        uiExprs.push_back(callExp);
        current->addLocal({VAL_INT});
      }
      else
        expr->_viewIndex = -1;
    }
    else
      parser.error("Out value having an illegal type");

    if (!callee) {
      delete outVar;
      delete[] name;
    }
  }
  else {
    IndexList indexList;

    for (int index = 0; index < expr->childrenCount; index++) {
      AttributeListExpr *subExpr = (AttributeListExpr *) expr->children[index];

      accept<int>(subExpr, 0);

      if (subExpr->_viewIndex != -1)
        indexList.set(index);
    }

    expr->_viewIndex = indexList.size != -1 ? indexList.array[0] : -1;
  }

  for (int index = 0; index < expr->attCount; index++)
    if (expr->attributes[index]->_index != -1)
;//      valueStack.pop(expr->attributes[index]->name.getString());

  outAttrIndex = oldOutAttrIndex;
}

int subViewIndex;

void Resolver::recalcLayout(AttributeListExpr *expr) {
  if (expr->childrenCount) {
    int acc = -1;
    IndexList indexList(expr->_viewIndex);

    for (int index = -1; (index = indexList.getNext(index)) != -1;) {
      accept<int>(expr->children[index], 0);

      if (acc != -1) {
        uiExprs.push_back(new OpcodeExpr((OpCode) subViewIndex, new OpcodeExpr((OpCode) acc, new OpcodeExpr(OP_ADD_LOCAL, NULL))));
        acc = current->localCount;
        current->addLocal({VAL_INT});
      }
      else
        acc = subViewIndex;
    }

    subViewIndex = acc;
  }
  else {
    uiExprs.push_back(new OpcodeExpr((OpCode) expr->_viewIndex, new OpcodeExpr((OpCode) getParseDir(), new OpcodeExpr(OP_GET_LOCAL_DIR, NULL))));
    subViewIndex = current->localCount;
    current->addLocal({VAL_INT});
  }
}
/*
Expr *Parser::forStatement(TokenType endGroupType) {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

  Expr *initializer = NULL;

  if (!match(TOKEN_SEPARATOR))
    initializer = match(TOKEN_VAR) ? varDeclaration(endGroupType) : expressionStatement(endGroupType);

  TokenType tokens[] = {TOKEN_SEPARATOR, TOKEN_EOF};
  Expr *condition = check(TOKEN_SEPARATOR) ? createBooleanExpr(true) : expression(tokens);

  consume(TOKEN_SEPARATOR, "Expect ';' or newline after loop condition.");

  TokenType tokens2[] = {TOKEN_RIGHT_PAREN, TOKEN_SEPARATOR, TOKEN_EOF};
  Expr *increment = check(TOKEN_RIGHT_PAREN) ? NULL : expression(tokens2);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

  Expr *body = statement(endGroupType);

  if (increment != NULL) {
    Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 2);

    expList[0] = new UnaryExpr(buildToken(TOKEN_PRINT, "print", 5, -1), body);
    expList[1] = new UnaryExpr(buildToken(TOKEN_PRINT, "print", 5, -1), increment);
    body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 2, expList, 0, NULL, NULL);
  }

  body = new BinaryExpr(condition, buildToken(TOKEN_WHILE, "while", 5, -1), body, OP_FALSE, false);

  if (initializer != NULL) {
    Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 2);

    expList[0] = initializer;
    expList[1] = body;
    body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 2, expList, 0, NULL, NULL);
  }

  return body;
}
*/