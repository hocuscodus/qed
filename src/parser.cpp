/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include "parser.hpp"
#include "memory.h"
#include "object.hpp"
#include "listunitareas.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define FORMAT_MESSAGE(fmt) \
  char message[256]; \
  va_list args; \
 \
  va_start (args, fmt); \
  vsnprintf(message, 256, fmt, args); \
  va_end(args);

#define KEY_DEF( identifier, unary, binary, prec )  { unary, binary, prec }
ParseExpRule expRules[] = { KEYS_DEF };
#undef KEY_DEF
Obj objString = {OBJ_STRING, NULL};
Type stringType = {VAL_OBJ, &objString};

ParseExpRule *getExpRule(TokenType type) {
  return &expRules[type];
}

static int scopeDepth = -1;

Parser::Parser(Scanner &scanner) : scanner(scanner) {
  hadError = false;
  panicMode = false;
  newFlag = false;

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

void Parser::consume(TokenType type, const char *fmt, ...) {
  if (current.type == type) {
    advance();
    return;
  }

  FORMAT_MESSAGE(fmt);
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

void Parser::errorAtCurrent(const char *fmt, ...) {
  FORMAT_MESSAGE(fmt);
  errorAt(&current, message);
}

void Parser::error(const char *fmt, ...) {
  if (this == NULL)
    return;
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
  FunctionExpr *expr = parse();

  return expr ? expr->body->_compiler.compile(expr, this) : NULL;
}

FunctionExpr *Parser::parse() {
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
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_EOF, ""), NULL);
  FunctionExpr *functionExpr = new FunctionExpr(VOID_TYPE, buildToken(TOKEN_IDENTIFIER, "Main"), 0, NULL, group, NULL);

  functionExpr->_function.expr = functionExpr;
  group->_compiler.pushScope(&functionExpr->_function, this);
  expList(group, TOKEN_EOF);

  if (check(TOKEN_LESS)) {
    int offset = 0;
    Point zoneOffsets;
    std::array<long, NUM_DIRS> arrayDirFlags;
    ValueStack<ValueStackElement> valueStack;

    functionExpr->ui = directive(TOKEN_EOF, NULL);
//		attrSets = numAttrSets != 0 ? new ChildAttrSets([0], numZones, 0, [2, 1], new ValueStack(), numAttrSets, mainFunc, this, inputStream, 0) : NULL;
//ChildAttrSets::ChildAttrSets(int *offset, Point &zoneOffsets, int childDir, std::array<long, NUM_DIRS> &arrayDirFlags,
//                             ValueStack<ValueStackElement> &valueStack, UIDirectiveExpr *directiveExpr, int parentRefreshFlags,
//                             ObjFunction *function) : std::vector<AttrSet *>() {
//    ((UIDirectiveExpr *) functionExpr->ui)->_attrSet.init(&offset, zoneOffsets, arrayDirFlags, valueStack, (UIDirectiveExpr *) functionExpr->ui, 0, &functionExpr->_function);
  }

  consume(TOKEN_EOF, "Expect end of file.");
  group->_compiler.popScope();
////////
/*  Compiler *compiler = new Compiler;

  compiler->parser = this;
  compiler->pushScope(newFunction(VOID_TYPE, NULL, 0));

  Expr *expr = grouping(TOKEN_EOF, compiler, "Expect end of file.");

  compiler->popScope();
*/
  return hadError ? NULL : functionExpr;
}

void Parser::passSeparator() {
  match(TOKEN_SEPARATOR);
}

Expr *Parser::anonymousIterator() {
  return iterator(NULL);
}

