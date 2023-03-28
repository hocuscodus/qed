/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */

#include <stack>
#include "reifier.hpp"
#include "object.hpp"

typedef struct {
  Compiler *compiler;
  int localStart;
} ReinferData;

static std::stack<ReinferData> reinferStack;

ReinferData *top() {
  return reinferStack.empty() ? NULL : &reinferStack.top();
}

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
  int localStart = top() ? top()->localStart : 0;

  reinferStack.push({&expr->_compiler, localStart});
  top()->compiler->localStart = localStart;

  for (int index = 0; index < expr->count; index++)
    expr->expressions[index]->accept(this);

  reinferStack.pop();
}

void Reifier::visitArrayExpr(ArrayExpr *expr) {
  for (int index = 0; index < expr->count; index++) {
    expr->expressions[index]->accept(this);
  }
}

void Reifier::visitListExpr(ListExpr *expr) {
  Declaration *dec = expr->_declaration.type.valueType != VAL_VOID ? &expr->_declaration : NULL;

  if (dec) {
    dec->realIndex = dec->isField ? top()->compiler->fieldCount++ : top()->compiler->localStart + top()->compiler->localCount++;

    if (!dec->isField)
      top()->localStart = dec->realIndex + 1;

    if (IS_FUNCTION(dec->type)) {
      ObjFunction *function = AS_FUNCTION_TYPE(dec->type);

      for (int i = 0; i < function->upvalueCount; i++) {
        Upvalue *upvalue = &function->upvalues[i];

        if (upvalue->isLocal) {
          Declaration &dec = top()->compiler->declarations[upvalue->index];

          if (!dec.isField)
            upvalue->index = upvalue->index;

          upvalue->index = upvalue->index;//local.realIndex;
        }
      }
    }
  }

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

void Reifier::visitReferenceExpr(ReferenceExpr *expr) {
}

void Reifier::visitSwapExpr(SwapExpr *expr) {
}

bool Reifier::reify() {
  accept(parser.expr, 0);
  return !parser.hadError;
}
