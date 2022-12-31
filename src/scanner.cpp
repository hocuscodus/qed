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
#include <string.h>
#include <stdio.h>
#include "scanner.hpp"

static bool isAlpha(char c) {
  char c1 = c & ~0x20;

  return (c1 >= 'A' && c1 <= 'Z') || c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isHexLetter(char c) {
  c &= ~0x20;
  return c >= 'A' && c <= 'F';
}

Token buildToken(TokenType type, const char *start, int length, int line = -1) {
  Token token;

  token.type = type;
  token.start = start;
  token.length = length;
  token.line = line;
  return token;
}

std::string Token::getString() {
  std::string str(start, start + length);

  return str;
}

bool Token::equal(const char *string) {
  return !memcmp(start, string, length) && string[length] == '\0';
}

void Token::declareError(const char *message) {
  fprintf(stderr, "[line %d] Error", line);

  if (type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", length, start);
  }

  fprintf(stderr, ": %s\n", message);
}

Scanner::Scanner(const char *source) {
  start = source;
  current = source;
  line = 1;
}

void Scanner::reset(const char *source) {
  start = source;
  current = source;
  line--;
}

Token Scanner::scanToken() {
  if (!skipWhitespace()) return errorToken("Unclosed comment");

  start = current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();

  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  switch (c) {
    case '(':
      return makeToken(TOKEN_LEFT_PAREN);

    case ')':
      return makeToken(TOKEN_RIGHT_PAREN);

    case '{':
      return makeToken(TOKEN_LEFT_BRACE);

    case '}':
      return makeToken(TOKEN_RIGHT_BRACE);

    case '[':
      return makeToken(TOKEN_LEFT_BRACKET);

    case ']':
      return makeToken(TOKEN_RIGHT_BRACKET);

    case ',':
      return makeToken(TOKEN_COMMA);

    case '.':
      return makeToken(TOKEN_DOT);

    case '~':
      return makeToken(TOKEN_COMP);

    case '%':
      return makeToken(TOKEN_PERCENT);

    case '?':
      return makeToken(TOKEN_QUESTION);

    case '-':
      return makeToken(match('=') ? TOKEN_MINUS_EQUAL : match('-') ? TOKEN_MINUS_MINUS : match('>') ? TOKEN_ARROW : TOKEN_MINUS);

    case '+':
      return makeToken(match('=') ? TOKEN_PLUS_EQUAL : match('+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS);

    case '/':
      return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);

    case '*':
      return makeToken(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);

    case '!':
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);

    case '=':
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);

    case '<':
      return makeToken(match('=')
        ? TOKEN_LESS_EQUAL
        : match('<')
          ? match('=')
            ? TOKEN_LESS_LESS_EQUAL
            : TOKEN_LESS_LESS
          : TOKEN_LESS);

    case '>':
      return makeToken(match('=')
        ? TOKEN_GREATER_EQUAL
        : match('>')
          ? match('>')
            ? TOKEN_GREATER_GREATER_GREATER
            : match('=')
              ? TOKEN_GREATER_GREATER_EQUAL
              : TOKEN_GREATER_GREATER
          : TOKEN_GREATER);

    case ':':
      return makeToken(match(':') ? TOKEN_COLON_COLON : TOKEN_COLON);

    case '&':
      return makeToken(match('=') ? TOKEN_AND_EQUAL : match('&') ? TOKEN_AND_AND : TOKEN_AND);

    case '|':
      return makeToken(match('=') ? TOKEN_OR_EQUAL : match('|') ? TOKEN_OR_OR : TOKEN_OR);

    case '^':
      return makeToken(match('=') ? TOKEN_XOR_EQUAL : TOKEN_XOR);

    case '"':
      return string();

    case '$': {
      while (isAlpha(peek())) advance();

      TokenType tokenType = checkKeyword(0, 5, "$EXPR", TOKEN_INSERT);

      if (tokenType != TOKEN_IDENTIFIER)
        return makeToken(tokenType);
      else
        break;
    }

    case '\n':
      line++;
      // no break; or return(...); here, fully intended...

    case ';':
      do
        if (!skipWhitespace()) return errorToken("Unclosed comment");
      while (match('\n') || match(';'));
      return makeToken(TOKEN_SEPARATOR);
  }

  return errorToken("Unexpected character.");
}

