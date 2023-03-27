/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_parser_h
#define qed_parser_h

#include "scanner.hpp"
#include "expr.hpp"

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_TERNARY,     // or
  PREC_LOGICAL_OR,  // or
  PREC_LOGICAL_AND, // and
  PREC_BITWISE_OR,  // or
  PREC_BITWISE_XOR, // xor
  PREC_BITWISE_AND, // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_SHIFT,       // >> << >>>
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! - ~
  PREC_CALL,        // ()
  PREC_MEMBER,      // .
  PREC_PRIMARY
} Precedence;

class Parser;

typedef Expr *(Parser::*UnaryExpFn)();
typedef Expr *(Parser::*BinaryExpFn)(Expr *left);

typedef struct {
  UnaryExpFn prefix;
  BinaryExpFn infix;
  Precedence precedence;
} ParseExpRule;

extern ParseExpRule expRules[];
extern Type stringType;

ParseExpRule *getExpRule(TokenType type);

class Parser {
  Scanner &scanner;
  Token current;
  Token previous;
  bool panicMode;
  uint64_t statementExprs = 0L; // Helper booleans in groups
public:
  bool hadError;
  GroupingExpr *expr;
public:
  Parser(Scanner &scanner);
private:
  void advance();
public:
  void consume(TokenType type, const char *message);
  bool check(TokenType type);
  bool check(TokenType *endGroupTypes);
  bool match(TokenType type);
  void errorAtCurrent(const char *fmt, ...);
  void error(const char *fmt, ...);
  void compilerError(const char *fmt, ...);
  void errorAt(Token *token, const char *fmt, ...);
////
  ObjFunction *compile();
  bool parse();
  void passSeparator();

  Expr *suffix(Expr *left);
  Expr *assignment(Expr *left);
  Expr *binary(Expr *left);
  Expr *binaryOrPostfix(Expr *left);
  Expr *ternary(Expr *left);
  uint8_t argumentList();
  Expr *dot(Expr *left);
  Expr *call(Expr *left);
  Expr *arrayElement(Expr *left);
  Expr *logical(Expr *left);
  Expr *literal();
  Expr *swap();
  Expr *primitiveType();
  UIAttributeExpr *attribute(TokenType endGroupType);
  UIDirectiveExpr *directive(TokenType endGroupType, UIDirectiveExpr *previous);
  Expr *grouping();
  Expr *grouping(TokenType endGroupType, const char *errorMessage);
  Expr *array();
  Expr *floatNumber();
  Expr *intNumber();
  Expr *string();
  Expr *variable();
  Expr *unary();
  Expr *parsePrecedence(Precedence precedence);
  Expr *declareVariable(Type &type, TokenType endGroupType);
  Expr *parseVariable(TokenType endGroupType, const char *errorMessage);
  Expr *expression(TokenType *endGroupType);
  Expr *function(Type *type, TokenType endGroupType);
  Type readType();
  Expr *funDeclaration(TokenType endGroupType);
  Expr *varDeclaration(TokenType endGroupType);
  Expr *expressionStatement(TokenType endGroupType);
  Expr *forStatement(TokenType endGroupType);
  Expr *ifStatement(TokenType endGroupType);
  Expr *whileStatement(TokenType endGroupType);
  Expr *printStatement(TokenType endGroupType);
  Expr *declaration(TokenType endGroupType);
  Expr *returnStatement(TokenType endGroupType);
  Expr *statement(TokenType endGroupType);
  void synchronize();
};

#endif


