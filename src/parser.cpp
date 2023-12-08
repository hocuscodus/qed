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
Obj objString = {OBJ_STRING};
Type stringType = {VAL_OBJ, &objString};
Obj objAny= {OBJ_ANY};
Type anyType = {VAL_OBJ, &objAny};

GroupingExpr *makeWrapperLambda(const char *name, DeclarationExpr *param, std::function<Expr*()> bodyFn);

int getDir(char op) {
  switch (op) {
    case '_': return 1;
    case '|': return 2;
    case '\\': return 3;
    default: return 0;
  }
}

const char *newString(const char *str) {
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
      body = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), body, NULL);

    addExpr(&((GroupingExpr *) body)->body, increment, buildToken(TOKEN_SEPARATOR, ";"));
  }

  return new WhileExpr(condition, body);
}

static Expr *createArrayExpr(Expr *iteratorExprs, Expr *body) {
  int index = 0;
  Point dirs{};
  Expr *dimDecs = NULL;
  DeclarationExpr *arrayParam = newDeclarationExpr(anyType, buildToken(TOKEN_IDENTIFIER, "array"), NULL);
  DeclarationExpr *xParam = newDeclarationExpr(INT_TYPE, buildToken(TOKEN_IDENTIFIER, "x"), NULL);
  DeclarationExpr *posParam = newDeclarationExpr(OBJ_TYPE(newArray(INT_TYPE)), buildToken(TOKEN_IDENTIFIER, "pos"), NULL);
  DeclarationExpr *handlerParam = newDeclarationExpr(resolveType(new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "voidHandler_"), NULL)), buildToken(TOKEN_IDENTIFIER, "handlerFn_"), NULL);
  Expr *initBody = NULL;
  GroupingExpr *mainGroup = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), NULL, NULL);

  pushScope(mainGroup);
  addExpr(&initBody, arrayParam, buildToken(TOKEN_SEPARATOR, ";"));
  addExpr(&initBody, xParam, buildToken(TOKEN_SEPARATOR, ";"));
  addExpr(&initBody, posParam, buildToken(TOKEN_SEPARATOR, ";"));
  addExpr(&initBody, handlerParam, buildToken(TOKEN_SEPARATOR, ";"));

  for (Expr **iteratorExprPtrs = &iteratorExprs; iteratorExprPtrs; iteratorExprPtrs = cdrAddress(*iteratorExprPtrs, TOKEN_SEPARATOR)) {
    char dimName[16];
    Expr *expr = car(*iteratorExprPtrs, TOKEN_SEPARATOR);
    IteratorExpr *iteratorExpr = expr->type == EXPR_ITERATOR ? (IteratorExpr *) expr : NULL;

    sprintf(dimName, "_d%d", index);

    if (iteratorExpr) {
      int dir = getDir(iteratorExpr->op.start[1]);

      for (int ndx = 0; ndx < NUM_DIRS; ndx++)
        dirs[ndx] |= !!(dir & (1 << ndx)) << index;

      expr = iteratorExpr->value;
    }

    Token decName = buildToken(TOKEN_IDENTIFIER, newString(dimName));

    expr = newDeclarationExpr(anyType, decName, expr);
    addExpr(&mainGroup->body, expr, buildToken(TOKEN_SEPARATOR, ";"));
    checkDeclaration(*getDeclaration(expr), decName, NULL, NULL);

    if (isGroup(*iteratorExprPtrs, TOKEN_SEPARATOR))
      ((BinaryExpr *) *iteratorExprPtrs)->left = expr;
    else
      *iteratorExprPtrs = expr;

    if (iteratorExpr && iteratorExpr->name.length) {
      Expr **indexExpr = new Expr *[1];
      indexExpr[0] = new LiteralExpr(VAL_INT, {.integer = index});
      Expr *getExpr = new ArrayElementExpr(new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "pos"), NULL), buildToken(TOKEN_LEFT_BRACKET, "["), 1, indexExpr);
      Expr *initializer = newDeclarationExpr(INT_TYPE, iteratorExpr->name, getExpr);

      addExpr(&initBody, initializer, buildToken(TOKEN_SEPARATOR, ";"));
    }

    if (iteratorExpr)
      delete iteratorExpr;

    addExpr(&dimDecs, new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, newString(dimName)), NULL), buildToken(TOKEN_COMMA, ","));
    index++;
  }
  NativeExpr *array = new NativeExpr(buildToken(TOKEN_IDENTIFIER, "array[x]"));
  Expr *bodyExpr = new AssignExpr(array, buildToken(TOKEN_EQUAL, "="), body);
  ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), NULL);
  Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "handlerFn_"), NULL);
  CallExpr *postCall = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), param, NULL);

  addExpr(&initBody, bodyExpr, buildToken(TOKEN_SEPARATOR, ";"));
  addExpr(&initBody, postCall, buildToken(TOKEN_SEPARATOR, ";"));

