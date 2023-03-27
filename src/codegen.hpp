/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_codegen_h
#define qed_codegen_h

#include "compiler.hpp"
#include "chunk.hpp"

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
  void visitVariableExpr(VariableExpr *expr);
  void visitSwapExpr(SwapExpr *expr);

  Chunk *currentChunk();
  void emitCode(Expr *expr);

  void emitByte(uint8_t byte);
  void emitBytes(uint8_t byte1, uint8_t byte2);
  void emitLoop(int loopStart);
  int emitJump(uint8_t instruction);
  void emitHalt();
  uint8_t makeConstant(Value value);
  void emitConstant(Value value);
  void patchJump(int offset);
  void endCompiler();
};

#endif
