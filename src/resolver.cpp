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
  &newPrimitive("handler", {VAL_HANDLER})->obj
};

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
  uiParseCount = -1;
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
          case OBJ_COMPILER_INSTANCE:
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

    functionExprs[0] = new VariableExpr(buildToken(TOKEN_IDENTIFIER, type, strlen(type), -1), VAL_HANDLER, false);
    functionExprs[1] = new CallExpr(nameExpr, buildToken(TOKEN_RIGHT_PAREN, ")", 1, -1), nbParms, parms, false, NULL);
    functionExprs[2] = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), count + restLength, bodyExprs, 0, NULL);

    return new ListExpr(3, functionExprs, EXPR_LIST, NULL);
}

void Resolver::visitCallExpr(CallExpr *expr)
{
  if (expr->newFlag && expr->handler != NULL)
  {
    expr->handler = generateUIFunction("void", "handler", NULL, expr->handler, 1, 0, NULL);
    accept<int>(expr->handler);
    removeLocal();
  }

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
        current->addLocal((Type) {VAL_OBJ, &newCompilerInstance(callable)->obj});
      else
        current->addLocal(callable->type);

      for (int index = 0; index < expr->count; index++) {
        accept<int>(expr->arguments[index]);

        Type argType = removeLocal();

        argType = argType;
      }
      break;
    }
    case OBJ_FUNCTION_PTR:
    {
      ObjFunctionPtr *callable = (ObjFunctionPtr *)type.objType;

      if (expr->count != callable->arity)
        parser.error("Expected %d arguments but got %d.", callable->arity, expr->count);

      if (expr->newFlag)
;//        current->addLocal((Type) {VAL_OBJ, &newCompilerInstance(callable)->obj});
      else
        current->addLocal(callable->type.valueType);

      for (int index = 0; index < expr->count; index++)
      {
        accept<int>(expr->arguments[index]);
        Type argType = removeLocal();

        argType = argType;
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
      AssignExpr *assignExpr = (AssignExpr *)subExpr;

      expr->listType = subExpr->type;
      checkDeclaration(&assignExpr->varExp->name);

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

        if (returnType.valueType != VAL_HANDLER && strcmp(compiler.function->name->chars, "return"))
          if (returnType.valueType == VAL_VOID)
            bodyExpr = parse("void return();", 0, 0, bodyExpr);
          else {
            char returnDec[128];

//            sprintf(returnDec, "void return(%s value);", returnType.valueType);
//            parse(returnDec);
          }

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
        current->setLocalObjType(compiler.function);
        expr->function = compiler.function;
      }
      return;
    }
    case EXPR_VARIABLE:
    {
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

  if (expr->index == -1) {
    resolveVariableExpr(expr);
    parser.error("Variable must be defined");
    current->addLocal(VAL_VOID);
  }
}

static std::list<Expr *> uiExprs;

