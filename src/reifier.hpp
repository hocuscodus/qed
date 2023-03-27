/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_reifier_h
#define qed_reifier_h

#include "parser.hpp"

/*
 * The palindrome class...
 */
class Reifier : public ExprVisitor {
  Parser &parser;
public:
  Reifier(Parser &parser);

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
  void visitStatementExpr(StatementExpr *expr);
  void visitSetExpr(SetExpr *expr);
  void visitSuperExpr(SuperExpr *expr);
  void visitTernaryExpr(TernaryExpr *expr);
  void visitThisExpr(ThisExpr *expr);
  void visitTypeExpr(TypeExpr *expr);
  void visitUnaryExpr(UnaryExpr *expr);
  void visitVariableExpr(VariableExpr *expr);
  void visitSwapExpr(SwapExpr *expr);

  bool reify();
};

#endif


