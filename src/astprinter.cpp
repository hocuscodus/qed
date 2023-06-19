/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <cstdarg>
#include <stdio.h>
#include "expr.hpp"

/*
void ASTPrinter::parenthesize(char *name, int numArgs, ...) {
  va_list ap;

  va_start(ap, numArgs);
  printf("(&s", name);

  while(numArgs--) {
    Expr *expr = va_arg(ap, Expr *);
    printf(" ");
    astPrint();
  }

  printf(")");
}

void ASTPrinter::parenthesize2(char *name, int numArgs, ...) {
  va_list ap;

  va_start(ap, numArgs);
  printf("(&s", name);
  transform(numArgs, ap);
  printf(")");
}

void ASTPrinter::transform(int numArgs, ...) {
  va_list ap;

  va_start(ap, numArgs);
  while(numArgs--) {
    Expr *part = va_arg(ap, Expr *);
    printf(" ");
    if (part instanceof Expr) {
      builder.append(((Expr)part).astPrint());
    } else if (part instanceof Token) {
      builder.append(((Token) part).lexeme);
    } else if (part instanceof List) {
      transform(builder, ((List) part).toArray());
    } else {
      builder.append(part);
    }
  }
}

static void print(Expr *expr) {
  astPrint();
  printf("\n");
}
*/
static void printObjType(Obj *obj) {
  switch(obj->type) {
    case OBJ_STRING: printf("String"); return;
    case OBJ_FUNCTION: {
      ObjString *name = ((ObjFunction *) obj)->name;

      printf("%.s", name->length, name->chars);
      return;
    }
    case OBJ_INSTANCE: {
      ObjString *name = ((ObjInstance *) obj)->callable->name;

      printf("%.s *", name->length, name->chars);
      return;
    }
  }
}

static void printType(Type *type) {
  switch(type->valueType) {
    case VAL_VOID:
      if (type->objType != NULL) {
        Token *token = &((ReferenceExpr *) type->objType)->name;

        printf("%.*s", token->length, token->start);
      }
      else
        printf("void");
      return;
    case VAL_BOOL: printf("bool"); return;
    case VAL_INT: printf("int"); return;
    case VAL_FLOAT: printf("float"); return;
    case VAL_OBJ: printObjType(type->objType); return;
  }
}

void AssignExpr::astPrint() {
  printf("(%.*s ", op.length, op.start);

  if (varExp)
    varExp->astPrint();

  printf(" ");

  if (value)
    value->astPrint();

  printf(")");
}

void UIAttributeExpr::astPrint() {
  printf("%.*s=(", name.length, name.start);
  if (handler)
    handler->astPrint();
  printf(")");
}

void UIDirectiveExpr::astPrint() {
  printf("(Directive(");
  for (int index = 0; index < attCount; index++)
    attributes[index]->astPrint();

  printf(")(");

  if (previous)
    previous->astPrint();

  if (lastChild)
    lastChild->astPrint();

  printf("))");
}

void BinaryExpr::astPrint() {
  printf("(%.*s ", op.length, op.start);
  left->astPrint();
  printf(" ");
  right->astPrint();
  printf(")");
}

void CallExpr::astPrint() {
//  parenthesize2("call", 2, callee, arguments);
  printf("(call ");
  callee->astPrint();
  for (int index = 0; index < count; index++) {
    printf(" ");
    arguments[index]->astPrint();
  }
  printf(")");
}

void ArrayElementExpr::astPrint() {
  printf("[index ");
  callee->astPrint();
  for (int index = 0; index < count; index++) {
    printf(" ");
    indexes[index]->astPrint();
  }
  printf("]");
}

void DeclarationExpr::astPrint() {
  printf("(var %.*s", name.length, name.start);

  if (initExpr)
    initExpr->astPrint();

  printf(")");
}