//    GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), NULL, NULL);
//  value = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), param, NULL);
  Token nameToken = buildToken(TOKEN_IDENTIFIER, "L");
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), initBody, NULL);
  FunctionExpr *wrapperFunc = newFunctionExpr(VOID_TYPE, nameToken, 3, group, NULL);
  GroupingExpr *initExpr = new GroupingExpr(buildToken(TOKEN_LEFT_PAREN, "("), wrapperFunc, NULL);
  Expr *params = NULL;

  pushScope(initExpr);
  checkDeclaration(wrapperFunc->_declaration, nameToken, wrapperFunc, NULL);
  pushScope(wrapperFunc);
  checkDeclaration(arrayParam->_declaration, arrayParam->_declaration.name, NULL, NULL);
  checkDeclaration(xParam->_declaration, xParam->_declaration.name, NULL, NULL);
  checkDeclaration(posParam->_declaration, posParam->_declaration.name, NULL, NULL);
  checkDeclaration(handlerParam->_declaration, handlerParam->_declaration.name, NULL, NULL);

  for (int ndx = 0; ndx < index; ndx++)
    checkDeclaration(getParam(wrapperFunc, 4 + ndx)->_declaration, getParam(wrapperFunc, 4 + ndx)->_declaration.name, NULL, NULL);

  popScope();
  popScope();
  addExpr(&params, new ArrayExpr(dimDecs), buildToken(TOKEN_COMMA, ","));
  addExpr(&params, initExpr, buildToken(TOKEN_COMMA, ","));

  Expr *qedArrayExpr = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "QEDArray"), NULL);
  Expr *newArrayExpr = new CallExpr(false, qedArrayExpr, buildToken(TOKEN_LEFT_PAREN, "("), params, NULL);

  addExpr(&mainGroup->body, newArrayExpr, buildToken(TOKEN_SEPARATOR, ";"));
  popScope();
  return mainGroup;
}

