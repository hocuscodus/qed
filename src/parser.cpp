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
#include "parser.hpp"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define KEY_DEF( identifier, unary, binary, prec )  { unary, binary, prec }
ParseExpRule expRules[] = { KEYS_DEF };
#undef KEY_DEF
/*
ParseExpRule expRules[] = {
    [TOKEN_LEFT_PAREN] = {&Parser::grouping, &Parser::call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {&Parser::grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACKET] = {&Parser::grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACKET] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, &Parser::dot, PREC_MEMBER},
    [TOKEN_MINUS] = {&Parser::unary, &Parser::binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, &Parser::binary, PREC_TERM},
    [TOKEN_SEPARATOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, &Parser::binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, &Parser::binary, PREC_FACTOR},
    [TOKEN_BANG] = {&Parser::unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, &Parser::binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, &Parser::assignment, PREC_ASSIGNMENT},
    [TOKEN_EQUAL_EQUAL] = {NULL, &Parser::binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, &Parser::binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, &Parser::binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, &Parser::binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, &Parser::binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {&Parser::variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {&Parser::string, NULL, PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FLOAT] = {&Parser::floatNumber, NULL, PREC_NONE},
    [TOKEN_INT] = {&Parser::intNumber, NULL, PREC_NONE},
    [TOKEN_TYPE_LITERAL] = {&Parser::primitiveType, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_AND_AND] = {NULL, &Parser::logical, PREC_AND},
    //  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {&Parser::literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_DEF] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NEW] = {&Parser::unary, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR_OR] = {NULL, &Parser::logical, PREC_EQUALITY},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {&Parser::literal, NULL, PREC_NONE},
  //  [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};
*/
Obj objString = {OBJ_STRING, NULL};
Type stringType = {VAL_OBJ, &objString};

ParseExpRule *getExpRule(TokenType type) {
  return &expRules[type];
}

static int scopeDepth = -1;

Parser::Parser(Scanner &scanner) : scanner(scanner) {
  hadError = false;
  panicMode = false;

  advance();
}

void Parser::advance() {
  previous = current;

  for (;;) {
    current = scanner.scanToken();

    if (current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(current.start);
  }
}

void Parser::consume(TokenType type, const char *message) {
  if (current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

bool Parser::check(TokenType type) {
  return current.type == type;
}

bool Parser::check(TokenType *endGroupTypes) {
  bool checked = false;

  for (int index = 0; !checked && endGroupTypes[index] != TOKEN_EOF; index++)
    checked = check(endGroupTypes[index]);

  return checked;
}

bool Parser::match(TokenType type) {
  if (!check(type))
    return false;
  advance();
  return true;
}

#define FORMAT_MESSAGE(fmt) \
  char message[256]; \
  va_list args; \
 \
  va_start (args, fmt); \
  vsnprintf(message, 256, fmt, args); \
  va_end(args);

void Parser::errorAtCurrent(const char *fmt, ...) {
  FORMAT_MESSAGE(fmt);
  errorAt(&current, message);
}

void Parser::error(const char *fmt, ...) {
  FORMAT_MESSAGE(fmt);
  errorAt(&previous, message);
}

void Parser::compilerError(const char *fmt, ...) {
  FORMAT_MESSAGE(fmt);
  previous.declareError(message);
}

void Parser::errorAt(Token *token, const char *fmt, ...) {
  if (panicMode)
    return;
  panicMode = true;
  FORMAT_MESSAGE(fmt);
  token->declareError(message);
  hadError = true;
}

#undef FORMAT_MESSAGE

bool Parser::parse() {
  /*
    int line = -1;

    for (;;) {
      Token token = scanner.scanToken();
      if (token.line != line) {
        printf("%4d ", token.line);
        line = token.line;
      } else {
        printf("   | ");
      }
      printf("%2d '%.*s'\n", token.type, token.length, token.start);

      if (token.type == TOKEN_EOF) break;
    }
  */
  expr = grouping(TOKEN_EOF, "Expect end of file.");

  return !hadError;
}

void Parser::passSeparator() {
  match(TOKEN_SEPARATOR);
}

Expr *Parser::assignment(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 0));

  if (right == NULL)
    error("Expect expression.");

  switch (left->type) {
  case EXPR_VARIABLE:
    return new AssignExpr((VariableExpr *) left, op, right);

  case EXPR_GET: {
    GetExpr *getExpr = (GetExpr *) left;

    return new SetExpr(getExpr->object, getExpr->name, op, right, -1);
  }
  default:
    errorAt(&op, "Invalid assignment target."); // [no-throw]
    return left;
  }
}

Expr *Parser::binary(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  if (right == NULL)
    error("Expect expression.");

  return new BinaryExpr(left, op, right, OP_FALSE, false); /*
   switch (op.type) {
     case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
     case TOKEN_EQUAL_EQUAL:   return new BinaryExpr(left, op, right);
     case TOKEN_GREATER:       emitByte(OP_GREATER); break;
     case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
     case TOKEN_LESS:          emitByte(OP_LESS); break;
     case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
     case TOKEN_PLUS:          emitByte(OP_ADD); break;
     case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
     case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
     case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
     default: return NULL; // Unreachable.
   }*/
}

Expr *Parser::binaryOrPostfix(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  return new BinaryExpr(left, op, right, OP_FALSE, false); /*
   switch (op.type) {
     case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
     case TOKEN_EQUAL_EQUAL:   return new BinaryExpr(left, op, right);
     case TOKEN_GREATER:       emitByte(OP_GREATER); break;
     case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
     case TOKEN_LESS:          emitByte(OP_LESS); break;
     case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
     case TOKEN_PLUS:          emitByte(OP_ADD); break;
     case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
     case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
     case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
     default: return NULL; // Unreachable.
   }*/
}

Expr *Parser::suffix(Expr *left) {
  return new UnaryExpr(previous, left);
}

Expr *Parser::ternary(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *expIfTrue = parsePrecedence((Precedence)(rule->precedence + 1));

  if (expIfTrue == NULL)
    error("Expect expression after '?'");

  passSeparator();
  consume(TOKEN_COLON, "Expect ':' after expression.");
  passSeparator();

  Expr *expIfFalse = parsePrecedence((Precedence)(rule->precedence));

  return new TernaryExpr(op, left, expIfFalse, expIfTrue);
}

Expr *Parser::dot(Expr *left) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");

  return new GetExpr(left, previous, -1);
}