void Resolver::visitSwapExpr(SwapExpr *expr) {
  expr->_expr = uiExprs.front();
  uiExprs.pop_front();
  accept<int>(expr->_expr);
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

int aCount;
std::stringstream s;
std::stringstream *ss = &s;

int nTabs = 0;

static void insertTabs() {
  for (int index = 0; index < nTabs; index++)
    (*ss) << "  ";
}

void Resolver::acceptGroupingExprUnits(GroupingExpr *expr) {
  TokenType type = expr->name.type;
  bool parenFlag = type == TOKEN_RIGHT_PAREN;

  for (int index = 0; index < expr->count; index++) {
    Expr *subExpr = expr->expressions[index];

    if (subExpr->type == EXPR_UIDIRECTIVE && ++uiParseCount >= 1)
      ss->str("");

    acceptSubExpr(subExpr);

    if (subExpr->type == EXPR_UIDIRECTIVE) {
      UIDirectiveExpr *exprUI = (UIDirectiveExpr *) subExpr;

      if (uiParseCount) {
        if (uiParseCount == 3) {
          nTabs++;
          insertTabs();
          (*ss) << "var size = (l" << exprUI->layoutIndexes[0] << " << 32) | l" << exprUI->layoutIndexes[1] << "\n";
          nTabs--;
        }

        printf("CODE %d\n%s", uiParseCount, ss->str().c_str());
        expr = (GroupingExpr *) parse(ss->str().c_str(), index, 1, expr);
      }
      else {
        expr->expressions = RESIZE_ARRAY(Expr *, expr->expressions, expr->count, expr->count + uiExprs.size() - 1);
        memcpy(&expr->expressions[index + uiExprs.size()], &expr->expressions[index + 1], (--expr->count - index) * sizeof(Expr *));

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
      Expr *clickFunction = generateUIFunction("void", "onEvent", "int pos0, int pos1", expr->ui, 1, 0, NULL);
      Expr *paintFunction = generateUIFunction("void", "paint", "int pos0, int pos1, int size0, int size1", expr->ui, 1, 0, NULL);
      Expr **uiFunctions = new Expr *[2];

      uiFunctions[0] = paintFunction;
      uiFunctions[1] = clickFunction;

      Expr *layoutFunction = generateUIFunction("void", "Layout", NULL, expr->ui, 3, 1, uiFunctions);
      Expr **layoutExprs = new Expr *[1];

      layoutExprs[0] = layoutFunction;

      Expr *valueFunction = generateUIFunction("void", "UI$", NULL, expr->ui, 1, 1, layoutExprs);

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

Expr *Resolver::parse(const char *source, int index, int replace, Expr *body) {
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
          memcpy(&bodyGroup->expressions[index + group->count], &bodyGroup->expressions[index + replace], (bodyGroup->count - index) * sizeof(Expr *));

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

static std::set<std::string> outAttrs({"out", "color", "fontSize", "width", "height", "alignX", "alignY", "zoomWidth", "zoomHeight"});

static char *generateInternalVarName(const char *prefix, int suffix) {
  char buffer[20];

  sprintf(buffer, "%s%d", prefix, suffix);

  char *str = new char[strlen(buffer) + 1];

  strcpy(str, buffer);
  return str;
}

void Resolver::processAttrs(UIDirectiveExpr *expr) {
  if (expr->previous)
    accept<int>(expr->previous);

  if (expr->lastChild)
    accept<int>(expr->lastChild);

  for (int index = 0; index < expr->attCount; index++) {
    UIAttributeExpr *attExpr = expr->attributes[index];

    if (outAttrs.find(attExpr->name.getString()) != outAttrs.end() && attExpr->handler) {
      accept<int>(attExpr->handler, 0);

      Type type = removeLocal();

      if (type.valueType != VAL_VOID) {
        if (!attExpr->name.getString().compare("out")) {
          if (type.valueType != VAL_OBJ || (type.objType->type != OBJ_COMPILER_INSTANCE && type.objType->type != OBJ_FUNCTION)) {
            attExpr->handler = convertToString(attExpr->handler, type, parser);
            type = stringType;
          }
        }

        if (type.valueType == VAL_OBJ && type.objType && type.objType->type == OBJ_COMPILER_INSTANCE)
          current->function->instanceIndexes->set(current->localCount);

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

int outAttrIndex = -1;
//ValueStack<int> valueStack(-1);
//ValueStack<ValueStackElement> valueStack2;
//std::list<UIAttributeExpr *> attExprs;

static int findAttrIndex(UIDirectiveExpr *expr, const char *name) {
  for (int index = 0; index < expr->attCount; index++)
    if (expr->attributes[index]->_index != -1 && expr->attributes[index]->name.equal(name))
      return expr->attributes[index]->_index;

  return -1;
}

void Resolver::pushAreas(UIDirectiveExpr *expr) {
  if (expr->previous)
    accept<int>(expr->previous);

  int oldOutAttrIndex = outAttrIndex;

  outAttrIndex = findAttrIndex(expr, "out");

  if (expr->lastChild) {
    accept<int>(expr->lastChild);

    for (UIDirectiveExpr *child = expr->lastChild; expr->viewIndex == -1 && child; child = child->previous)
      expr->viewIndex = child->viewIndex != -1 ? 0 : -1;
  }
  else
    if (outAttrIndex != -1) {
      char name[20];

      sprintf(name, "v%d", outAttrIndex);

      Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
      Type outType = current->enclosing->locals[current->enclosing->resolveLocal(&token)].type;
      char *callee = NULL;

      if (outType.valueType == VAL_OBJ && outType.objType) {
        switch (outType.objType->type) {
          case OBJ_COMPILER_INSTANCE:
            callee = "getInstanceSize";
            break;

          case OBJ_STRING:
            callee = "getTextSize";
            break;
        }

        if (callee) {
          expr->viewIndex = aCount;
          (*ss) << "  var a" << aCount++ << " = " << callee << "(v" << outAttrIndex << ")\n";
        }
      }
    }

  for (int index = 0; index < expr->attCount; index++)
    if (expr->attributes[index]->_index != -1)
;//      valueStack.pop(expr->attributes[index]->name.getString());

  outAttrIndex = oldOutAttrIndex;
}

void Resolver::recalcLayout(UIDirectiveExpr *expr) {
  int parseDir = getParseDir();

  if (expr->previous)
    accept<int>(expr->previous);

  if (expr->lastChild)
    accept<int>(expr->lastChild);

  if (expr->viewIndex != -1) {
    if (expr->lastChild)
      expr->layoutIndexes[parseDir] = expr->lastChild->layoutIndexes[parseDir];
    else {
      (*ss) << "  var l" << aCount << " = a" << expr->viewIndex << (parseDir ? " & 0xFFFFFFFF" : " >> 32") << "\n";
      expr->layoutIndexes[parseDir] = aCount++;
    }

    for (UIDirectiveExpr *previous = expr->previous; previous; previous = previous->previous)
      if (previous->viewIndex != -1) {
        (*ss) << "  var l" << aCount << " = l" << previous->layoutIndexes[parseDir] << " + l" << expr->layoutIndexes[parseDir] << "\n";
        expr->layoutIndexes[parseDir] = aCount++;
        break;
      }
  }
}

static void insertPoint(const char *prefix) {/*
  ss << "new Point(";

  for (int dir = 0; dir < NUM_DIRS; dir++)
    ss << (dir == 0 ? "" : ", ") << prefix << dir;

  (*ss) << ")";*/
  (*ss) << "((" << prefix << "0 << 32) | " << prefix << "1)";
}

void Resolver::paint(UIDirectiveExpr *expr) {
  if (expr->previous)
    accept<int>(expr->previous);

  int oldOutAttrIndex = outAttrIndex;
  UIDirectiveExpr *previous;

  outAttrIndex = findAttrIndex(expr, "out");

  for (previous = expr->previous; previous; previous = previous->previous)
    if (previous->viewIndex != -1)
      break;

  insertTabs();
  (*ss) << "{\n";
  nTabs++;

  if (nTabs > 1)
    if (previous) {
      insertTabs();
      (*ss) << "int pos0 = pos0 + l" << previous->layoutIndexes[0] << "\n";
      insertTabs();
      (*ss) << "int pos1 = pos1 + l" << previous->layoutIndexes[1] << "\n";
      insertTabs();
      (*ss) << "int size0 = l" << expr->layoutIndexes[0] << " - l" << previous->layoutIndexes[0] << "\n";
      insertTabs();
      (*ss) << "int size1 = l" << expr->layoutIndexes[1] << " - l" << previous->layoutIndexes[1] << "\n";
    }
    else {
      insertTabs();
      if (expr->layoutIndexes[0])
        (*ss) << "int size0 = l" << expr->layoutIndexes[0] << "\n";
      else
  ;//      (*ss) << "int size0 = 0\n";
      insertTabs();
      if (expr->layoutIndexes[1])
        (*ss) << "int size1 = l" << expr->layoutIndexes[1] << "\n";
      else
  ;//      (*ss) << "int size1 = 0\n";
    }

  if (expr->lastChild)
    accept<int>(expr->lastChild);
  else
    if (outAttrIndex != -1) {
      char name[20];

      sprintf(name, "v%d", outAttrIndex);

      Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
      Type outType = current->enclosing->enclosing->locals[current->enclosing->enclosing->resolveLocal(&token)].type;
      char *callee = NULL;

      if (outType.valueType == VAL_OBJ && outType.objType) {
        switch (outType.objType->type) {
          case OBJ_COMPILER_INSTANCE:
            callee = "displayInstance";
            break;

          case OBJ_STRING:
            callee = "displayText";
            break;

          case OBJ_FUNCTION:
            insertTabs();
            (*ss) << ((ObjFunction *) outType.objType)->name->chars << "(";
            insertPoint("pos");
            (*ss) << ", ";
            insertPoint("size");
            (*ss) << ")\n";
            break;
        }

        if (callee) {
          insertTabs();
          (*ss) << callee << "(" << name << ", ";
          insertPoint("pos");
          (*ss) << ", ";
          insertPoint("size");
          (*ss) << ")\n";
        }
      }
      else
        parser.error("Out value having an illegal type");
    }

  --nTabs;
  insertTabs();
  (*ss) << "}\n";

  for (int index = 0; index < expr->attCount; index++)
    if (expr->attributes[index]->_index != -1)
;//      valueStack.pop(expr->attributes[index]->name.getString());

  outAttrIndex = oldOutAttrIndex;
}

void Resolver::onEvent(UIDirectiveExpr *expr) {/*
  int oldOutAttrIndex = outAttrIndex;

  outAttrIndex = findAttrIndex(expr, "out");

  if (!expr->previous && outAttrIndex != -1) {
    char name[20];

    sprintf(name, "v%d", outAttrIndex);

    Token token = buildToken(TOKEN_IDENTIFIER, name, strlen(name), -1);
    Type outType = current->enclosing->enclosing->locals[current->enclosing->enclosing->resolveLocal(&token)].type;

    if (outType.valueType == VAL_OBJ && outType.objType) {
      switch (outType.objType->type) {
        case OBJ_COMPILER_INSTANCE:
          insertTabs();
          (*ss) << "onInstanceEvent(" << name << ", ";
          insertPoint("pos");
          (*ss) << ")\n";
          break;

        case OBJ_STRING:
        case OBJ_FUNCTION:
          for (int index = 0; index < expr->attCount; index++) {
            UIAttributeExpr *attExpr = expr->attributes[index];

            if (!memcmp(attExpr->name.getString().c_str(), "on", 2))
              if (attExpr->handler) {
                insertTabs();
                (*ss) << "if (" << "\"onRelease\"" << " == \"" << attExpr->name.getString() << "\") {\n";
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
    else
      parser.error("Out value having an illegal type");
  }
  else {
//    accept<int>(&expr);
    int lastOffset = expr->previous->layoutIndexes[0];
    std::stringstream *oldSs = ss;
    std::stringstream ss2;

    ss = &ss2;
    nTabs++;
    accept<int>(expr, 0);
    nTabs--;
    ss = oldSs;
    ss2 << std::flush;

    if (ss2.rdbuf()->in_avail()) {
      insertTabs();
      (*ss) << "if (pos0 >= l" << lastOffset << ") {\n";
      nTabs++;
      insertTabs();
      (*ss) << "pos0 = pos0 - l" << lastOffset << "\n";
      (*ss) << ss2.str();
      --nTabs;
      insertTabs();
      (*ss) << "}\n";
      insertTabs();
      (*ss) << "else\n";
    }

    if (expr->previous->type == EXPR_UIBINARY) {
      insertTabs();
      (*ss) << "if (pos0 >= 0) {\n";
      nTabs++;
      (*ss) << ss2.str();
      --nTabs;
      insertTabs();
      (*ss) << "}\n";
    }
    else
      accept<int>(expr->previous, 0);
  }

  for (int index = 0; index < expr->attCount; index++)
    if (expr->attributes[index]->_index != -1)
;//      valueStack.pop(expr->attributes[index]->name.getString());

  outAttrIndex = oldOutAttrIndex;*/
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
    body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 2, expList, 0, NULL);
  }

  body = new BinaryExpr(condition, buildToken(TOKEN_WHILE, "while", 5, -1), body, OP_FALSE, false);

  if (initializer != NULL) {
    Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 2);

    expList[0] = initializer;
    expList[1] = body;
    body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 2, expList, 0, NULL);
  }

  return body;
}
*/