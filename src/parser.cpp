/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdarg.h>
#include <string.h>
#include "parser.hpp"
#include "memory.h"
#include "compiler.hpp"

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

static int getDir(char op) {
  switch (op) {
    case '_': return 1;
    case '|': return 2;
    case '\\': return 3;
    default: return 0;
  }
}

static const char *newString(const char *str) {
  char *newStr = new char[strlen(str) + 1];

  strcpy(newStr, str);
  return newStr;
}

ParseExpRule *getExpRule(TokenType type) {
  return &expRules[type];
}

static Expr *createWhileExpr(Expr *condition, Expr *increment, Expr *body) {
  if (increment != NULL) {
    if (body->type != EXPR_GROUPING)
      body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), body, NULL, NULL);

    addExpr(&((GroupingExpr *) body)->body, increment, buildToken(TOKEN_SEPARATOR, ";"));
  }

  return new WhileExpr(condition, body);
}

Parser::Parser(Scanner &scanner) : scanner(scanner) {
  hadError = false;
  panicMode = false;
  newFlag = false;
  next = buildToken(TOKEN_SEPARATOR, ";");

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
std::stringstream &line(std::stringstream &str);

std::string compile(FunctionExpr *expr, Parser *parser) {
#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "Original parse: ");
  expr->astPrint();
  fprintf(stderr, "\n");
#endif
  expr->findTypes(*parser);
//  while (expr->findTypes(*parser));
  fprintf(stderr, "Symbol parse: ");
  expr->astPrint();
  fprintf(stderr, "\n");

  expr->resolve(*parser);

  if (parser->hadError)
    return "";

#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "Type checking parse: ");
  expr->astPrint();
  fprintf(stderr, "\n\n");
#endif
  Expr *cpsExpr = expr->toCps([](Expr *expr) {return expr;});
#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "CPS parse: ");
  cpsExpr->astPrint();
  fprintf(stderr, "\n");
#endif
  std::stringstream str;
  line(str) << "\"use strict\";\n";
  line(str) << "const canvas = document.getElementById(\"canvas\");\n";
  line(str) << "let postHandler = null;\n";
  line(str) << "let attributeStacks = [];\n";
  line(str) << "const ctx = canvas.getContext(\"2d\");\n";
  line(str) << "function toColor(color) {return \"#\" + color.toString(16).padStart(6, '0');}\n";
  line(str) << "let getAttribute = function(index) {\n";
  line(str) << "  return attributeStacks[index][attributeStacks[index].length - 1];\n";
  line(str) << "}\n";

  str << cpsExpr->toCode(*parser, &expr->_function);

  line(str) << "pushAttribute(4, 20);\n";
  line(str) << "pushAttribute(10, 0);\n";
  line(str) << "pushAttribute(11, 1.0);\n";
  line(str) << "let layout_ = null;\n";
  line(str) << "function _refresh() {\n";
  line(str) << "//  if (ui_ != undefined && --postCount == 0) {\n";
  line(str) << "    setUI_();\n";
  line(str) << "    layout_ = new ui_.Layout_();\n";
  line(str) << "    ctx.globalAlpha = 1.0;\n";
  line(str) << "    ctx.clearRect(0, 0, canvas.width, canvas.height);\n";
  line(str) << "    layout_.paint(0, 0, layout_.size >> 16, layout_.size & 65535);\n";
  line(str) << "//  }\n";
  line(str) << "}\n";
  line(str) << "executeEvents_();\n";
  line(str) << "canvas.addEventListener(\"mousedown\", function(ev) {\n";
  line(str) << "  postCount++;\n";
  line(str) << "  var rect = canvas.getBoundingClientRect();\n";
  line(str) << "  layout_.onEvent(0, ev.clientX - rect.left, ev.clientY - rect.top, layout_.size >> 16, layout_.size & 65535);\n";
  line(str) << "  _refresh();\n";
  line(str) << "  });\n";
  line(str) << "canvas.addEventListener(\"mouseup\", function(ev) {\n";
  line(str) << "  postCount++;\n";
  line(str) << "  var rect = canvas.getBoundingClientRect();\n";
  line(str) << "  layout_.onEvent(1, ev.clientX - rect.left, ev.clientY - rect.top, layout_.size >> 16, layout_.size & 65535);\n";
  line(str) << "  _refresh();\n";
  line(str) << "});\n";
  line(str) << "canvas.onselectstart = function () { return false; }\n";

  return parser->hadError ? std::string() : str.str();
}

std::string Parser::compile() {
  FunctionExpr *expr = parse();

  return expr ? ::compile(expr, this) : std::string();
}

FunctionExpr *Parser::parse() {
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_EOF, ""), NULL, NULL, NULL);
  FunctionExpr *functionExpr = newFunctionExpr(VOID_TYPE, buildToken(TOKEN_IDENTIFIER, "Main_"), 0, NULL, group);

  pushScope(functionExpr);
  expList(group, TOKEN_EOF);

  if (check(TOKEN_LESS)) {
    int offset = 0;
    Point zoneOffsets;
    std::array<long, NUM_DIRS> arrayDirFlags;
    ValueStack<ValueStackElement> valueStack;

    functionExpr->body->ui = directive(TOKEN_EOF, NULL);
//		attrSets = numAttrSets != 0 ? new ChildAttrSets([0], numZones, 0, [2, 1], new ValueStack(), numAttrSets, mainFunc, this, inputStream, 0) : NULL;
//ChildAttrSets::ChildAttrSets(int *offset, Point &zoneOffsets, int childDir, std::array<long, NUM_DIRS> &arrayDirFlags,
//                             ValueStack<ValueStackElement> &valueStack, UIDirectiveExpr *directiveExpr, int parentRefreshFlags,
//                             ObjFunction *function) : std::vector<AttrSet *>() {
//    ((UIDirectiveExpr *) functionExpr->ui)->_attrSet.init(&offset, zoneOffsets, arrayDirFlags, valueStack, (UIDirectiveExpr *) functionExpr->ui, 0, &functionExpr->_function);
  }

  consume(TOKEN_EOF, "Expect end of file.");
  popScope();
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

    return *addExpr(&left, right, buildToken(TOKEN_SEPARATOR, ","));
  }
}