static const char *getHandlerType(Type type) {
  switch (type.valueType) {
    case VAL_VOID: return "voidHandler_";
    case VAL_BOOL: return "boolHandler_";
    case VAL_INT: return "intHandler_";
    case VAL_FLOAT: return "floatHandler_";
    case VAL_OBJ:
      switch (type.objType->type) {
        case OBJ_STRING: return "stringHandler_";
        case OBJ_ANY: return "anyHandler_";
        default: return NULL;
      }
    default: return NULL;
  }
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
  expr->resolve(*parser);

  if (parser->hadError)
    return NULL;

#ifdef DEBUG_PRINT_CODE
  fprintf(stderr, "Adapted parse: ");
  expr->astPrint();
  fprintf(stderr, "\n          ");/*
  for (int i = 0; i < declarationCount; i++) {
    Token *token = &declarations[i].name;

    if (token != NULL)
      fprintf(stderr, "[ %.*s ]", token->length, token->start);
    else
      fprintf(stderr, "[ N/A ]");
  }*/
  fprintf(stderr, "\n");
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
  line(str) << "    ui_ = new UI_();\n";
  line(str) << "    layout_ = new ui_.Layout_();\n";
  line(str) << "    ctx.globalAlpha = 1.0;\n";
  line(str) << "    ctx.clearRect(0, 0, canvas.width, canvas.height);\n";
  line(str) << "    layout_.paint(0, 0, layout_.size >> 16, layout_.size & 65535);\n";
  line(str) << "//  }\n";
  line(str) << "}\n";
  line(str) << "executeEvents_();\n";
  line(str) << "_refresh();\n";
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
  GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_EOF, ""), NULL, NULL);
  FunctionExpr *functionExpr = newFunctionExpr(VOID_TYPE, buildToken(TOKEN_IDENTIFIER, "Main_"), 0, group, NULL);

  pushScope(functionExpr);
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
  popScope();
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

    return *addExpr(&left, right, buildToken(TOKEN_SEPARATOR, ","));
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
    return new AssignExpr(left, op, right);

  case EXPR_GET: {
    GetExpr *getExpr = (GetExpr *) left;

    return new SetExpr(getExpr->object, getExpr->name, op, right, -1);
  }

  case EXPR_NATIVE:
  case EXPR_ARRAYELEMENT: {
//    GetExpr *getExpr = (GetExpr *) left;

    return new AssignExpr(left, op, right);
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

  return new GetExpr(left, previous, -1);
}

Expr *Parser::call(Expr *left) {
  TokenType tokens[] = {TOKEN_RIGHT_PAREN, TOKEN_COMMA, TOKEN_ELSE, TOKEN_EOF};
  Expr *params = NULL;
  Expr *handler = NULL;
  bool newFlag = this->newFlag;

  this->newFlag = false;

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
  GroupingExpr *group = new GroupingExpr(previous, NULL, NULL);

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

  pushScope(group);
  expList(group, endGroupType);
  consume(endGroupType, "Expect '%c' after expression.", closingChar);
  popScope();
/*
  if (op.type != TOKEN_LEFT_BRACE && group->body && isGroup(group->body, TOKEN_SEPARATOR)) {
    getCurrent()->hasSuperCalls |= group->hasSuperCalls;
    return new CallExpr(false, new GroupingExpr(op, newFunctionExpr(VOID_TYPE, buildToken(TOKEN_IDENTIFIER, group->hasSuperCalls ? "L" : "l"), 0, group, NULL), NULL), op, NULL, NULL);
  }
*/
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

  if (check(TOKEN_STAR) && checkNext(TOKEN_IDENTIFIER)) {
    Type type = resolveType(left);

    if (IS_FUNCTION(type)) {
      type = OBJ_TYPE(newInstance(AS_FUNCTION_TYPE(type)));
      advance();
      return left;
    }
  }

  while (precedence <= getExpRule(current.type)->precedence) {
    advance();

    left = (this->*getExpRule(previous.type)->infix)(left);
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

  if (exp == NULL)
    error("Expect expression.");

  Type returnType = resolveType(exp);

  if (returnType.valueType != -1 &&//->type == EXPR_REFERENCE &&//) {
    /*if (*/check(TOKEN_IDENTIFIER) && (checkNext(TOKEN_EQUAL) || checkNext(TOKEN_CALL) || checkNext(TOKEN_SEPARATOR))) {
    consume(TOKEN_IDENTIFIER, "Expect name identifier after type.");

    Token name = previous;

    if(match(TOKEN_CALL)) {
      GroupingExpr *group = new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), NULL, NULL);
      FunctionExpr *functionExpr = newFunctionExpr(returnType, name, 0, group, NULL);

      pushScope(functionExpr);
      passSeparator();

      if (!check(TOKEN_RIGHT_PAREN))
        do {
          DeclarationExpr *param = parseVariable(endGroupTypes, "Expect parameter name.");

          if (!IS_UNKNOWN(param->_declaration.type)) {
            addExpr(&group->body, param, buildToken(TOKEN_SEPARATOR, ";"));
            functionExpr->arity++;
//            checkDeclaration(param->_declaration, param->_declaration.name, NULL, this);
//            functionExpr->params = RESIZE_ARRAY(DeclarationExpr *, functionExpr->params, functionExpr->arity, functionExpr->arity + 1);
//            functionExpr->params[functionExpr->arity++] = param;
//            param->_declaration = group->_compiler.addDeclaration(paramType, param->name, NULL, false, this);
          }
          else
            error("Parameter %d not typed correctly", functionExpr->arity + 1);
        } while (match(TOKEN_COMMA));

      consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

      if (name.isUserClass()) {
        const char *type = getHandlerType(returnType);
        ReferenceExpr *paramTypeExpr = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, type), NULL);

        if (getCurrent()) {
          Token name = buildToken(TOKEN_IDENTIFIER, "handlerFn_");
          Type paramType = resolveType(paramTypeExpr);

          if (!IS_UNKNOWN(paramType)) {
            DeclarationExpr *dec = newDeclarationExpr(paramType, name, NULL);

            checkDeclaration(dec->_declaration, dec->_declaration.name, NULL, this);
            addExpr(&group->body, dec, buildToken(TOKEN_SEPARATOR, ";"));
          }
          else
            error("Parameter %d not typed correctly", functionExpr->arity + 1);
        }
        else
          functionExpr->arity++;
      }

      bool parenFlag = match(TOKEN_LEFT_PAREN) || match(TOKEN_CALL);
      TokenType endGroupType = parenFlag ? TOKEN_RIGHT_PAREN : TOKEN_RIGHT_BRACE;

      if (!parenFlag)
        consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

      group->name = previous;
      popScope();
      /*functionExpr->_declaration = */checkDeclaration(functionExpr->_declaration, name, functionExpr, this);
      pushScope(functionExpr);
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
      popScope();
      return functionExpr;
    }
    else
      return declareVariable(exp, endGroupTypes);