Expr *Parser::call(Expr *left) {
  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_COMMA, TOKEN_ELSE, TOKEN_EOF};
  uint8_t argCount = 0;
  Expr **expList = NULL;
  Expr *handler = NULL;

  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      Expr *expr = expression(tokens);

      if (argCount == 255)
        error("Can't have more than 255 arguments.");

      expList = RESIZE_ARRAY(Expr *, expList, argCount, argCount + 1);
      expList[argCount++] = expr;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

  if (match(TOKEN_ARROW)) {
    passSeparator();
    handler = statement(TOKEN_SEPARATOR);
  }

  return new CallExpr(left, previous, argCount, expList, false, handler);
}

Expr *Parser::logical(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  return new LogicalExpr(left, op, right);
}

static Expr *createBooleanExpr(bool value) {
  return new LiteralExpr(VAL_BOOL, {.boolean = value});
}

Expr *Parser::literal() {
  Expr *exp = NULL;

  switch (previous.type) {
  case TOKEN_FALSE:
    exp = createBooleanExpr(false);
    break;
  case TOKEN_TRUE:
    exp = createBooleanExpr(true);
    break;
  default:
    return NULL; // Unreachable.
  }
  return exp;
}

Expr *Parser::swap() {
  return new SwapExpr();
}

Expr *Parser::primitiveType() {
  switch (previous.start[0]) {
  case 'v':
    switch (previous.start[1]) {
    case 'a':
      return new VariableExpr(previous, 5, false);
    case 'o':
      return new VariableExpr(previous, VAL_VOID, false);
    }
  case 'b': return new VariableExpr(previous, VAL_BOOL, false);
  case 'i': return new VariableExpr(previous, VAL_INT, false);
  case 'f': return new VariableExpr(previous, VAL_FLOAT, false);
  case 'S': return new VariableExpr(previous, VAL_OBJ, false);
  default: return NULL; // Unreachable.
  }
}