Expr *Parser::assignment(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 0));

  if (!right)
    error("Expect expression.");

  switch (left->type) {
    case EXPR_REFERENCE:
      return new AssignExpr(left, op, right);

    case EXPR_GET: {
      GetExpr *getExpr = (GetExpr *) left;

      return new SetExpr(getExpr->object, getExpr->name, op, right);
    }

    case EXPR_NATIVE:
    case EXPR_ARRAYELEMENT:default: {
  //    GetExpr *getExpr = (GetExpr *) left;

      return new AssignExpr(left, op, right);
    }
  //  default:
      //TODO: remove comment when fixed
  //    errorAt(&op, "Invalid assignment target."); // [no-throw]
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

Expr *Parser::as(Expr *expr) {
  // ignore optional separator before second operand
  passSeparator();

  Expr *typeExpr = parsePrecedence((Precedence)(getExpRule(TOKEN_AS)->precedence + 1));
  Type type = typeExpr ? resolveType(typeExpr) : UNKNOWN_TYPE;

  if (IS_UNKNOWN(type))
    error("Expect type.");

  return new CastExpr(type, expr);
}

Expr *Parser::binary(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  if (right == NULL)
    error("Expect expression.");

  return new BinaryExpr(left, op, right);
}

Expr *Parser::rightBinary(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 0));

  if (right == NULL)
    error("Expect expression.");

  return new BinaryExpr(left, op, right);
}

Expr *Parser::binaryOrPostfix(Expr *left) {
  Token op = previous;
  ParseExpRule *rule = getExpRule(op.type);

  // ignore optional separator before second operand
  passSeparator();

  Expr *right = parsePrecedence((Precedence)(rule->precedence + 1));

  return new BinaryExpr(left, op, right);
}