//    }
  }
  else {
    Expr *iteratorExprs = NULL;

    while (!check(endGroupTypes) && !check(TOKEN_EOF)) {
      addExpr(&iteratorExprs, exp, buildToken(TOKEN_SEPARATOR, ","));
      exp = parsePrecedence((Precedence)(PREC_NONE + 1));

      if (!exp)
        error("Expect expression.");
    }

    if ((*getLastBodyExpr(&exp, TOKEN_SEPARATOR))->type == EXPR_ITERATOR)
      error("Cannot define an iterator list without a body expression.");

    if (iteratorExprs)
      exp = createArrayExpr(iteratorExprs, exp);

    return exp;
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

  GroupingExpr *group = match(TOKEN_SEPARATOR) ? NULL : new GroupingExpr(buildToken(TOKEN_LEFT_BRACE, "{"), NULL, NULL);

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

Expr *Parser::returnStatement(TokenType endGroupType) {
  TokenType tokens[] = {TOKEN_SEPARATOR, endGroupType, TOKEN_ELSE, TOKEN_EOF};
  Token keyword = previous;
  Expr *value = NULL;

  if (!check(tokens))
    value = expression(tokens);

  if (!check(endGroupType))
    consume(TOKEN_SEPARATOR, "Expect ';' after return value.");

  if (getFunction()->_declaration.name.isUserClass()) {
    ReferenceExpr *callee = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "post_"), NULL);
    Expr *param = new ReferenceExpr(buildToken(TOKEN_IDENTIFIER, "handlerFn_"), NULL);

    if (value) {
      CallExpr *call = new CallExpr(false, param, buildToken(TOKEN_LEFT_PAREN, "("), value, NULL);

      param = makeWrapperLambda("lambda_", NULL, [call]() {return call;});
    }

    value = new CallExpr(false, callee, buildToken(TOKEN_LEFT_PAREN, "("), param, NULL);
  }

  return new ReturnExpr(keyword, getFunction()->_declaration.name.isUserClass(), value);
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