Expr *Parser::grouping() {
  char closingChar;
  TokenType endGroupType;
  char errorMessage[64];

  switch (previous.type) {
  case TOKEN_LEFT_PAREN:
    closingChar = ')';
    endGroupType = TOKEN_RIGHT_PAREN;
    break;

  case TOKEN_LEFT_BRACKET:
    closingChar = ']';
    endGroupType = TOKEN_RIGHT_BRACKET;
    break;

  case TOKEN_LEFT_BRACE:
    closingChar = '}';
    endGroupType = TOKEN_RIGHT_BRACE;
    break;
  }

  sprintf(errorMessage, "Expect '%c' after expression.", closingChar);
  return grouping(endGroupType, errorMessage);
}

UIAttributeExpr *Parser::attribute(TokenType endGroupType) {
  consume(TOKEN_IDENTIFIER, "Expect attribute name");
  passSeparator();
  Token identifier = previous;
  consume(TOKEN_COLON, "Expect colon after attribute name");
  passSeparator();
  Expr *handler = statement(endGroupType);

  if (handler->type == EXPR_STATEMENT) {
    StatementExpr *oldHandler = (StatementExpr *) handler;

    handler = oldHandler->expr;
    oldHandler->expr = NULL;
    delete oldHandler;
  }

  return new UIAttributeExpr(identifier, handler);
}

UIDirectiveExpr *newNode(int attCount, UIAttributeExpr **attributes, UIDirectiveExpr *previous, UIDirectiveExpr *lastChild)
{
  UIDirectiveExpr *node = new UIDirectiveExpr(attCount, attributes, previous, lastChild, -1);

  for (int dir = 0; dir < NUM_DIRS; dir++)
    node->_layoutIndexes[dir] = -1;

  return (node);
}

UIDirectiveExpr *Parser::directive(TokenType endGroupType, UIDirectiveExpr *previous) {
  int attCount = 0;
  UIAttributeExpr **attributeList = NULL;
  UIDirectiveExpr *lastChild = NULL;

  while (check(TOKEN_IDENTIFIER)) {
    UIAttributeExpr *att = attribute(endGroupType);

    attributeList = RESIZE_ARRAY(UIAttributeExpr *, attributeList, attCount, attCount + 1);
    attributeList[attCount++] = att;
    passSeparator();
  }

  while (check(TOKEN_LESS) && !check(TOKEN_EOF)) {
    consume(TOKEN_LESS, "Expect '<' to start a child directive");
    passSeparator();
    lastChild = directive(endGroupType, lastChild);
    passSeparator();
    consume(TOKEN_GREATER, "Expect '>' to end a directive");
    passSeparator();
  }

  return newNode(attCount, attributeList, previous, lastChild);
}

Expr *Parser::grouping(TokenType endGroupType, const char *errorMessage) {
  int count = 0;
  Expr **expList = NULL;
  bool uiFlag = false;

  passSeparator();
  statementExprs &= ~(1 << ++scopeDepth);

  while (!check(endGroupType) && !check(TOKEN_EOF)) {
    uiFlag |= endGroupType != TOKEN_RIGHT_PAREN && check(TOKEN_LESS);

    if (uiFlag)
      break;
    else {
      statementExprs &= ~(1 << scopeDepth);
      Expr *exp = declaration(endGroupType);

      expList = RESIZE_ARRAY(Expr *, expList, count, count + 1);
      expList[count++] = exp;
    }
  }

  Expr *ui = uiFlag ? directive(endGroupType, NULL) : NULL;

  consume(endGroupType, errorMessage);
  bool wasStatementExpr = (statementExprs & (1 << scopeDepth)) != 0;
  Expr *exp = endGroupType == TOKEN_RIGHT_PAREN && wasStatementExpr ? ((StatementExpr *) expList[count - 1])->expr : NULL;

  if (exp != NULL) {
    ((StatementExpr *) expList[count - 1])->expr = NULL;
    delete (StatementExpr *) expList[count - 1];

    if (count == 1)
      RESIZE_ARRAY(Expr *, expList, 1, 0);
    else {
      expList[count - 1] = exp;
      exp = NULL;
    }
  }

  scopeDepth--;
  return exp != NULL ? exp : new GroupingExpr(previous, count, expList, 0, ui);
}

Expr *Parser::floatNumber() {
  double value = strtod(previous.start, NULL);

  return new LiteralExpr(VAL_FLOAT, {.floating = value});
}

Expr *Parser::intNumber() {
  long value = strtol(previous.start, NULL, previous.start[1] == 'x' ? 16 : 10);

  return new LiteralExpr(VAL_INT, {.integer = value});
}