Expr *Parser::suffix(Expr *left) {
  Token op = previous;

  // Emit the operator instruction.
  switch (op.type) {
    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS_MINUS:
      return new AssignExpr((ReferenceExpr *) left, op, NULL);

    case TOKEN_PERCENT:
      if (getExpRule(current.type)->prefix) {
        Expr *right = parsePrecedence((Precedence)(getExpRule(TOKEN_PERCENT)->precedence + 1));

        if (right != NULL)
          return new BinaryExpr(left, op, right);
        else
          error("Expect expression.");
      }
      // no break statement, intended

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

  return new GetExpr(left, previous);
}

Expr *Parser::call(Expr *left) {
  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_COMMA, TOKEN_ELSE, TOKEN_EOF};
  Expr *params = NULL;
  Expr *handler = NULL;
  Token op = previous;
  bool newFlag = this->newFlag;

  this->newFlag = false;

  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      Token comma = previous;
      Expr *expr = expression(tokens);

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

  return new CallExpr(newFlag, left, op, params, handler);
}

Expr *Parser::arrayElement(Expr *left) {
  Token op = previous;
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

  return new ArrayElementExpr(left, op, indexCount, expList);
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
  return new SwapExpr(NULL);
}

Expr *Parser::primitiveType() {
  switch (previous.start[0]) {
  case 'v':
    switch (previous.start[1]) {
    case 'a':
      return new PrimitiveExpr(previous, anyType);
    case 'o':
      return new PrimitiveExpr(previous, VOID_TYPE);
    }
  case 'b': return new PrimitiveExpr(previous, BOOL_TYPE);
  case 'i': return new PrimitiveExpr(previous, INT_TYPE);
  case 'f': return new PrimitiveExpr(previous, FLOAT_TYPE);
  case 'S': return new PrimitiveExpr(previous, stringType);
  default: return NULL; // Unreachable.
  }
}

Expr *Parser::grouping() {
  char closingChar;
  TokenType endGroupType;
  Token op = previous;
  bool parenFlag = false;
  GroupingExpr *group = new GroupingExpr(previous, NULL, NULL, NULL);

  switch (op.type) {
  case TOKEN_LEFT_PAREN:
  case TOKEN_CALL:
    closingChar = ')';
    endGroupType = TOKEN_RIGHT_PAREN;
    parenFlag = true;
    break;

  case TOKEN_LEFT_BRACE:
    closingChar = '}';
    endGroupType = TOKEN_RIGHT_BRACE;
    break;
  }

  pushScope(group);
  expList(group, endGroupType);

  if (op.type == TOKEN_LEFT_BRACE && check(TOKEN_LESS)) {
    int offset = 0;
    Point zoneOffsets;
    std::array<long, NUM_DIRS> arrayDirFlags;
    ValueStack<ValueStackElement> valueStack;

    group->ui = directive(parenFlag ? TOKEN_RIGHT_PAREN : TOKEN_RIGHT_BRACE, NULL);
//        ((UIDirectiveExpr *) functionExpr->ui)->_attrSet.init(&offset, zoneOffsets, arrayDirFlags, valueStack, (UIDirectiveExpr *) functionExpr->ui, 0, &functionExpr->_function);
  }

  consume(endGroupType, "Expect '%c' after expression.", closingChar);
  popScope();
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
  int childDir = match(TOKEN_UNDERLINE) || match(TOKEN_OR) || match(TOKEN_BACKSLASH) ? getDir(this->previous.start[0]) : 0;

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

  while (!check(endGroupType) && !check(TOKEN_EOF)) {
    if (endGroupType != TOKEN_RIGHT_PAREN && check(TOKEN_LESS))
      break;
    else {
      Token op = previous;
      Expr *exp = declaration(endGroupType);

      if (groupingExpr->body && op.type != TOKEN_SEPARATOR)
        op = buildToken(TOKEN_SEPARATOR, ";");

      addExpr(&groupingExpr->body, exp, op);
    }
  }
}

