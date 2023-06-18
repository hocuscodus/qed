/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include "parser.hpp"
#include "memory.h"
#include "object.hpp"
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
  advance();
}

void Parser::advance() {
  previous = current;
  current = next;

  for (;;) {
    next = scanner.scanToken();

    if (next.type != TOKEN_ERROR)
      break;

    errorAtCurrent(next.start);
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

bool Parser::checkNext(TokenType type) {
  return next.type == type;
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

ObjFunction *Parser::compile() {
  GroupingExpr *expr = (GroupingExpr *) parse();

  return expr ? expr->_compiler.compile(expr, this) : NULL;
}

Expr *Parser::parse() {
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
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_EOF, "", 0, -1), 0, NULL, NULL);
  FunctionExpr *functionExpr = new FunctionExpr(NULL, buildToken(TOKEN_IDENTIFIER, "Main", 4, -1), 0, group);

  group->_compiler.groupingExpr = group;
  functionExpr->_function.expr = functionExpr;
  functionExpr->_function.type = VOID_TYPE;
  functionExpr->_function.name = NULL;//copyString(name.start, name.length);
  group->_compiler.beginScope(&functionExpr->_function, this);
  expList(group, TOKEN_EOF, "Expect end of file.");
  group->_compiler.endScope();
////////
/*  Compiler *compiler = new Compiler;

  compiler->parser = this;
  compiler->beginScope(newFunction(VOID_TYPE, NULL, 0));

  Expr *expr = grouping(TOKEN_EOF, compiler, "Expect end of file.");

  compiler->endScope();
*/
  return hadError ? NULL : group;
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
  case EXPR_REFERENCE:
    return new AssignExpr((ReferenceExpr *) left, op, right);

  case EXPR_GET: {
    GetExpr *getExpr = (GetExpr *) left;

    return new SetExpr(getExpr->object, getExpr->name, op, right, -1);
  }
  default:
    errorAt(&op, "Invalid assignment target."); // [no-throw]
    return left;
  }
}

Scanner fakeScanner("");

struct FakeParser : Parser {

  FakeParser() : Parser(fakeScanner) {}

  void errorAt(Token *token, const char *fmt, ...) {
      hadError = true;
  }
};

