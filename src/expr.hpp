/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * This code has been generated using astgenerator.cpp.
 * Do not manually modify it.
 */
#ifndef expr_h
#define expr_h

#include "chunk.hpp"
#include "compiler.hpp"

struct ObjFunction;

struct ObjCallable;

typedef enum {
  EXPR_TYPE,
  EXPR_UIATTRIBUTE,
  EXPR_UIDIRECTIVE,
  EXPR_ASSIGN,
  EXPR_BINARY,
  EXPR_CAST,
  EXPR_GROUPING,
  EXPR_ARRAY,
  EXPR_CALL,
  EXPR_ARRAYELEMENT,
  EXPR_DECLARATION,
  EXPR_FUNCTION,
  EXPR_GET,
  EXPR_IF,
  EXPR_LIST,
  EXPR_LITERAL,
  EXPR_LOGICAL,
  EXPR_REFERENCE,
  EXPR_RETURN,
  EXPR_SET,
  EXPR_TERNARY,
  EXPR_THIS,
  EXPR_UNARY,
  EXPR_WHILE,
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

struct TypeExpr : public Expr {
  Token name;
  bool functionFlag;
  bool noneFlag;
  int numDim;
  int index;
  Declaration* declaration;

  TypeExpr(Token name, bool functionFlag, bool noneFlag, int numDim, int index, Declaration* declaration);
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
  Expr* varExp;
  Token op;
  Expr* value;

  AssignExpr(Expr* varExp, Token op, Expr* value);
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

  BinaryExpr(Expr* left, Token op, Expr* right);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct CastExpr : public Expr {
  Expr* typeExpr;
  Expr* expr;
  Type _srcType;
  Type _dstType;

  CastExpr(Expr* typeExpr, Expr* expr);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct GroupingExpr : public Expr {
  Token name;
  Expr* body;
  Compiler _compiler;

  GroupingExpr(Token name, Expr* body);
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
  bool newFlag;
  Expr* callee;
  Token paren;
  Expr* params;
  Expr* handler;

  CallExpr(bool newFlag, Expr* callee, Token paren, Expr* params, Expr* handler);
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
  Type returnType;
  Token name;
  int arity;
  DeclarationExpr** params;
  GroupingExpr* body;
  Expr* ui;
  ObjFunction _function;
  Declaration* _declaration;

  FunctionExpr(Type returnType, Token name, int arity, DeclarationExpr** params, GroupingExpr* body, Expr* ui);
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
  Declaration* _declaration;

  GetExpr(Expr* object, Token name, int index);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct IfExpr : public Expr {
  Expr* condition;
  Expr* thenBranch;
  Expr* elseBranch;

  IfExpr(Expr* condition, Expr* thenBranch, Expr* elseBranch);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ListExpr : public Expr {
  Expr* expressions;

  ListExpr(Expr* expressions);
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

struct ReferenceExpr : public Expr {
  Token name;
  Type returnType;
  Declaration* _declaration;

  ReferenceExpr(Token name, Type returnType);
  void cleanExprs();
  void astPrint();
  Expr *toCps(K k);
  Type resolve(Parser &parser);
  void toCode(Parser &parser, ObjFunction *function);
};

struct ReturnExpr : public Expr {
  Token keyword;
  bool isUserClass;
  Expr* value;

  ReturnExpr(Token keyword, bool isUserClass, Expr* value);
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

struct WhileExpr : public Expr {
  Expr* condition;
  Expr* body;

  WhileExpr(Expr* condition, Expr* body);
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