void FunctionExpr::astPrint() {
  printf("(fun %.*s(", name.length, name.start);
  for (int index = 0; index < arity; index++) {
    printf(", ");
    body->expressions[index]->astPrint();
  }
  printf(") {");
  printf("(group'%.*s' ", body->name.length, body->name.start);
  for (int index = arity; index < body->count; index++) {
    printf(", ");
    body->expressions[index]->astPrint();
    printf(";");
  }
  if (ui)
    ui->astPrint();
  printf(")");
  printf(") }");
}

void GetExpr::astPrint() {
//  return parenthesize2(".", expr.object, expr.name.lexeme);
  printf("(. ");
  object->astPrint();
  printf(" %.*s)", name.length, name.start);
}

void GroupingExpr::astPrint() {
  printf("(group'%.*s' ", name.length, name.start);
  for (int index = 0; index < count; index++) {
    printf(", ");
    expressions[index]->astPrint();
    printf(";");
  }
  printf(")");
}

void IfExpr::astPrint() {
  printf("(if ");
  condition->astPrint();
  printf(" ");
  thenBranch->astPrint();

  if (elseBranch) {
    printf(" else ");
    elseBranch->astPrint();
  }
  printf(")");
}

void ArrayExpr::astPrint() {
  printf("[");
  for (int index = 0; index < count; index++) {
    if (index) printf(", ");
    expressions[index]->astPrint();
  }
  printf("]");
}

void ListExpr::astPrint() {
  printf("(list ");
  for (int index = 0; index < count; index++) {
    printf(", ");
    expressions[index]->astPrint();
  }
  printf(")");
}

void LiteralExpr::astPrint() {
  switch (type) {
    case VAL_BOOL:
      printf("%s", as.boolean ? "true" : "false");
      break;

    case VAL_FLOAT:
      printf("%f", as.floating);
      break;

    case VAL_INT:
      printf("%ld", as.integer);
      break;

    case VAL_OBJ:
      switch (as.obj->type) {
        case OBJ_STRING:
          printf("%.*s", ((ObjString *) as.obj)->length, ((ObjString *) as.obj)->chars);
          break;

        default:
          printf("NULL");
          break;
      }
      break;

    default:
      printf("NULL");
      break;
  }
}

void LogicalExpr::astPrint() {
  printf("(%.*s ", op.length, op.start);
  left->astPrint();
  printf(" ");
  right->astPrint();
  printf(")");
}

void ReturnExpr::astPrint() {
  printf("return ");
  if (value != NULL)
    value->astPrint();
  printf(";");
}

void CastExpr::astPrint() {
}

void SetExpr::astPrint() {
  printf("(%.*s (. ", op.length, op.start);
  object->astPrint();
  printf(" %.*s) ", name.length, name.start);
  value->astPrint();
  printf(")");
}

void TernaryExpr::astPrint() {
  printf("(%.*s ", op.length, op.start);
  left->astPrint();
  printf(" ");
  middle->astPrint();
  printf(" ");
  if (right)
    right->astPrint();
  else
    printf("NULL");
  printf(")");
}

void ThisExpr::astPrint() {
  printf("this");
}

void TypeExpr::astPrint() {
  printf("(type %.*s)", name.length, name.start);
}

void UnaryExpr::astPrint() {
  printf("(%.*s ", op.length, op.start);
  right->astPrint();
  printf(")");
}

void ReferenceExpr::astPrint() {
  printf("(%s %.*s)", returnType.toString(), name.length, name.start);
}

void WhileExpr::astPrint() {
  printf("(while ");
  condition->astPrint();
  printf(" ");
  body->astPrint();
  printf(")");
}

void SwapExpr::astPrint() {
  printf("(swap)");
}

void NativeExpr::astPrint() {
  printf("<native>");
}
//< omit
/* Representing Code printer-main < Representing Code omit
public static void main(String[] args) {
  Expr expression = new Binary(
      new Unary(
          new Token(TokenType.MINUS, "-", null, 1),
          new Literal(123)),
      new Token(TokenType.STAR, "*", null, 1),
      new Grouping(
          new Literal(45.67)));

  System.out.println(new AstPrinter().print(expression));
}
*/