Expr *Parser::array() {
  TokenType tokens[] = {TOKEN_RIGHT_BRACKET, TOKEN_COMMA, TOKEN_ELSE, TOKEN_EOF};
  Expr *body = NULL;

  while (!check(TOKEN_RIGHT_BRACKET) && !check(TOKEN_EOF) && (!body || match(TOKEN_COMMA))) {
    Token op = previous;

    passSeparator();

    Expr *exp = expression(tokens);

    addExpr(&body, exp, op);
    passSeparator();
  }

  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression.");
  return new ArrayExpr(body);
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
  return new ReferenceExpr(previous, NULL);
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

  while (true) {
    ParseExpRule *rule = getExpRule(current.type);
    bool isIdentifier = rule->prefix && !rule->infix;

    if (isIdentifier)
      rule = getExpRule(TOKEN_ARRAY);

    if (precedence <= rule->precedence) {
      if (isIdentifier)
        previous = buildToken(TOKEN_ARRAY, "$$");
      else
        advance();

      left = (this->*rule->infix)(left);
    }
    else
      break;
  }

  return left;
}

DeclarationExpr *Parser::declareVariable(Expr *typeExpr, TokenType *endGroupTypes) {
  Token name = previous;
  Expr *expr = match(TOKEN_EQUAL) ? expression(endGroupTypes) : NULL;
  Type decType = resolveType(typeExpr);
  DeclarationExpr *dec = newDeclarationExpr(decType, name, expr);

  checkDeclaration(dec->_declaration, name, NULL, this);
  return dec;
}

DeclarationExpr *Parser::parseVariable(TokenType *endGroupTypes, const char *errorMessage) {
  Expr *typeExp = parsePrecedence((Precedence)(PREC_NONE + 1));

  consume(TOKEN_IDENTIFIER, errorMessage);
  return declareVariable(typeExp, endGroupTypes);
}

Expr *Parser::expression(TokenType *endGroupTypes) {
  Expr *exp = parsePrecedence((Precedence)(PREC_NONE + 1));

  if (!exp)
    error("Expect expression.");

  return exp;
}

Expr *Parser::varDeclaration(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = parseVariable(tokens, "Expect variable name.");

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' or newline after variable declaration.");

  return exp;
}

Expr *Parser::expressionStatement(TokenType endGroupType) {
  Token cur = current;
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Expr *exp = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' or newline after expression.");

  return exp;
}

Expr *Parser::forStatement(TokenType endGroupType) {
  if (!match(TOKEN_CALL))
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

  GroupingExpr *group = match(TOKEN_SEPARATOR) ? NULL : new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), NULL, NULL, NULL);

  if (group) {
    pushScope(group);
    addExpr(&group->body, match(TOKEN_VAR) ? varDeclaration(endGroupType) : expressionStatement(endGroupType), buildToken(TOKEN_SEPARATOR, ";"));
  }

  TokenType tokens[] = {TOKEN_SEPARATOR, TOKEN_ELSE, TOKEN_EOF};
  Expr *condition = check(TOKEN_SEPARATOR) ? createBooleanExpr(true) : expression(tokens);

  consume(TOKEN_SEPARATOR, "Expect ';' or newline after loop condition.");

  TokenType tokens2[] = {TOKEN_RIGHT_PAREN, TOKEN_SEPARATOR, TOKEN_ELSE, TOKEN_EOF};
  Expr *increment = check(TOKEN_RIGHT_PAREN) ? NULL : expression(tokens2);

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

  Expr *whileExp = createWhileExpr(condition, increment, statement(endGroupType));

  if (group) {
    addExpr(&group->body, whileExp, buildToken(TOKEN_SEPARATOR, ";"));
    popScope();
  }

  return group ? group : whileExp;
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
  Expr *exp = statement(endGroupType);

  if (panicMode)
    synchronize();

  return exp;
}

Expr *Parser::returnStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Token keyword = previous;
  Expr *value = NULL;

  if (!check(tokens))
    value = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' after return value.");

  return new ReturnExpr(keyword, false, value);
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