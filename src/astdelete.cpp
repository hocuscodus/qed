/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <cstdarg>
#include <stdio.h>
#include "expr.hpp"


void AssignExpr::cleanExprs() {
}

void UIAttributeExpr::cleanExprs() {
}

void UIDirectiveExpr::cleanExprs() {
}

void BinaryExpr::cleanExprs() {
}

void CallExpr::cleanExprs() {
}

void ArrayElementExpr::cleanExprs() {
}

void DeclarationExpr::cleanExprs() {
}

void FunctionExpr::cleanExprs() {
}

void GetExpr::cleanExprs() {
}

void GroupingExpr::cleanExprs() {
}

void ArrayExpr::cleanExprs() {
}

void ListExpr::cleanExprs() {
}

void LiteralExpr::cleanExprs() {
}

void LogicalExpr::cleanExprs() {
}

void OpcodeExpr::cleanExprs() {
}

void ReturnExpr::cleanExprs() {
}

void SetExpr::cleanExprs() {
}

void StatementExpr::cleanExprs() {
  if (expr)
    expr->destroy();
}

void SuperExpr::cleanExprs() {
}

void TernaryExpr::cleanExprs() {
}

void ThisExpr::cleanExprs() {
}

void TypeExpr::cleanExprs() {
}

void UnaryExpr::cleanExprs() {
}

void ReferenceExpr::cleanExprs() {
}

void SwapExpr::cleanExprs() {
}

void NativeExpr::cleanExprs() {
}
