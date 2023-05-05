/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_codegen_h
#define qed_codegen_h

#include "parser.hpp"

class CodeGenerator : public ExprVisitor {
  Parser &parser;
  ObjFunction *function;
public:
  CodeGenerator(Parser &parser, ObjFunction *function);

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
  void visitReferenceExpr(ReferenceExpr *expr);
  void visitSwapExpr(SwapExpr *expr);
  void visitNativeExpr(NativeExpr *expr);

  std::stringstream &str();
  std::stringstream &line();
  void startBlock();
  void endBlock();
};

#endif
