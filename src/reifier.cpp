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

#include "reifier.hpp"
#include "object.hpp"

Reifier::Reifier(Parser &parser) : ExprVisitor(), parser(parser) {
}

void Reifier::visitAssignExpr(AssignExpr *expr) {
  if (expr->value)
    expr->value->accept(this);
}

void Reifier::visitUIAttributeExpr(UIAttributeExpr *expr) {
}

void Reifier::visitUIDirectiveExpr(UIDirectiveExpr *expr) {
}

void Reifier::visitBinaryExpr(BinaryExpr *expr) {
  expr->left->accept(this);
  expr->right->accept(this);
}

void Reifier::visitCallExpr(CallExpr *expr) {
  expr->callee->accept(this);
  for (int index = 0; index < expr->count; index++) {
    expr->arguments[index]->accept(this);
  }
}

void Reifier::visitArrayElementExpr(ArrayElementExpr *expr) {
  expr->callee->accept(this);
  for (int index = 0; index < expr->count; index++) {
    expr->indexes[index]->accept(this);
  }
}

void Reifier::visitDeclarationExpr(DeclarationExpr *expr) {
  accept<int>(expr->initExpr, 0);
}

void Reifier::visitFunctionExpr(FunctionExpr *expr) {
  accept<int>(expr->body, 0);
}

void Reifier::visitGetExpr(GetExpr *expr) {
  expr->object->accept(this);
}

void Reifier::visitGroupingExpr(GroupingExpr *expr) {
  int fieldIndex = 0;
  int localIndex = 0;

  for (int index = 0; index < expr->_compiler.localCount; index++) {
    Local *local = &expr->_compiler.locals[expr->_compiler.localCount + index];
    ObjFunction *function = AS_FUNCTION_TYPE(local->type);

    local->realIndex = local->isField ? fieldIndex++ : localIndex++;

    if (function)
      for (int i = 0; i < function->upvalueCount; i++) {
        Upvalue *upvalue = &function->upvalues[i];

        if (upvalue->isLocal) {
          Local &local = expr->_compiler.locals[upvalue->index];

          if (!local.isField)
            upvalue->index = upvalue->index;

          upvalue->index = upvalue->index;//local.realIndex;
        }
      }
  }

  for (int index = 0; index < expr->count; index++) {
    expr->expressions[index]->accept(this);
  }
  if (expr->ui)
    accept<int>(expr->ui, 0);
}

void Reifier::visitArrayExpr(ArrayExpr *expr) {
  for (int index = 0; index < expr->count; index++) {
    expr->expressions[index]->accept(this);
  }
}

void Reifier::visitListExpr(ListExpr *expr) {
  for (int index = 0; index < expr->count; index++) {
    expr->expressions[index]->accept(this);
  }
}

void Reifier::visitLiteralExpr(LiteralExpr *expr) {
}

void Reifier::visitLogicalExpr(LogicalExpr *expr) {
  expr->left->accept(this);
  expr->right->accept(this);
}

void Reifier::visitOpcodeExpr(OpcodeExpr *expr) {
  if (expr->right)
    expr->right->accept(this);
}

void Reifier::visitReturnExpr(ReturnExpr *expr) {
  if (expr->value != NULL)
    accept<int>(expr->value, 0);
}

void Reifier::visitSetExpr(SetExpr *expr) {
  expr->object->accept(this);
  expr->value->accept(this);
}

void Reifier::visitStatementExpr(StatementExpr *expr) {
  expr->expr->accept(this);
}

void Reifier::visitSuperExpr(SuperExpr *expr) {
}

void Reifier::visitTernaryExpr(TernaryExpr *expr) {
  expr->left->accept(this);
  expr->middle->accept(this);

  if (expr->right)
    expr->right->accept(this);
}

void Reifier::visitThisExpr(ThisExpr *expr) {
}

void Reifier::visitTypeExpr(TypeExpr *expr) {
}

void Reifier::visitUnaryExpr(UnaryExpr *expr) {
  expr->right->accept(this);
}

void Reifier::visitVariableExpr(VariableExpr *expr) {
}

void Reifier::visitSwapExpr(SwapExpr *expr) {
}

bool Reifier::reify() {
  accept(parser.expr, 0);
  return !parser.hadError;
}
