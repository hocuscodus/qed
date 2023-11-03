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
  PREC_ITERATOR,    // :\ :: :_ :|
  PREC_ASSIGNMENT,  // =
  PREC_TERNARY,     // ?:
  PREC_LOGICAL_OR,  // ||
  PREC_LOGICAL_AND, // &&
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_BITWISE_XOR, // ^             akin to !=, A^~(B) akin to ==
  PREC_BITWISE_OR,  // |             akin to +
  PREC_BITWISE_AND, // &             akin to *
  PREC_SHIFT,       // >> << >>>     akin to powered base
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
extern Type anyType;

ParseExpRule *getExpRule(TokenType type);

class Parser {
  Scanner &scanner;
  Token next;
  Token current;
  Token previous;
  bool panicMode;
  bool newFlag;
public:
  bool hadError;
public:
  Parser(Scanner &scanner);
private:
  void advance();
public:
  void consume(TokenType type, const char *fmt, ...);
  bool check(TokenType type);
  bool check(TokenType *endGroupTypes);
  bool checkNext(TokenType type);
  bool match(TokenType type);
  void errorAtCurrent(const char *fmt, ...);
  void error(const char *fmt, ...);
  void compilerError(const char *fmt, ...);
  virtual void errorAt(Token *token, const char *fmt, ...);
////
  ObjFunction *compile();
  FunctionExpr *parse();
  void passSeparator();

  Expr *anonymousIterator();
  Expr *iterator(Expr *left);
  Expr *suffix(Expr *left);
  Expr *assignment(Expr *left);
  Expr *as(Expr *left);
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
  void expList(GroupingExpr *groupingExpr, TokenType endGroupType);
  Expr *array();
  Expr *floatNumber();
  Expr *intNumber();
  Expr *string();
  Expr *variable();
  Expr *unary();
  Expr *parsePrecedence(Precedence precedence);
  DeclarationExpr *declareVariable(Expr *typeExpr, TokenType *endGroupTypes);
  DeclarationExpr *parseVariable(TokenType *endGroupTypes, const char *errorMessage);
  Expr *expression(TokenType *endGroupType);
  Expr *varDeclaration(TokenType endGroupType);
  Expr *expressionStatement(TokenType endGroupType);
  Expr *forStatement(TokenType endGroupType);
  Expr *ifStatement(TokenType endGroupType);
  Expr *whileStatement(TokenType endGroupType);
  Expr *declaration(TokenType endGroupType);
  Expr *returnStatement(TokenType endGroupType);
  Expr *statement(TokenType endGroupType);
  void synchronize();
};

#endif


