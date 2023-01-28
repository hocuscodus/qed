/*
 * The QED Programming Language
 * Copyright (C) 2022  Hocus Codus Software inc.
 */
#ifndef expr_h
#define expr_h

#include "chunk.hpp"

#include "object.hpp"
#include "scanner.hpp"

typedef enum {
  EXPR_VARIABLE,
  EXPR_UIATTRIBUTE,
  EXPR_UIDIRECTIVE,
  EXPR_ASSIGN,
  EXPR_BINARY,
  EXPR_GROUPING,
  EXPR_CALL,
  EXPR_DECLARATION,
  EXPR_FUNCTION,
  EXPR_GET,
  EXPR_LIST,
  EXPR_LITERAL,
  EXPR_LOGICAL,
  EXPR_OPCODE,
  EXPR_RETURN,
  EXPR_SET,
  EXPR_STATEMENT,
  EXPR_SUPER,
  EXPR_TERNARY,
  EXPR_THIS,
  EXPR_TYPE,
  EXPR_UNARY,
  EXPR_SWAP
} ExprType;

struct ExprVisitor;

struct Expr {
  ExprType type;

  Expr(ExprType type);

  virtual void accept(ExprVisitor *visitor) = 0;
};

struct VariableExpr;
struct UIAttributeExpr;
struct UIDirectiveExpr;
struct AssignExpr;
struct BinaryExpr;
struct GroupingExpr;
struct CallExpr;
struct DeclarationExpr;
struct FunctionExpr;
struct GetExpr;
struct ListExpr;
struct LiteralExpr;
struct LogicalExpr;
struct OpcodeExpr;
struct ReturnExpr;
struct SetExpr;
struct StatementExpr;
struct SuperExpr;
struct TernaryExpr;
struct ThisExpr;
struct TypeExpr;
struct UnaryExpr;
struct SwapExpr;

struct ExprVisitor {
  template <typename T> T accept(Expr *expr, T buf = T()) {
    static T _buf;

    _buf = buf;

    if (expr != NULL)
      expr->accept(this);

    return _buf;
  }

  template <typename T> void set(T buf) {
    accept(NULL, buf);
  }

  virtual void visitVariableExpr(VariableExpr *expr) = 0;
  virtual void visitUIAttributeExpr(UIAttributeExpr *expr) = 0;
  virtual void visitUIDirectiveExpr(UIDirectiveExpr *expr) = 0;
  virtual void visitAssignExpr(AssignExpr *expr) = 0;
  virtual void visitBinaryExpr(BinaryExpr *expr) = 0;
  virtual void visitGroupingExpr(GroupingExpr *expr) = 0;
  virtual void visitCallExpr(CallExpr *expr) = 0;
  virtual void visitDeclarationExpr(DeclarationExpr *expr) = 0;
  virtual void visitFunctionExpr(FunctionExpr *expr) = 0;
  virtual void visitGetExpr(GetExpr *expr) = 0;
  virtual void visitListExpr(ListExpr *expr) = 0;
  virtual void visitLiteralExpr(LiteralExpr *expr) = 0;
  virtual void visitLogicalExpr(LogicalExpr *expr) = 0;
  virtual void visitOpcodeExpr(OpcodeExpr *expr) = 0;
  virtual void visitReturnExpr(ReturnExpr *expr) = 0;
  virtual void visitSetExpr(SetExpr *expr) = 0;
  virtual void visitStatementExpr(StatementExpr *expr) = 0;
  virtual void visitSuperExpr(SuperExpr *expr) = 0;
  virtual void visitTernaryExpr(TernaryExpr *expr) = 0;
  virtual void visitThisExpr(ThisExpr *expr) = 0;
  virtual void visitTypeExpr(TypeExpr *expr) = 0;
  virtual void visitUnaryExpr(UnaryExpr *expr) = 0;
  virtual void visitSwapExpr(SwapExpr *expr) = 0;
};

struct VariableExpr : public Expr {
  Token name;
  int8_t index;
  bool upvalueFlag;

  VariableExpr(Token name, int8_t index, bool upvalueFlag);
  void accept(ExprVisitor *visitor);
};

struct UIAttributeExpr : public Expr {
  Token name;
  Expr* handler;
  int _uiIndex;
  int _index;

  UIAttributeExpr(Token name, Expr* handler);
  void accept(ExprVisitor *visitor);
};

struct UIDirectiveExpr : public Expr {
  int childDir;
  int attCount;
  UIAttributeExpr** attributes;
  UIDirectiveExpr* previous;
  UIDirectiveExpr* lastChild;
  int viewIndex;
  int _layoutIndexes[NUM_DIRS];
  long _eventFlags;

