/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_scanner_h
#define qed_scanner_h

#include <string>

#define KEYS_DEF \
    /* Single-character tokens. */ \
    KEY_DEF( TOKEN_LEFT_PAREN, &Parser::grouping, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_CALL, &Parser::grouping, &Parser::call, PREC_CALL ),  \
    KEY_DEF( TOKEN_RIGHT_PAREN, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_LEFT_BRACE, &Parser::grouping, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_RIGHT_BRACE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_LEFT_BRACKET, &Parser::array, &Parser::arrayElement, PREC_CALL ),  \
    KEY_DEF( TOKEN_RIGHT_BRACKET, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_COMMA, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_DOT, NULL, &Parser::dot, PREC_MEMBER ),  \
    KEY_DEF( TOKEN_COMP, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PERCENT, NULL, &Parser::suffix, PREC_FACTOR ),  \
    KEY_DEF( TOKEN_QUESTION, NULL, &Parser::ternary, PREC_TERNARY ),  \
    /* One, two or three character tokens. */ \
    KEY_DEF( TOKEN_BANG, &Parser::unary, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_BANG_EQUAL, NULL, &Parser::binary, PREC_EQUALITY ),  \
    KEY_DEF( TOKEN_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_EQUAL_EQUAL, NULL, &Parser::binary, PREC_EQUALITY ),  \
    KEY_DEF( TOKEN_GREATER, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_GREATER_EQUAL, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_GREATER_GREATER, NULL, &Parser::binary, PREC_SHIFT ),  \
    KEY_DEF( TOKEN_GREATER_GREATER_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_GREATER_GREATER_GREATER, NULL, &Parser::binary, PREC_SHIFT ),  \
    KEY_DEF( TOKEN_LESS, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_LESS_EQUAL, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_LESS_LESS, NULL, &Parser::binary, PREC_SHIFT ),  \
    KEY_DEF( TOKEN_LESS_LESS_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_MINUS, &Parser::unary, &Parser::binary, PREC_TERM ),  \
    KEY_DEF( TOKEN_MINUS_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_MINUS_MINUS, &Parser::unary, &Parser::suffix, PREC_CALL ),  \
    KEY_DEF( TOKEN_ARROW, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PLUS, NULL, &Parser::binary, PREC_TERM ),  \
    KEY_DEF( TOKEN_PLUS_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_PLUS_PLUS, &Parser::unary, &Parser::suffix, PREC_CALL),  \
    KEY_DEF( TOKEN_SLASH, NULL, &Parser::binary, PREC_FACTOR ),  \
    KEY_DEF( TOKEN_SLASH_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_STAR, NULL, &Parser::binary, PREC_FACTOR ),  \
    KEY_DEF( TOKEN_STAR_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_AND, NULL, &Parser::binary, PREC_BITWISE_AND ),  \
    KEY_DEF( TOKEN_AND_AND, NULL, &Parser::logical, PREC_LOGICAL_AND ),  \
    KEY_DEF( TOKEN_AND_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_OR, NULL, &Parser::binary, PREC_BITWISE_OR ),  \
    KEY_DEF( TOKEN_OR_OR, NULL, &Parser::logical, PREC_LOGICAL_OR ),  \
    KEY_DEF( TOKEN_OR_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_XOR, NULL, NULL, PREC_BITWISE_XOR ),  \
    KEY_DEF( TOKEN_XOR_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_COLON, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_BACKSLASH, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_UNDERLINE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_ARRAY, NULL, &Parser::rightBinary, PREC_ARRAY ),  \
    KEY_DEF( TOKEN_ITERATOR, &Parser::anonymousIterator, &Parser::iterator, PREC_ITERATOR ),  \
    /* Literals. */ \
    KEY_DEF( TOKEN_IDENTIFIER, &Parser::variable, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_STRING, &Parser::string, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_INT, &Parser::intNumber, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FLOAT, &Parser::floatNumber, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_VAL, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_VAR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_TYPE_LITERAL, &Parser::primitiveType, NULL, PREC_NONE ),  \
    /* Keywords. */ \
    KEY_DEF( TOKEN_AS, NULL, &Parser::as, PREC_UNARY ),  \
    KEY_DEF( TOKEN_DEF, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_ELSE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FALSE, &Parser::literal, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FOR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FUN, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_IF, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_IMPORT, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_NEW, &Parser::unary, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PACKAGE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_RETURN, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_SUPER, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_THIS, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_TRUE, &Parser::literal, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_WHILE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_SEPARATOR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_NATIVE_CODE, &Parser::literal, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_ERROR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_EOF, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_INSERT, &Parser::swap, NULL, PREC_NONE ),

#define KEY_DEF( identifier, unary, binary, prec )  identifier
typedef enum { KEYS_DEF } TokenType;
#undef KEY_DEF

struct Token {
  TokenType type;
  const char *start;
  int length;
  int line;

  std::string getString();
  bool equal(const char *string);
  int getStartIndex();
  bool endsWith(const char *suffix);
  bool isClass();
  inline bool isInternal() {return start[0] == '_';}
  inline bool isHandler() {return endsWith("_");}
  inline bool isUserClass() {return isClass() && !isHandler();}

  virtual void declareError(const char *message);
};

Token buildToken(TokenType type, const char *start, int length, int line = -1);
Token buildToken(TokenType type, const char *start);

class Scanner {
  const char *start;
  const char *current;
  int line;
public:
  Scanner(const char *source);

  void reset(const char *source);
  Token scanToken();
private:
  bool isAtEnd();
  char advance();
  char peek();
  char peekNext();
  bool match(char expected);
  Token makeToken(TokenType type);
  Token errorToken(const char *message);
  Token skipWhitespace();
  bool skipRecursiveComment(char chr);
  TokenType checkKeyword(int start, int length, const char *rest, TokenType type);
  TokenType identifierType();
  Token identifier();
  Token number();
  Token string();
};

#endif


