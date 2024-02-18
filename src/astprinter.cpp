/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include "compiler.hpp"

static void printObjType(Obj *obj) {
  switch(obj->type) {
    case OBJ_STRING: fprintf(stderr, "String"); return;
    case OBJ_FUNCTION: {
      Token name = ((ObjFunction *) obj)->expr->_declaration.name;

      fprintf(stderr, "%.s", name.length, name.start);
      return;
    }
    case OBJ_INSTANCE:
      printObjType(&((ObjInstance *) obj)->callable->obj);
      fprintf(stderr, " *");
      return;
  }
}

static void printType(Type *type) {
  switch(type->valueType) {
    case VAL_VOID:
      if (type->objType != NULL) {
        Token *token = &((ReferenceExpr *) type->objType)->name;

        fprintf(stderr, "%.*s", token->length, token->start);
      }
      else
        fprintf(stderr, "void");
      return;
    case VAL_BOOL: fprintf(stderr, "bool"); return;
    case VAL_INT: fprintf(stderr, "int"); return;
    case VAL_FLOAT: fprintf(stderr, "float"); return;
    case VAL_OBJ: printObjType(type->objType); return;
  }
}

void IteratorExpr::astPrint() {
}

void AssignExpr::astPrint() {
  fprintf(stderr, "(%.*s ", op.length, op.start);

  if (varExp)
    varExp->astPrint();

  fprintf(stderr, " ");

  if (value)
    value->astPrint();

  fprintf(stderr, ")");
}

void UIAttributeExpr::astPrint() {
  fprintf(stderr, "%.*s=(", name.length, name.start);
  if (handler)
    handler->astPrint();
  fprintf(stderr, ")");
}

void UIDirectiveExpr::astPrint() {
  fprintf(stderr, "(Directive(");
  for (int index = 0; index < attCount; index++)
    attributes[index]->astPrint();

  fprintf(stderr, ")(");

  if (previous)
    previous->astPrint();

  if (lastChild)
    lastChild->astPrint();

  fprintf(stderr, "))");
}

void BinaryExpr::astPrint() {
  switch (op.type) {
    case TOKEN_SEPARATOR:
    case TOKEN_COMMA:
      left->astPrint();
      fprintf(stderr, op.type == TOKEN_SEPARATOR ? " " : ", ");
      right->astPrint();
      break;

    default:
      fprintf(stderr, "(%.*s ", op.length, op.start);
      left->astPrint();
      fprintf(stderr, " ");
      right->astPrint();
      fprintf(stderr, ")");
      break;
  }
}

void CallExpr::astPrint() {
//  parenthesize2("call", 2, callee, arguments);
  fprintf(stderr, "(call ");
  callee->astPrint();

  if (args) {
    fprintf(stderr, " ");
    args->astPrint();
  }

  fprintf(stderr, ")");
}

void ArrayElementExpr::astPrint() {
  fprintf(stderr, "[index ");
  callee->astPrint();
  for (int index = 0; index < count; index++) {
    fprintf(stderr, " ");
    indexes[index]->astPrint();
  }
  fprintf(stderr, "]");
}

void DeclarationExpr::astPrint() {
  fprintf(stderr, "(%s %.*s", _declaration.type.toString(), _declaration.name.length, _declaration.name.start);

  if (initExpr)
    initExpr->astPrint();

  fprintf(stderr, ")");
}

void FunctionExpr::astPrint() {
  fprintf(stderr, "fun %.*s(", _declaration.name.length, _declaration.name.start);
  for (int index = 0; index < arity; index++) {
    if (index)
      fprintf(stderr, ", ");

    getParam(this, index)->astPrint();
  }
  fprintf(stderr, ") {");

  if (body)
    body->astPrint();

  if (ui)
;//    ui->astPrint();

  fprintf(stderr, "}");
}

void GetExpr::astPrint() {
//  return parenthesize2(".", expr.object, expr.name.lexeme);
  fprintf(stderr, "(. ");
  object->astPrint();
  fprintf(stderr, " %.*s)", name.length, name.start);
}

void GroupingExpr::astPrint() {
  fprintf(stderr, "(group'%.*s' ", name.length, name.start);

  if (body)
    body->astPrint();

  fprintf(stderr, ")");
}

void IfExpr::astPrint() {
  fprintf(stderr, "(if ");
  condition->astPrint();
  fprintf(stderr, " ");
  thenBranch->astPrint();

  if (elseBranch) {
    fprintf(stderr, " else ");
    elseBranch->astPrint();
  }
  fprintf(stderr, ")");
}

void ArrayExpr::astPrint() {
  fprintf(stderr, "[");

  if (body)
    body->astPrint();

  fprintf(stderr, "]");
}

void LiteralExpr::astPrint() {
  switch (type) {
    case VAL_BOOL:
      fprintf(stderr, "%s", as.boolean ? "true" : "false");
      break;

    case VAL_FLOAT:
      fprintf(stderr, "%f", as.floating);
      break;

    case VAL_INT:
      fprintf(stderr, "%ld", as.integer);
      break;

    case VAL_OBJ:
      switch (as.obj->type) {
        case OBJ_STRING:
          fprintf(stderr, "%s", ((ObjString *) as.obj)->str);
          break;

        default:
          fprintf(stderr, "NULL");
          break;
      }
      break;

    default:
      fprintf(stderr, "NULL");
      break;
  }
}

void LogicalExpr::astPrint() {
  fprintf(stderr, "(%.*s ", op.length, op.start);
  left->astPrint();
  fprintf(stderr, " ");
  right->astPrint();
  fprintf(stderr, ")");
}

void ReturnExpr::astPrint() {
  fprintf(stderr, "return");
  if (value) {
    fprintf(stderr, " ");
    value->astPrint();
  }
  fprintf(stderr, ";");
}

void CastExpr::astPrint() {
  fprintf(stderr, "(");
  printType(&type);
  fprintf(stderr, ") ");
  expr->astPrint();
}

void SetExpr::astPrint() {
  fprintf(stderr, "(%.*s (. ", op.length, op.start);
  object->astPrint();
  fprintf(stderr, " %.*s) ", name.length, name.start);
  value->astPrint();
  fprintf(stderr, ")");
}

void TernaryExpr::astPrint() {
  fprintf(stderr, "(%.*s ", op.length, op.start);
  left->astPrint();
  fprintf(stderr, " ");
  middle->astPrint();
  fprintf(stderr, " ");
  if (right)
    right->astPrint();
  else
    fprintf(stderr, "NULL");
  fprintf(stderr, ")");
}

void ThisExpr::astPrint() {
  fprintf(stderr, "this");
}

void UnaryExpr::astPrint() {
  fprintf(stderr, "(%.*s ", op.length, op.start);
  right->astPrint();
  fprintf(stderr, ")");
}

void PrimitiveExpr::astPrint() {
  fprintf(stderr, "(%.*s)", name.length, name.start);
}

void ReferenceExpr::astPrint() {
  fprintf(stderr, "(%s %.*s)", declaration ? getDeclarationType(declaration).toString() : "var", name.length, name.start);
}

void WhileExpr::astPrint() {
  fprintf(stderr, "(while ");
  condition->astPrint();
  fprintf(stderr, " ");
  body->astPrint();
  fprintf(stderr, ")");
}

void SwapExpr::astPrint() {
  fprintf(stderr, "(swap)");
}

void NativeExpr::astPrint() {
  fprintf(stderr, "<native>");
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
