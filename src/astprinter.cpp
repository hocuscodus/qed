/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <cstdarg>
#include <stdio.h>
#include "astprinter.hpp"
#include "object.hpp"

/*
void ASTPrinter::parenthesize(char *name, int numArgs, ...) {
  va_list ap;

  va_start(ap, numArgs);
  printf("(&s", name);

  while(numArgs--) {
    Expr *expr = va_arg(ap, Expr *);
    printf(" ");
    expr->accept(this);
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
      builder.append(((Expr)part).accept(this));
    } else if (part instanceof Token) {
      builder.append(((Token) part).lexeme);
    } else if (part instanceof List) {
      transform(builder, ((List) part).toArray());
    } else {
      builder.append(part);
    }
  }
}
*/
ASTPrinter::ASTPrinter() : ExprVisitor() {
}

void ASTPrinter::print(Expr *expr) {
  expr->accept(this);
  printf("\n");
}
/*
void ASTPrinter::visitBlockStmt(Expr *expr) {
  StringBuilder builder = new StringBuilder();
  builder.append("(block ");

  for (Stmt statement : stmt.statements) {
    builder.append(statement.accept(this));
  }

  builder.append(")");
  return builder.toString();
}

void ASTPrinter::visitClassStmt(Expr *expr) {
  StringBuilder builder = new StringBuilder();
  builder.append("(class " + stmt.name.lexeme);

  if (stmt.superclass != null) {
    builder.append(" < " + print(stmt.superclass));
  }

  for (Stmt.Function method : stmt.methods) {
    builder.append(" " + print(method));
  }

  builder.append(")");
  return builder.toString();
}

void ASTPrinter::visitExpressionStmt(Expr *expr) {
  return parenthesize(";", stmt.expression);
}

void ASTPrinter::visitFunctionStmt(Expr *expr) {
  StringBuilder builder = new StringBuilder();
  builder.append("(fun " + stmt.name.lexeme + "(");

  for (Token param : stmt.params) {
    if (param != stmt.params.get(0)) builder.append(" ");
    builder.append(param.lexeme);
  }

  builder.append(") ");

  for (Stmt body : stmt.body) {
    builder.append(body.accept(this));
  }

  builder.append(")");
  return builder.toString();
}

void ASTPrinter::visitIfStmt(Expr *expr) {
  if (stmt.elseBranch == null) {
    return parenthesize2("if", stmt.condition, stmt.thenBranch);
  }

  return parenthesize2("if-else", stmt.condition, stmt.thenBranch,
      stmt.elseBranch);
}

void ASTPrinter::visitPrintStmt(Expr *expr) {
  return parenthesize("print", stmt.expression);
}

void ASTPrinter::visitReturnStmt(Expr *expr) {
  if (stmt.value == null) return "(return)";
  return parenthesize("return", stmt.value);
}

void ASTPrinter::visitVarStmt(Expr *expr) {
  if (stmt.initializer == null) {
    return parenthesize2("var", stmt.name);
  }

  return parenthesize2("var", stmt.name, "=", stmt.initializer);
}

void ASTPrinter::visitWhileStmt(Expr *expr) {
  return parenthesize2("while", stmt.condition, stmt.body);
}
*/
void ASTPrinter::visitAssignExpr(AssignExpr *expr) {
  printf("(%.*s %d ", expr->op.length, expr->op.start, expr->varExp->index);

  if (expr->value)
    expr->value->accept(this);

  printf(")");
}

void ASTPrinter::visitUIAttributeExpr(UIAttributeExpr *expr) {
  printf("%.*s=(", expr->name.length, expr->name.start);
  if (expr->handler)
    accept<int>(expr->handler, 0);
  printf(")");
}

void ASTPrinter::visitUIDirectiveExpr(UIDirectiveExpr *expr) {
  printf("(Directive(");
  for (int index = 0; index < expr->attCount; index++)
    accept<int>(expr->attributes[index], 0);

  printf(")(");

  if (expr->previous)
    accept<int>(expr->previous, 0);

  if (expr->lastChild)
    accept<int>(expr->lastChild, 0);

  printf("))");
}

void ASTPrinter::visitBinaryExpr(BinaryExpr *expr) {
  printf("(%.*s ", expr->op.length, expr->op.start);
  expr->left->accept(this);
  printf(" ");
  expr->right->accept(this);
  printf(")");
}

void ASTPrinter::visitCallExpr(CallExpr *expr) {
//  parenthesize2("call", 2, expr->callee, expr->arguments);
  printf("(call ");
  expr->callee->accept(this);
  for (int index = 0; index < expr->count; index++) {
    printf(" ");
    expr->arguments[index]->accept(this);
  }
  printf(")");
}

void ASTPrinter::visitArrayElementExpr(ArrayElementExpr *expr) {
  printf("[index ");
  expr->callee->accept(this);
  for (int index = 0; index < expr->count; index++) {
    printf(" ");
    expr->indexes[index]->accept(this);
  }
  printf("]");
}

void ASTPrinter::visitDeclarationExpr(DeclarationExpr *expr) {
  printf("(var %.*s", expr->name.length, expr->name.start);
  accept<int>(expr->initExpr, 0);
  printf(")");
}