Expr *Parser::iterator(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  if (right == NULL)
    error("Expect expression.");

  if (left && left->type == EXPR_REFERENCE)
    return new IteratorExpr(((ReferenceExpr *) left)->name, op, right);
  else {
    if (right->type != EXPR_ITERATOR && op.start[1] != ':')
      right = new IteratorExpr(buildToken(TOKEN_IDENTIFIER, ""), op, right);

    return *addExpr(&left, right, buildToken(TOKEN_COMMA, ","));
  }
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
  Expr *params = NULL;
  Expr *handler = NULL;
  bool newFlag = this->newFlag;

  this->newFlag = false;

  if (!newFlag)
    getCurrent()->hasSuperCalls |= false;//isUserClass(left);

  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      Token comma = previous;
      Expr *expr = expression(tokens);

//      if (argCount == 255)
//        error("Can't have more than 255 arguments.");

      addExpr(&params, expr, comma);
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

  return new CallExpr(newFlag, left, previous, params, handler);
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
  Token op = previous;
  GroupingExpr *group = new GroupingExpr(previous, NULL);

  switch (op.type) {
  case TOKEN_LEFT_PAREN:
  case TOKEN_CALL:
    closingChar = ')';
    endGroupType = TOKEN_RIGHT_PAREN;
    break;

  case TOKEN_LEFT_BRACE:
    closingChar = '}';
    endGroupType = TOKEN_RIGHT_BRACE;
    break;
  }

  group->_compiler.pushScope(group);
  expList(group, endGroupType);
  consume(endGroupType, "Expect '%c' after expression.", closingChar);
  group->_compiler.popScope();

  if (op.type != TOKEN_LEFT_BRACE && group->body && isGroup(group->body, TOKEN_SEPARATOR)) {
    getCurrent()->hasSuperCalls |= group->_compiler.hasSuperCalls;
    return new CallExpr(false, new GroupingExpr(op, new FunctionExpr(VOID_TYPE, buildToken(TOKEN_IDENTIFIER, group->_compiler.hasSuperCalls ? "L" : "l"), 0, NULL, group, NULL)), op, NULL, NULL);
  }

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

void Parser::expList(GroupingExpr *groupingExpr, TokenType endGroupType) {
  passSeparator();
  scopeDepth++;

  int count = 0;

  while (!check(endGroupType) && !check(TOKEN_EOF)) {
    if (endGroupType != TOKEN_RIGHT_PAREN && check(TOKEN_LESS))
      break;
    else {
      Token op = !count++ ? buildToken(TOKEN_SEPARATOR, ";") : previous;
      Expr *exp = declaration(endGroupType);

      if (groupingExpr->body && op.type != TOKEN_SEPARATOR)
        op.type = TOKEN_SEPARATOR;

      addExpr(&groupingExpr->body, exp, op);
    }
  }

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
      if (newFlag)
        errorAt(&op, "Use 'new' once before call");

      newFlag = true;

      Expr *right = parsePrecedence(PREC_CALL);

      if (right->type != EXPR_CALL)
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

  if (exp->type == EXPR_REFERENCE &&//) {
    /*if (*/check(TOKEN_IDENTIFIER) && (checkNext(TOKEN_EQUAL) || checkNext(TOKEN_CALL) || checkNext(TOKEN_SEPARATOR))) {
    consume(TOKEN_IDENTIFIER, "Expect name identifier after type.");

    Token name = previous;

    if(match(TOKEN_CALL)) {
      Type returnType = exp->resolve(*this);
      GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), NULL);
      FunctionExpr *functionExpr = new FunctionExpr(returnType, name, 0, NULL, group, NULL);

      functionExpr->_function.expr = functionExpr;
      group->_compiler.pushScope(&functionExpr->_function, this);
      passSeparator();

      if (!check(TOKEN_RIGHT_PAREN))
        do {
          DeclarationExpr *param = parseVariable(endGroupTypes, "Expect parameter name.");
          Type paramType = param->typeExpr->resolve(*this);

          if (!IS_UNKNOWN(paramType)) {
            functionExpr->params = RESIZE_ARRAY(DeclarationExpr *, functionExpr->params, functionExpr->arity, functionExpr->arity + 1);
            functionExpr->params[functionExpr->arity++] = param;
            param->_declaration = group->_compiler.addDeclaration(paramType, param->name, NULL, false, this);
          }
          else
            error("Parameter %d not typed correctly", functionExpr->arity + 1);
        } while (match(TOKEN_COMMA));

      consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

      bool parenFlag = match(TOKEN_LEFT_PAREN) || match(TOKEN_CALL);
      TokenType endGroupType = parenFlag ? TOKEN_RIGHT_PAREN : TOKEN_RIGHT_BRACE;

      if (!parenFlag)
        consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

      group->name = previous;
      functionExpr->_declaration = getCurrent()->enclosing->checkDeclaration(OBJ_TYPE(&functionExpr->_function), name, &functionExpr->_function, this);
      expList(group, endGroupType);

      if (check(TOKEN_LESS)) {
        int offset = 0;
        Point zoneOffsets;
        std::array<long, NUM_DIRS> arrayDirFlags;
        ValueStack<ValueStackElement> valueStack;

        functionExpr->ui = directive(parenFlag ? TOKEN_RIGHT_PAREN : TOKEN_RIGHT_BRACE, NULL);
        ((UIDirectiveExpr *) functionExpr->ui)->_attrSet.init(&offset, zoneOffsets, arrayDirFlags, valueStack, (UIDirectiveExpr *) functionExpr->ui, 0, &functionExpr->_function);
      }

      consume(endGroupType, "Expect '%c' after expression.", parenFlag ? ')' : '}');
      group->_compiler.popScope();
      return functionExpr;
    }
    else
      return declareVariable(exp, endGroupTypes);
//    }
  }
  else {
    Expr *iteratorExprs = NULL;

    while (!check(endGroupTypes) && !check(TOKEN_EOF)) {
      addExpr(&iteratorExprs, exp, buildToken(TOKEN_COMMA, ","));
      exp = parsePrecedence((Precedence)(PREC_NONE + 1));

      if (!exp)
        error("Expect expression.");
    }

    if ((*getLastBodyExpr(&exp, TOKEN_COMMA))->type == EXPR_ITERATOR)
      error("Cannot define an iterator list without a body expression.");

    return iteratorExprs ? new GroupingExpr(buildToken(TOKEN_COMMA, ","), *addExpr(&iteratorExprs, exp, buildToken(TOKEN_COMMA, ","))) : exp;
  }
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

  return exp;
}

