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
#ifndef qed_scanner_h
#define qed_scanner_h

#include <string>

#define KEYS_DEF \
    /* Single-character tokens. */ \
    KEY_DEF( TOKEN_LEFT_PAREN, &Parser::grouping, &Parser::call, PREC_CALL ),  \
    KEY_DEF( TOKEN_RIGHT_PAREN, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_LEFT_BRACE, &Parser::grouping, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_RIGHT_BRACE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_LEFT_BRACKET, &Parser::grouping, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_RIGHT_BRACKET, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_COMMA, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_DOT, NULL, &Parser::dot, PREC_MEMBER ),  \
    KEY_DEF( TOKEN_COMP, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PERCENT, NULL, NULL, PREC_FACTOR ),  \
    KEY_DEF( TOKEN_QUESTION, NULL, &Parser::ternary, PREC_TERNARY ),  \
    /* One, two or three character tokens. */ \
    KEY_DEF( TOKEN_BANG, &Parser::unary, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_BANG_EQUAL, NULL, &Parser::binary, PREC_EQUALITY ),  \
    KEY_DEF( TOKEN_EQUAL, NULL, &Parser::assignment, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_EQUAL_EQUAL, NULL, &Parser::binary, PREC_EQUALITY ),  \
    KEY_DEF( TOKEN_GREATER, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_GREATER_EQUAL, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_GREATER_GREATER, NULL, &Parser::binary, PREC_SHIFT ),  \
    KEY_DEF( TOKEN_GREATER_GREATER_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_GREATER_GREATER_GREATER, NULL, &Parser::binary, PREC_SHIFT ),  \
    KEY_DEF( TOKEN_LESS, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_LESS_EQUAL, NULL, &Parser::binary, PREC_COMPARISON ),  \
    KEY_DEF( TOKEN_LESS_LESS, NULL, &Parser::binary, PREC_SHIFT ),  \
    KEY_DEF( TOKEN_LESS_LESS_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_MINUS, &Parser::unary, &Parser::binary, PREC_TERM ),  \
    KEY_DEF( TOKEN_MINUS_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_MINUS_MINUS, &Parser::unary, &Parser::suffix, PREC_CALL ),  \
    KEY_DEF( TOKEN_ARROW, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PLUS, NULL, &Parser::binary, PREC_TERM ),  \
    KEY_DEF( TOKEN_PLUS_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_PLUS_PLUS, &Parser::unary, &Parser::suffix, PREC_CALL),  \
    KEY_DEF( TOKEN_SLASH, NULL, &Parser::binary, PREC_FACTOR ),  \
    KEY_DEF( TOKEN_SLASH_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_STAR, NULL, &Parser::binary, PREC_FACTOR ),  \
    KEY_DEF( TOKEN_STAR_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_AND, NULL, &Parser::binary, PREC_BITWISE_AND ),  \
    KEY_DEF( TOKEN_AND_AND, NULL, &Parser::binary, PREC_LOGICAL_AND ),  \
    KEY_DEF( TOKEN_AND_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_OR, NULL, &Parser::binary, PREC_BITWISE_OR ),  \
    KEY_DEF( TOKEN_OR_OR, NULL, &Parser::binary, PREC_LOGICAL_OR ),  \
    KEY_DEF( TOKEN_OR_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_XOR, NULL, NULL, PREC_BITWISE_XOR ),  \
    KEY_DEF( TOKEN_XOR_EQUAL, NULL, NULL, PREC_ASSIGNMENT ),  \
    KEY_DEF( TOKEN_COLON, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_COLON_COLON, NULL, NULL, PREC_NONE ),  \
    /* Literals. */ \
    KEY_DEF( TOKEN_IDENTIFIER, &Parser::variable, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_STRING, &Parser::string, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_INT, &Parser::intNumber, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FLOAT, &Parser::floatNumber, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_VAL, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_VAR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_TYPE_LITERAL, &Parser::primitiveType, NULL, PREC_NONE ),  \
    /* Keywords. */ \
    KEY_DEF( TOKEN_AS, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_DEF, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_ELSE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FALSE, &Parser::literal, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FOR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_FUN, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_IF, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_IMPORT, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_MOD, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_NEW, &Parser::unary, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PACKAGE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_PRINT, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_RETURN, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_SUPER, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_THIS, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_TRUE, &Parser::literal, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_WHILE, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_SEPARATOR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_ERROR, NULL, NULL, PREC_NONE ),  \
    KEY_DEF( TOKEN_EOF, NULL, NULL, PREC_NONE ),

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

  virtual void declareError(const char *message);
};

Token buildToken(TokenType type, const char *start, int length, int line);

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
  bool skipWhitespace();
  bool skipRecursiveComment();
  TokenType checkKeyword(int start, int length, const char *rest, TokenType type);
  TokenType identifierType();
  Token identifier();
  Token number();
  Token string();
};

#endif