  UIDirectiveExpr(int childDir, int attCount, UIAttributeExpr** attributes, UIDirectiveExpr* previous, UIDirectiveExpr* lastChild, int viewIndex);
  void accept(ExprVisitor *visitor);
};

struct AssignExpr : public Expr {
  VariableExpr* varExp;
  Token op;
  Expr* value;

  AssignExpr(VariableExpr* varExp, Token op, Expr* value);
  void accept(ExprVisitor *visitor);
};

struct BinaryExpr : public Expr {
  Expr* left;
  Token op;
  Expr* right;
  OpCode opCode;
  bool notFlag;

  BinaryExpr(Expr* left, Token op, Expr* right, OpCode opCode, bool notFlag);
  void accept(ExprVisitor *visitor);
};

struct GroupingExpr : public Expr {
  Token name;
  int count;
  Expr** expressions;
  int popLevels;
  Expr* ui;

  GroupingExpr(Token name, int count, Expr** expressions, int popLevels, Expr* ui);
  void accept(ExprVisitor *visitor);
};

struct CallExpr : public Expr {
  Expr* callee;
  Token paren;
  int count;
  Expr** arguments;
  bool newFlag;
  Expr* handler;

  CallExpr(Expr* callee, Token paren, int count, Expr** arguments, bool newFlag, Expr* handler);
  void accept(ExprVisitor *visitor);
};

struct DeclarationExpr : public Expr {
  Type type;
  Token name;
  Expr* initExpr;

  DeclarationExpr(Type type, Token name, Expr* initExpr);
  void accept(ExprVisitor *visitor);
};

struct FunctionExpr : public Expr {
  Type type;
  Token name;
  int count;
  Expr** params;
  Expr* body;
  ObjFunction* function;

  FunctionExpr(Type type, Token name, int count, Expr** params, Expr* body, ObjFunction* function);
  void accept(ExprVisitor *visitor);
};

struct GetExpr : public Expr {
  Expr* object;
  Token name;
  int index;

  GetExpr(Expr* object, Token name, int index);
  void accept(ExprVisitor *visitor);
};

struct ListExpr : public Expr {
  int count;
  Expr** expressions;
  ExprType listType;
  ObjFunction* function;

  ListExpr(int count, Expr** expressions, ExprType listType, ObjFunction* function);
  void accept(ExprVisitor *visitor);
};

struct LiteralExpr : public Expr {
  ValueType type;
  As as;

  LiteralExpr(ValueType type, As as);
  void accept(ExprVisitor *visitor);
};

struct LogicalExpr : public Expr {
  Expr* left;
  Token op;
  Expr* right;

  LogicalExpr(Expr* left, Token op, Expr* right);
  void accept(ExprVisitor *visitor);
};

struct OpcodeExpr : public Expr {
  OpCode op;
  Expr* right;

  OpcodeExpr(OpCode op, Expr* right);
  void accept(ExprVisitor *visitor);
};

struct ReturnExpr : public Expr {
  Token keyword;
  Expr* value;

  ReturnExpr(Token keyword, Expr* value);
  void accept(ExprVisitor *visitor);
};

struct SetExpr : public Expr {
  Expr* object;
  Token name;
  Token op;
  Expr* value;
  int index;

  SetExpr(Expr* object, Token name, Token op, Expr* value, int index);
  void accept(ExprVisitor *visitor);
};

struct StatementExpr : public Expr {
  Expr* expr;

  StatementExpr(Expr* expr);
  void accept(ExprVisitor *visitor);
};

struct SuperExpr : public Expr {
  Token keyword;
  Token method;

  SuperExpr(Token keyword, Token method);
  void accept(ExprVisitor *visitor);
};

struct TernaryExpr : public Expr {
  Token op;
  Expr* left;
  Expr* middle;
  Expr* right;

  TernaryExpr(Token op, Expr* left, Expr* middle, Expr* right);
  void accept(ExprVisitor *visitor);
};

struct ThisExpr : public Expr {
  Token keyword;

  ThisExpr(Token keyword);
  void accept(ExprVisitor *visitor);
};

struct TypeExpr : public Expr {
  Type type;

  TypeExpr(Type type);
  void accept(ExprVisitor *visitor);
};

struct UnaryExpr : public Expr {
  Token op;
  Expr* right;

  UnaryExpr(Token op, Expr* right);
  void accept(ExprVisitor *visitor);
};

struct SwapExpr : public Expr {
  Expr* _expr;

  SwapExpr();
  void accept(ExprVisitor *visitor);
};

#endif