Expr *Parser::string() {
  return new LiteralExpr(VAL_OBJ, {.obj = &copyString(previous.start + 1, previous.length - 2)->obj});
}

Expr *Parser::variable() {
  return new VariableExpr(previous, -1, false);
}

Expr *Parser::unary() {
  Token op = previous;

  // Emit the operator instruction.
  if (op.type == TOKEN_NEW) {
    Expr *right = parsePrecedence(PREC_CALL);

    if (right->type == EXPR_CALL) {
      CallExpr *call = (CallExpr *) right;

      if (call->newFlag)
        errorAt(&op, "Use 'new' once before call");
      else
        call->newFlag = true;
    }
    else
      errorAt(&op, "Cannot use 'new' before non-callable expression");

    return right;
  }
  else
    return new UnaryExpr(op, parsePrecedence(PREC_UNARY));
}

Expr *Parser::parsePrecedence(Precedence precedence) {
  advance();

  UnaryExpFn prefixRule = getExpRule(previous.type)->prefix;

  if (prefixRule == NULL)
    return NULL;

  Expr *left = (this->*prefixRule)();

  while (precedence <= getExpRule(current.type)->precedence) {
    advance();

    left = (this->*getExpRule(previous.type)->infix)(left);
  }

  return left;
}

bool identifiersEqual(Token *a, Token *b) {
  return a->length != 0 && a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

Expr *Parser::declareVariable(Type &type, TokenType endGroupType) {
  Token name = previous;
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *expr = match(TOKEN_EQUAL) ? expression(tokens) : NULL;

  return new DeclarationExpr(type, name, expr);
}

Expr *Parser::parseVariable(TokenType endGroupType, const char *errorMessage) {
  Type type = readType();

  consume(TOKEN_IDENTIFIER, errorMessage);
  return declareVariable(type, endGroupType);
}

Expr *Parser::expression(TokenType *endGroupTypes) {
  int count = 0;
  Expr **expList = NULL;
  Expr *exp = parsePrecedence((Precedence)(PREC_NONE + 1));

  if (exp == NULL)
    error("Expect expression.");

  while (!check(endGroupTypes) && !check(TOKEN_EOF)) {
    Expr *exp2 = parsePrecedence((Precedence)(PREC_NONE + 1));

    if (exp2 == NULL)
      error("Expect expression.");

    if (expList == NULL) {
      expList = RESIZE_ARRAY(Expr *, expList, count, count + 1);
      expList[count++] = exp;
    }

    expList = RESIZE_ARRAY(Expr *, expList, count, count + 1);
    expList[count++] = exp2;
  }

  return expList != NULL ? new ListExpr(count, expList, EXPR_LIST, NULL) : exp;
}

Expr *Parser::function(Type *type, TokenType endGroupType) {
  Token name = previous;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  int arity = 0;
  Expr **parameters = NULL;

  passSeparator();

  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      if (arity >= 255)
        errorAtCurrent("Can't have more than 255 parameters.");

      Expr *param = parseVariable(endGroupType, "Expect parameter name.");

      parameters = RESIZE_ARRAY(Expr *, parameters, arity, arity + 1);
      parameters[arity++] = param;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

  Expr *body = grouping(TOKEN_RIGHT_BRACE, "Expect '}' after expression.");

  return new FunctionExpr(*type, name, arity, parameters, body, NULL);
}

Type Parser::readType() {
  if (match(TOKEN_TYPE_LITERAL))
    switch (previous.start[0]) {
    case 'v': return {VAL_VOID, NULL};
    case 'b': return {VAL_BOOL};
    case 'i': return {VAL_INT};
    case 'f': return {VAL_FLOAT};
    case 'S': return stringType;
//    default: return {VAL_VOID, &objString}; // Unreachable.
    }
  else {
    consume(TOKEN_IDENTIFIER, "Expect type.");
    return {VAL_VOID, (Obj *) new VariableExpr(previous, -1, false)};
  }
}

Expr *Parser::funDeclaration(TokenType endGroupType) {
  Type type = readType();
  consume(TOKEN_IDENTIFIER, "Expect function name.");

  Expr *expr = function(&type, endGroupType);

  passSeparator();
  return expr;
}

Expr *Parser::varDeclaration(TokenType endGroupType) {
  Expr *exp = parseVariable(endGroupType, "Expect variable name.");

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' or newline after variable declaration.");

  return exp;
}

Expr *Parser::expressionStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = expression(tokens);

  if (!check(endGroupType) && !check(TOKEN_ELSE))
    consume(TOKEN_SEPARATOR, "Expect ';' or newline after expression.");

  statementExprs |= 1 << scopeDepth;
  return new StatementExpr(exp);
}