void ASTPrinter::visitFunctionExpr(FunctionExpr *expr) {
  printf("(fun %.*s", expr->name.length, expr->name.start);
  accept<int>(expr->body, 0);
  printf(")");
}

void ASTPrinter::visitGetExpr(GetExpr *expr) {
//  return parenthesize2(".", expr.object, expr.name.lexeme);
  printf("(. ");
  expr->object->accept(this);
  printf(" %.*s)", expr->name.length, expr->name.start);
}

void ASTPrinter::visitGroupingExpr(GroupingExpr *expr) {
  printf("(group'%.*s' ", expr->name.length, expr->name.start);
  for (int index = 0; index < expr->count; index++) {
    printf(", ");
    expr->expressions[index]->accept(this);
  }
  if (expr->ui)
    accept<int>(expr->ui, 0);
  printf(")");
}

void ASTPrinter::visitArrayExpr(ArrayExpr *expr) {
  printf("[");
  for (int index = 0; index < expr->count; index++) {
    if (index) printf(", ");
    expr->expressions[index]->accept(this);
  }
  printf("]");
}

void ASTPrinter::visitListExpr(ListExpr *expr) {
  printf("(list ");
  for (int index = 0; index < expr->count; index++) {
    printf(", ");
    expr->expressions[index]->accept(this);
  }
  printf(")");
}

void ASTPrinter::visitLiteralExpr(LiteralExpr *expr) {
  switch (expr->type) {
    case VAL_BOOL:
      printf("%s", expr->as.boolean ? "true" : "false");
      break;

    case VAL_FLOAT:
      printf("%f", expr->as.floating);
      break;

    case VAL_INT:
      printf("%ld", expr->as.integer);
      break;

    case VAL_OBJ:
      switch (expr->as.obj->type) {
        case OBJ_STRING:
          printf("%.*s", ((ObjString *) expr->as.obj)->length, ((ObjString *) expr->as.obj)->chars);
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

void ASTPrinter::visitLogicalExpr(LogicalExpr *expr) {
  printf("(%.*s ", expr->op.length, expr->op.start);
  expr->left->accept(this);
  printf(" ");
  expr->right->accept(this);
  printf(")");
}

void ASTPrinter::visitOpcodeExpr(OpcodeExpr *expr) {
  printf("(Op %d ", expr->op);
  if (expr->right)
    expr->right->accept(this);
  printf(")");
}

void ASTPrinter::visitReturnExpr(ReturnExpr *expr) {
  printf("return ");
  if (expr->value != NULL)
    accept<int>(expr->value, 0);
  printf(";");
}

void ASTPrinter::visitSetExpr(SetExpr *expr) {
  printf("(%.*s (. ", expr->op.length, expr->op.start);
  expr->object->accept(this);
  printf(" %.*s) ", expr->name.length, expr->name.start);
  expr->value->accept(this);
  printf(")");
}

void ASTPrinter::visitStatementExpr(StatementExpr *expr) {
  expr->expr->accept(this);
  printf(";");
}

void ASTPrinter::visitSuperExpr(SuperExpr *expr) {
//  parenthesize2("super", 1, expr->method);
}

void ASTPrinter::visitTernaryExpr(TernaryExpr *expr) {
  printf("(%.*s ", expr->op.length, expr->op.start);
  expr->left->accept(this);
  printf(" ");
  expr->middle->accept(this);
  printf(" ");
  if (expr->right)
    expr->right->accept(this);
  else
    printf("NULL");
  printf(")");
}

void ASTPrinter::visitThisExpr(ThisExpr *expr) {
  printf("this");
}

void ASTPrinter::visitTypeExpr(TypeExpr *expr) {
  printType(&expr->type);
}

void ASTPrinter::visitUnaryExpr(UnaryExpr *expr) {
  printf("(%.*s ", expr->op.length, expr->op.start);
  expr->right->accept(this);
  printf(")");
}

void ASTPrinter::visitVariableExpr(VariableExpr *expr) {
  printf("(var %d)", expr->index);
}

void ASTPrinter::visitSwapExpr(SwapExpr *expr) {
  printf("(swap)");
}

void ASTPrinter::printType(Type *type) {
  switch(type->valueType) {
    case VAL_VOID:
      if (type->objType != NULL) {
        Token *token = &((VariableExpr *) type->objType)->name;

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

void ASTPrinter::printObjType(Obj *obj) {
  switch(obj->type) {
    case OBJ_STRING: printf("String"); return;
    case OBJ_FUNCTION: {
      ObjString *name = ((ObjCallable *) obj)->name;

      printf("%.s", name->length, name->chars);
      return;
    }
    case OBJ_INSTANCE: {
      ObjString *name = ((ObjInstance *) obj)->callable->name;

      printf("%.s instance", name->length, name->chars);
      return;
    }
  }
}
//< omit
/* Representing Code printer-main < Representing Code omit
public static void main(String[] args) {
  Expr expression = new expr->Binary(
      new expr->Unary(
          new Token(TokenType.MINUS, "-", null, 1),
          new expr->Literal(123)),
      new Token(TokenType.STAR, "*", null, 1),
      new expr->Grouping(
          new expr->Literal(45.67)));

  System.out.println(new AstPrinter().print(expression));
}
*/
