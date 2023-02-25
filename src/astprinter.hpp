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
#ifndef qed_astprinter_h
#define qed_astprinter_h

#include "expr.hpp"

class ASTPrinter : public ExprVisitor {/*
  void parenthesize(char *name, int num_args, ...);
  void parenthesize2(char *name, int numArgs, ...);
  void transform(int numArgs, ...);*/
public:
  ASTPrinter();

  void visitAssignExpr(AssignExpr *expr);
  void visitUIAttributeExpr(UIAttributeExpr *expr);
  void visitUIDirectiveExpr(UIDirectiveExpr *expr);
  void visitBinaryExpr(BinaryExpr *expr);
  void visitCallExpr(CallExpr *expr);
  void visitArrayElementExpr(ArrayElementExpr *expr);
  void visitDeclarationExpr(DeclarationExpr *expr);
  void visitFunctionExpr(FunctionExpr *expr);
  void visitGetExpr(GetExpr *expr);
  void visitGroupingExpr(GroupingExpr *expr);
  void visitArrayExpr(ArrayExpr *expr);
  void visitListExpr(ListExpr *expr);
  void visitLiteralExpr(LiteralExpr *expr);
  void visitLogicalExpr(LogicalExpr *expr);
  void visitOpcodeExpr(OpcodeExpr *expr);
  void visitReturnExpr(ReturnExpr *expr);
  void visitSetExpr(SetExpr *expr);
  void visitStatementExpr(StatementExpr *expr);
  void visitSuperExpr(SuperExpr *expr);
  void visitTernaryExpr(TernaryExpr *expr);
  void visitThisExpr(ThisExpr *expr);
  void visitTypeExpr(TypeExpr *expr);
  void visitUnaryExpr(UnaryExpr *expr);
  void visitVariableExpr(VariableExpr *expr);
  void visitSwapExpr(SwapExpr *expr);

  void print(Expr *expr);
  void printType(Type *expr);
  void printObjType(Obj *obj);
};

#endif


