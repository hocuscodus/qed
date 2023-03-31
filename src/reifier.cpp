/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */

#include <stack>
#include "reifier.hpp"
#include "object.hpp"
#include "memory.h"

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
  reinferStack.push({&expr->_compiler, expr->_compiler.inBlock() ? top()->localStart : 0});

  for (int index = 0; index < expr->count; index++)
    expr->expressions[index]->accept(this);

  reinferStack.pop();
/*
  if (!expr->_compiler.inBlock()) {
    ObjFunction *function = expr->_compiler.function;

    if (expr->_compiler.fieldCount) {
      function->fields = ALLOCATE(Field, expr->_compiler.fieldCount);

      for (int count = 0, index = 0; index < expr->_compiler.getDeclarationCount(); index++) {
        Declaration *dec = &expr->_compiler.getDeclaration(index);

        if (dec->isField) {
          function->fields[count].type = dec->type;
          function->fields[count++].name = copyString(dec->name.start, dec->name.length);
        }
      }
    }
  }*/
}

void Reifier::visitArrayExpr(ArrayExpr *expr) {
  for (int index = 0; index < expr->count; index++) {
    expr->expressions[index]->accept(this);
  }
}

void Reifier::visitListExpr(ListExpr *expr) {
  Declaration *dec = expr->_declaration;

  if (dec) {
    dec->realIndex = dec->isField ? top()->compiler->fieldCount++ : top()->localStart++;

    if (IS_FUNCTION(dec->type)) {
      ObjFunction *function = AS_FUNCTION_TYPE(dec->type);
#ifdef NO_CLOSURE
      for (int i = 0; i < function->upvalueCount; i++) {
        Upvalue *upvalue = &function->upvalues[i];

        if (upvalue->isField) {
          Declaration *dec2 = &top()->compiler->declarations[upvalue->index];

          upvalue->index = dec2->realIndex;
        }
      }
#endif
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
