/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 */
#ifndef expr_h
#define expr_h

#include "chunk.hpp"
#include "compiler.hpp"

struct ObjFunction;

struct ObjCallable;

typedef enum {
  EXPR_REFERENCE,
  EXPR_UIATTRIBUTE,
  EXPR_UIDIRECTIVE,
  EXPR_ASSIGN,
  EXPR_BINARY,
  EXPR_GROUPING,
  EXPR_ARRAY,
  EXPR_CALL,
  EXPR_ARRAYELEMENT,
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
  EXPR_SWAP,
  EXPR_NATIVE
} ExprType;

struct Expr {
  ExprType type;

  Expr(ExprType type);

  void destroy();

  virtual void cleanExprs() = 0;
  virtual void astPrint() = 0;
  virtual Expr *toCps(K k) = 0;
  virtual Type resolve(Parser &parser) = 0;
  virtual void toCode(Parser &parser, ObjFunction *function) = 0;
};

struct ReferenceExpr : public Expr {
  Token name;
  int8_t index;
  bool upvalueFlag;
  Declaration* _declaration;

  ReferenceExpr(Token name, int8_t index, bool upvalueFlag);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct UIAttributeExpr : public Expr {
  Token name;
  Expr* handler;
  int _uiIndex;
  int _index;

  UIAttributeExpr(Token name, Expr* handler);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct UIDirectiveExpr : public Expr {
  int childDir;
  int attCount;
  UIAttributeExpr** attributes;
  UIDirectiveExpr* previous;
  UIDirectiveExpr* lastChild;
  int viewIndex;
  bool childrenViewFlag;
  int _layoutIndexes[NUM_DIRS];
  long _eventFlags;

  UIDirectiveExpr(int childDir, int attCount, UIAttributeExpr** attributes, UIDirectiveExpr* previous, UIDirectiveExpr* lastChild, int viewIndex, bool childrenViewFlag);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct AssignExpr : public Expr {
  ReferenceExpr* varExp;
  Token op;
  Expr* value;
  OpCode opCode;

  AssignExpr(ReferenceExpr* varExp, Token op, Expr* value, OpCode opCode);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct BinaryExpr : public Expr {
  Expr* left;
  Token op;
  Expr* right;
  OpCode opCode;

  BinaryExpr(Expr* left, Token op, Expr* right, OpCode opCode);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct GroupingExpr : public Expr {
  Token name;
  int count;
  Expr** expressions;
  int popLevels;
  Expr* ui;
  Compiler _compiler;

  GroupingExpr(Token name, int count, Expr** expressions, int popLevels, Expr* ui);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ArrayExpr : public Expr {
  int count;
  Expr** expressions;
  ObjFunction* function;

  ArrayExpr(int count, Expr** expressions, ObjFunction* function);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct CallExpr : public Expr {
  Expr* callee;
  Token paren;
  int count;
  Expr** arguments;
  bool newFlag;
  Expr* handler;
  ObjCallable* callable;
  ObjFunction* handlerFunction;

  CallExpr(Expr* callee, Token paren, int count, Expr** arguments, bool newFlag, Expr* handler, ObjCallable* callable, ObjFunction* handlerFunction);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ArrayElementExpr : public Expr {
  Expr* callee;
  Token bracket;
  int count;
  Expr** indexes;

  ArrayElementExpr(Expr* callee, Token bracket, int count, Expr** indexes);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct DeclarationExpr : public Expr {
  Expr* typeExpr;
  Token name;
  Expr* initExpr;
  Declaration* _declaration;

  DeclarationExpr(Expr* typeExpr, Token name, Expr* initExpr);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct FunctionExpr : public Expr {
  Expr* typeExpr;
  Token name;
  int count;
  DeclarationExpr** params;
  GroupingExpr* body;
  ObjFunction* function;
  Declaration* _declaration;

  FunctionExpr(Expr* typeExpr, Token name, int count, DeclarationExpr** params, GroupingExpr* body, ObjFunction* function);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct GetExpr : public Expr {
  Expr* object;
  Token name;
  int index;

  GetExpr(Expr* object, Token name, int index);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ListExpr : public Expr {
  int count;
  Expr** expressions;

  ListExpr(int count, Expr** expressions);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct LiteralExpr : public Expr {
  ValueType type;
  As as;

  LiteralExpr(ValueType type, As as);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct LogicalExpr : public Expr {
  Expr* left;
  Token op;
  Expr* right;

  LogicalExpr(Expr* left, Token op, Expr* right);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct OpcodeExpr : public Expr {
  OpCode op;
  Expr* right;

  OpcodeExpr(OpCode op, Expr* right);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ReturnExpr : public Expr {
  Token keyword;
  Expr* value;

  ReturnExpr(Token keyword, Expr* value);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct SetExpr : public Expr {
  Expr* object;
  Token name;
  Token op;
  Expr* value;
  int index;

  SetExpr(Expr* object, Token name, Token op, Expr* value, int index);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct StatementExpr : public Expr {
  Expr* expr;

  StatementExpr(Expr* expr);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct SuperExpr : public Expr {
  Token keyword;
  Token method;

  SuperExpr(Token keyword, Token method);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct TernaryExpr : public Expr {
  Token op;
  Expr* left;
  Expr* middle;
  Expr* right;

  TernaryExpr(Token op, Expr* left, Expr* middle, Expr* right);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ThisExpr : public Expr {
  Token keyword;

  ThisExpr(Token keyword);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct TypeExpr : public Expr {
  Expr* typeExpr;

  TypeExpr(Expr* typeExpr);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct UnaryExpr : public Expr {
  Token op;
  Expr* right;

  UnaryExpr(Token op, Expr* right);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct SwapExpr : public Expr {
  Expr* _expr;

  SwapExpr();
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct NativeExpr : public Expr {
  Token nativeCode;

  NativeExpr(Token nativeCode);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

#endif