Expr *Parser::binary(Expr *left) {
  Token op = previous;

  if (op.type == TOKEN_STAR && left->type == EXPR_REFERENCE && check(TOKEN_IDENTIFIER)) {
    FakeParser tempParser;
    ReferenceExpr *expr = (ReferenceExpr *) left;

    left->resolve(tempParser);

    if (!tempParser.hadError && IS_FUNCTION(expr->returnType)) {
      expr->returnType = OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(expr->returnType)));
      return left;
    }
  }

  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  if (right == NULL)
    error("Expect expression.");

  return new BinaryExpr(left, op, right); /*
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

  return new BinaryExpr(left, op, right); /*
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
  Token op = previous;

  // Emit the operator instruction.
  switch (op.type) {
    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS_MINUS:
      return new AssignExpr((ReferenceExpr *) left, op, NULL);

    default:
      return new UnaryExpr(op, left);
  }
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

  return new TernaryExpr(op, left, expIfTrue, expIfFalse);
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

    if (current.type == TOKEN_LEFT_BRACE) {
      advance();
      handler = grouping();
    }
    else
      handler = statement(TOKEN_SEPARATOR);
  }

  return new CallExpr(false, left, previous, argCount, expList, handler);
}

Expr *Parser::arrayElement(Expr *left) {
  TokenType tokens[] = {TOKEN_RIGHT_BRACKET, TOKEN_COMMA, TOKEN_ELSE, TOKEN_EOF};
  uint8_t indexCount = 0;
  Expr **expList = NULL;
  Expr *handler = NULL;

  do {
    if (check(TOKEN_RIGHT_BRACKET))
      break;

    Expr *expr = expression(tokens);

    if (indexCount == 255)
      error("Can't have more than 255 indexes.");

    expList = RESIZE_ARRAY(Expr *, expList, indexCount, indexCount + 1);
    expList[indexCount++] = expr;
  } while (match(TOKEN_COMMA));

  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after indexes.");

  return new ArrayElementExpr(left, previous, indexCount, expList);
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
  case TOKEN_NATIVE_CODE:
    exp = new NativeExpr(previous);
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
      return new ReferenceExpr(previous, UNKNOWN_TYPE);
    case 'o':
      return new ReferenceExpr(previous, VOID_TYPE);
    }
  case 'b': return new ReferenceExpr(previous, BOOL_TYPE);
  case 'i': return new ReferenceExpr(previous, INT_TYPE);
  case 'f': return new ReferenceExpr(previous, FLOAT_TYPE);
  case 'S': return new ReferenceExpr(previous, stringType);
  default: return NULL; // Unreachable.
  }
}

Expr *Parser::grouping() {
  char closingChar;
  TokenType endGroupType;
  char errorMessage[64];
  GroupingExpr *group = new GroupingExpr(previous, 0, NULL, NULL);

  switch (previous.type) {
  case TOKEN_LEFT_PAREN:
    closingChar = ')';
    endGroupType = TOKEN_RIGHT_PAREN;
    break;

  case TOKEN_LEFT_BRACE:
    closingChar = '}';
    endGroupType = TOKEN_RIGHT_BRACE;
    break;
  }

  sprintf(errorMessage, "Expect '%c' after expression.", closingChar);
  group->_compiler.groupingExpr = group;
  group->_compiler.beginScope();
  expList(group, endGroupType, errorMessage);
  group->_compiler.endScope();

  if (endGroupType == TOKEN_RIGHT_PAREN && (statementExprs & (1 << (scopeDepth + 1))) != 0)
    if (group->count == 1) {
      Expr *exp = group->expressions[group->count - 1];

      RESIZE_ARRAY(Expr *, group->expressions, 1, 0);
      return exp;
    }
    else
      return NULL;

  return group;
}

UIAttributeExpr *Parser::attribute(TokenType endGroupType) {
  consume(TOKEN_IDENTIFIER, "Expect attribute name");
  passSeparator();

  Token identifier = previous;

  consume(TOKEN_COLON, "Expect colon after attribute name");
  passSeparator();

  Expr *handler;

  if (current.type == TOKEN_LEFT_BRACE) {
    advance();
    handler = grouping();
  }
  else
    handler = statement(endGroupType);

  return new UIAttributeExpr(identifier, handler);
}

UIDirectiveExpr *newNode(int childDir, int attCount, UIAttributeExpr **attributes, UIDirectiveExpr *previous, UIDirectiveExpr *lastChild) {
  UIDirectiveExpr *node = new UIDirectiveExpr(childDir, attCount, attributes, previous, lastChild, 0, false);

  for (int dir = 0; dir < NUM_DIRS; dir++)
    node->_layoutIndexes[dir] = -1;

  return (node);
}

UIDirectiveExpr *Parser::directive(TokenType endGroupType, UIDirectiveExpr *previous) {
  int attCount = 0;
  UIAttributeExpr **attributeList = NULL;
  UIDirectiveExpr *lastChild = NULL;
  int childDir = 0;

  if (!match(TOKEN_DOT))
    if (match(TOKEN_UNDERLINE))
      childDir = 1;
    else
      if (match(TOKEN_OR))
        childDir = 2;
      else
        if (match(TOKEN_BACKSLASH))
          childDir = 3;

  passSeparator();

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

  return newNode(childDir, attCount, attributeList, previous, lastChild);
}

void Parser::expList(GroupingExpr *groupingExpr, TokenType endGroupType, const char *errorMessage) {
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

      groupingExpr->expressions = RESIZE_ARRAY(Expr *, groupingExpr->expressions, groupingExpr->count, groupingExpr->count + 1);
      groupingExpr->expressions[groupingExpr->count++] = exp;
    }
  }

  if (uiFlag)
    groupingExpr->ui = directive(endGroupType, NULL);

  consume(endGroupType, errorMessage);
  scopeDepth--;
}

Expr *Parser::array() {
  TokenType tokens[] = {TOKEN_RIGHT_BRACKET, TOKEN_COMMA, TOKEN_ELSE, TOKEN_EOF};
  int count = 0;
  Expr **expList = NULL;

  do {
    passSeparator();

    if (check(TOKEN_RIGHT_BRACKET))
      break;

    Expr *exp = expression(tokens);

    if (exp) {
      if (count >= 255)
        errorAtCurrent("Can't have more than 255 parameters.");

      expList = RESIZE_ARRAY(Expr *, expList, count, count + 1);
      expList[count++] = exp;
    }

    passSeparator();
  } while (match(TOKEN_COMMA));

  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression.");
  return new ArrayExpr(count, expList, NULL);
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
  return new ReferenceExpr(previous, UNKNOWN_TYPE);
}

Expr *Parser::unary() {
  Token op = previous;

  // Emit the operator instruction.
  switch (op.type) {
    case TOKEN_NEW: {
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
    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS_MINUS: {
      Expr *right = parsePrecedence(PREC_CALL);

      return new AssignExpr(NULL, op, right);
    }
    default:
      return new UnaryExpr(op, parsePrecedence(PREC_UNARY));
  }
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

DeclarationExpr *Parser::declareVariable(Expr *typeExpr, TokenType *endGroupTypes) {
  Token name = previous;
  Expr *expr = match(TOKEN_EQUAL) ? expression(endGroupTypes) : NULL;

  return new DeclarationExpr(typeExpr, name, expr);
}

DeclarationExpr *Parser::parseVariable(TokenType *endGroupTypes, const char *errorMessage) {
  Expr *typeExp = parsePrecedence((Precedence)(PREC_NONE + 1));

  consume(TOKEN_IDENTIFIER, errorMessage);
  return declareVariable(typeExp, endGroupTypes);
}

Expr *Parser::expression(TokenType *endGroupTypes) {
  Expr *exp = parsePrecedence((Precedence)(PREC_NONE + 1));

  if (exp == NULL)
    error("Expect expression.");

  if (exp->type == EXPR_REFERENCE) {
    if (check(TOKEN_IDENTIFIER) && (checkNext(TOKEN_EQUAL) || checkNext(TOKEN_LEFT_PAREN) || checkNext(TOKEN_SEPARATOR))) {
      consume(TOKEN_IDENTIFIER, "Expect name identifier after type.");

      Token name = previous;

      if(match(TOKEN_LEFT_PAREN)) {
        exp->resolve(*this);

        GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{", 1, -1), 0, NULL, NULL);
        FunctionExpr *functionExpr = new FunctionExpr(exp, name, 0, group);

        functionExpr->_function.expr = functionExpr;
        functionExpr->_function.type = ((ReferenceExpr *) exp)->returnType;
        functionExpr->_function.name = copyString(name.start, name.length);
        group->_compiler.groupingExpr = group;
        group->_compiler.beginScope(&functionExpr->_function, this);
  //      bindFunction(compiler.prefix, function);
        passSeparator();

        if (!check(TOKEN_RIGHT_PAREN))
          do {
            DeclarationExpr *param = parseVariable(endGroupTypes, "Expect parameter name.");

            param->typeExpr->resolve(*this);

            if (param->typeExpr->type == EXPR_REFERENCE && !IS_UNKNOWN(((ReferenceExpr *) param->typeExpr)->returnType)) {
              group->expressions = RESIZE_ARRAY(Expr *, group->expressions, group->count, group->count + 1);
              group->expressions[group->count++] = param;
              group->_compiler.addDeclaration(((ReferenceExpr *) param->typeExpr)->returnType, param->name, NULL, false, this);
            }
            else
              error("Parameter %d not typed correctly", group->count + 1);
          } while (match(TOKEN_COMMA));

        consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

        bool parenFlag = match(TOKEN_LEFT_PAREN);

        if (!parenFlag)
          consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

        group->name = previous;
        functionExpr->arity = group->count;
        functionExpr->_declaration = getCurrent()->enclosing->checkDeclaration(OBJ_TYPE(&functionExpr->_function), name, &functionExpr->_function, this);
        expList(group, parenFlag ? TOKEN_RIGHT_PAREN : TOKEN_RIGHT_BRACE, "Expect '}' after expression.");
        group->_compiler.endScope();
        return functionExpr;
      }
      else
        exp = declareVariable(exp, endGroupTypes);
    }
  }

  int count = 0;
  Expr **expList = NULL;

  while (!check(endGroupTypes) && !check(TOKEN_EOF)) {
    Expr *exp2 = parsePrecedence((Precedence)(PREC_NONE + 1));

    if (!exp2)
      error("Expect expression.");

    if (!expList) {
      expList = RESIZE_ARRAY(Expr *, expList, count, count + 1);
      expList[count++] = exp;
    }

    expList = RESIZE_ARRAY(Expr *, expList, count, count + 1);
    expList[count++] = exp2;
  }

  return expList ? new ListExpr(count, expList) : exp;
}

Expr *Parser::varDeclaration(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = parseVariable(tokens, "Expect variable name.");

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' or newline after variable declaration.");

  return exp;
}

Expr *Parser::expressionStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' or newline after expression.");

  statementExprs |= 1 << scopeDepth;
  return exp;
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
    body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{", 1, -1), 2, expList, NULL);
    ((GroupingExpr *) body)->_compiler.groupingExpr = (GroupingExpr *) body;
  }

  body = new WhileExpr(condition, body);

  if (initializer != NULL) {
    Expr **expList = RESIZE_ARRAY(Expr *, NULL, 0, 2);

    expList[0] = initializer;
    expList[1] = body;
    body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{", 1, -1), 2, expList, NULL);
    ((GroupingExpr *) body)->_compiler.groupingExpr = (GroupingExpr *) body;
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

  return new IfExpr(condExp, thenExp, elseExp);
}

Expr *Parser::whileStatement(TokenType endGroupType) {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");

  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = expression(tokens);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  return new WhileExpr(exp, statement(endGroupType));
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
  else if (match(TOKEN_RETURN))
    return returnStatement(endGroupType);
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
    case TOKEN_RETURN:
      return;

    default:; // Do nothing.
    }

    advance();
  }
}