bool Scanner::isAtEnd() {
  return *current == '\0';
}

char Scanner::advance() {
  return current++[0];
}

char Scanner::peek() {
  return *current;
}

char Scanner::peekNext() {
  if (isAtEnd()) return '\0';
  return current[1];
}

bool Scanner::match(char expected) {
  if (isAtEnd() || *current != expected) return false;
  current++;
  return true;
}

Token Scanner::makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = start;
  token.length = (int)(current - start);
  token.line = line;
  return token;
}

Token Scanner::errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = line;
  return token;
}

bool Scanner::skipWhitespace() {
  for (;;) {
    switch (peek()) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '/':
        switch (peekNext()) {
          case '/':
            // A comment goes until the end of the line.
            while (peek() != '\n' && !isAtEnd()) advance();
            break;

          case '*':
            current += 2; // Skip header '/*'
            // A recursive comment goes until same level '*/'
            if (!skipRecursiveComment())
              return false;
            break;

          default:
            return true;
        }
        break;

      default:
        return true;
    }
  }
}

bool Scanner::skipRecursiveComment() {
  for (;;) {
    switch (peek()) {
      case 0: // end of file, unclosed comment error
        return false;

      case '/':
        advance();

        if (peek() == '*') {
          advance();

          if (!skipRecursiveComment())
            return false;
        }
        break;

      case '*':
        advance();

        if (peek() == '/') {
          advance();
          return true;
        }
        break;

      case '\n':
        line++;
        // no break; or return; here, fully intended...

      default:
        advance();
    }
  }
}

TokenType Scanner::checkKeyword(int start, int length, const char *rest, TokenType type) {
  return current - this->start == start + length && memcmp(this->start + start, rest, length) == 0 ? type : TOKEN_IDENTIFIER;
}

TokenType Scanner::identifierType() {
  switch (start[0]) {
    case 'a': return checkKeyword(1, 1, "s", TOKEN_AS);
    case 'b': return checkKeyword(1, 3, "ool", TOKEN_TYPE_LITERAL);
    case 'd': return checkKeyword(1, 2, "ef", TOKEN_DEF);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':// return checkKeyword(1, 4, "alse", TOKEN_FALSE);
      if (current - start > 1)
        switch (start[1]) {
          case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
          case 'l': return checkKeyword(2, 3, "oat", TOKEN_TYPE_LITERAL);
          case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
          case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
        }
      break;
    case 'i':
      if (current - start > 1)
        switch (start[1]) {
          case 'f': return checkKeyword(2, 0, "", TOKEN_IF);
          case 'm': return checkKeyword(2, 4, "port", TOKEN_IMPORT);
          case 'n': return checkKeyword(2, 1, "t", TOKEN_TYPE_LITERAL);
        }
      break;
    case 'm': return checkKeyword(1, 2, "od", TOKEN_MOD);
    case 'n': return checkKeyword(1, 2, "ew", TOKEN_NEW);
    case 'p':
      if (current - start > 1)
        switch (start[1]) {
          case 'a': return checkKeyword(2, 5, "ckage", TOKEN_PACKAGE);
          case 'r': return checkKeyword(2, 3, "int", TOKEN_PRINT);
        }
      break;
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 'S': return checkKeyword(1, 5, "tring", TOKEN_TYPE_LITERAL);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (current - start > 1)
        switch (start[1]) {
          case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
          case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
        }
      break;
    case 'v':
      if (current - start > 1)
        switch (start[1]) {
          case 'a':
            if (current - start > 2) {
              switch (start[2]) {
                case 'l': return checkKeyword(3, 0, "", TOKEN_VAL);
                case 'r': return checkKeyword(3, 0, "", TOKEN_TYPE_LITERAL);
              }
            }
            break;
          case 'o': return checkKeyword(2, 2, "id", TOKEN_TYPE_LITERAL);
        }
      break;
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

Token Scanner::identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();

  return makeToken(identifierType());
}

Token Scanner::number() {
  bool hex = *start == '0' && peek() == 'x';

  if (hex)
    advance();

  while (isDigit(peek()) || (hex && isHexLetter(peek()))) advance();

  // Look for a fractional part.
  if (!hex && peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();

    return makeToken(TOKEN_FLOAT);
  }
  else
    return makeToken(TOKEN_INT);
}

Token Scanner::string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}