Expr *Parser::forStatement(TokenType endGroupType) {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

  Expr *initializer = NULL;

  if (!match(TOKEN_SEPARATOR))
    initializer = match(TOKEN_VAR) ? varDeclaration(endGroupType) : expressionStatement(endGroupType);

  TokenType tokens[] = {TOKEN_SEPARATOR, TOKEN_ELSE, TOKEN_EOF};
  Expr *condition = check(TOKEN_SEPARATOR) ? createBooleanExpr(true) : expression(tokens);

  consume(TOKEN_SEPARATOR, "Expect ';' or newline after loop condition.");

  TokenType tokens2[] = {TOKEN_RIGHT_PAREN, TOKEN_SEPARATOR, TOKEN_ELSE, TOKEN_EOF};
  Expr *increment = check(TOKEN_RIGHT_PAREN) ? NULL : expression(tokens2);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

  Expr *body = statement(endGroupType);

  if (increment != NULL) {
    Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 2);

    expList[0] = new UnaryExpr(buildToken(TOKEN_PRINT, "print", 5, -1), body);
    expList[1] = new UnaryExpr(buildToken(TOKEN_PRINT, "print", 5, -1), increment);
    body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 2, expList, 0, NULL);
  }

  body = new BinaryExpr(condition, buildToken(TOKEN_WHILE, "while", 5, -1), body, OP_FALSE, false);

  if (initializer != NULL) {
    Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 2);

    expList[0] = initializer;
    expList[1] = body;
    body = new GroupingExpr(buildToken(TOKEN_RIGHT_BRACE, "}", 1, -1), 2, expList, 0, NULL);
  }

  return body;
}

Expr *Parser::ifStatement(TokenType endGroupType) {
  Token name = previous;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");

  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_ELSE, TOKEN_EOF};
  Expr *condExp = expression(tokens);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

  Expr *thenExp = statement(endGroupType);
  Expr *elseExp = match(TOKEN_ELSE) ? statement(endGroupType) : NULL;

  return new TernaryExpr(name, condExp, thenExp, elseExp);
}

Expr *Parser::whileStatement(TokenType endGroupType) {
  Token name = previous;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");

  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = expression(tokens);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  return new BinaryExpr(exp, name, statement(endGroupType), OP_FALSE, false);
}

Expr *Parser::declaration(TokenType endGroupType) {
  Expr *exp;
/*
  if (match(TOKEN_FUN))
    exp = funDeclaration(endGroupType);
  else if (match(TOKEN_VAR))
    exp = varDeclaration(endGroupType);
  else*/
    exp = statement(endGroupType);

  if (panicMode)
    synchronize();

  return exp;
}

Expr *Parser::printStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *value = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' after value.");

  return new UnaryExpr(buildToken(TOKEN_PRINT, "print", 5, -1), value);
}

Expr *Parser::returnStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Token keyword = previous;
  Expr *value = NULL;

  if (!check(tokens))
    value = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' after return value.");

  return new ReturnExpr(keyword, value);
}

Expr *Parser::statement(TokenType endGroupType) {
  // ignore optional separator before second operand
  passSeparator();

  if (match(TOKEN_PRINT))
    return printStatement(endGroupType);
//  else if (match(TOKEN_RETURN))
//    return returnStatement(endGroupType);
  else if (match(TOKEN_IF))
    return ifStatement(endGroupType);
  else if (match(TOKEN_WHILE))
    return whileStatement(endGroupType);
  else if (match(TOKEN_FOR))
    return forStatement(endGroupType);
  else
    return expressionStatement(endGroupType);
}

void Parser::synchronize() {
  panicMode = false;

  while (current.type != TOKEN_EOF) {
    if (previous.type == TOKEN_SEPARATOR)
      return;
    switch (current.type) {
      //      case TOKEN_CLASS:
      //      case TOKEN_FUN:
    case TOKEN_TYPE_LITERAL:
      //      case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
//    case TOKEN_RETURN:
      return;

    default:; // Do nothing.
    }

    advance();
  }
}