Expr *Parser::forStatement(TokenType endGroupType) {
  if (!match(TOKEN_CALL))
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
//    body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), new BinaryExpr(new UnaryExpr(buildToken(TOKEN_PRINT, "print"), body), buildToken(TOKEN_SEPARATOR, ";"), new UnaryExpr(buildToken(TOKEN_PRINT, "print"), increment)));
    ((GroupingExpr *) body)->_compiler.groupingExpr = (GroupingExpr *) body;
  }

  body = new WhileExpr(condition, body);

  if (initializer != NULL) {
    body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), new BinaryExpr(initializer, buildToken(TOKEN_SEPARATOR, ";"), body));
    ((GroupingExpr *) body)->_compiler.groupingExpr = (GroupingExpr *) body;
  }

  return body;
}

Expr *Parser::ifStatement(TokenType endGroupType) {
  Token name = previous;

  if (!match(TOKEN_CALL))
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");

  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_ELSE, TOKEN_EOF};
  Expr *condExp = expression(tokens);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

  Expr *thenExp = statement(endGroupType);
  Expr *elseExp = match(TOKEN_ELSE) ? statement(endGroupType) : NULL;

  return new IfExpr(condExp, thenExp, elseExp);
}

Expr *Parser::whileStatement(TokenType endGroupType) {
  if (!match(TOKEN_CALL))
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

GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, Type paramType, std::function<Expr*()> bodyFn);
Expr *Parser::returnStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Token keyword = previous;
  Expr *value = NULL;

  if (!check(tokens))
    value = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' after return value.");

  if (getCurrent()->function->isUserClass()) {
    ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), UNKNOWN_TYPE);
    Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "handlerFn_"), UNKNOWN_TYPE);

    if (value) {
      CallExpr *call = new CallExpr(false, param, buildToken(TOKEN_CALL, "("), value, NULL);

      param = makeWrapperLambda("lambda_", NULL, UNKNOWN_TYPE, [call]() {return call;});
    }

    value = new CallExpr(false, callee, buildToken(TOKEN_CALL, "("), param, NULL);
  }

  return new ReturnExpr(keyword, getCurrent()->function->isUserClass(), value);
}

Expr *Parser::statement(TokenType endGroupType) {
  // ignore optional separator before second operand
  passSeparator();

  if (match(TOKEN_RETURN))
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
    case TOKEN_TYPE_LITERAL:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_FOR:
    case TOKEN_RETURN:
      return;

    default:; // Do nothing.
    }

    advance();
  }
}