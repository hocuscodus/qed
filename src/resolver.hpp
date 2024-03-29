/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_resolver_h
#define qed_resolver_h

#include "parser.hpp"
#include <stack>

#define UI_PARSES_DEF \
    UI_PARSE_DEF( PARSE_VALUES, &Resolver::processAttrs ), \
    UI_PARSE_DEF( PARSE_AREAS, &Resolver::pushAreas ), \
    UI_PARSE_DEF( PARSE_LAYOUT, &Resolver::recalcLayout ), \
    UI_PARSE_DEF( PARSE_REFRESH, &Resolver::paint ), \
    UI_PARSE_DEF( PARSE_EVENTS, &Resolver::onEvent ), \

#define UI_PARSE_DEF( identifier, directiveFn )  identifier
typedef enum { UI_PARSES_DEF } ParseStep;
#undef UI_PARSE_DEF

struct ValueStack3 {
  int max;
	std::stack<int> *map;

  ValueStack3(int max) {
    this->max = max;
    map = new std::stack<int>[max];

    for (int index = 0; index < max; index++)
      map[index].push(-1);
  }

  ~ValueStack3() {
    delete[] map;
  }

	void push(int key, int value) {
    if (key >= 0 && key < max)
      map[key].push(value);
  }

	void pop(int key) {
    if (key >= 0 && key < max)
      map[key].pop();
  }

	int get(int key) {
    return key >= 0 && key < max ? map[key].top() : -1;
  }
};

class Resolver : public ExprVisitor {
  Parser &parser;
  Expr *exp;
  int uiParseCount;
public:
  Resolver(Parser &parser, Expr *exp);

  void visitAssignExpr(AssignExpr *expr);
  void visitUIAttributeExpr(UIAttributeExpr *expr);
  void visitUIDirectiveExpr(UIDirectiveExpr *expr);
  void visitBinaryExpr(BinaryExpr *expr);
  void visitCallExpr(CallExpr *expr);
  void visitArrayElementExpr(ArrayElementExpr *expr);
  void visitDeclarationExpr(DeclarationExpr *expr);
  void visitFunctionExpr(FunctionExpr *expr);
  void visitGetExpr(GetExpr *expr);
  void visitGroupingExpr(GroupingExpr *expr);
  void visitArrayExpr(ArrayExpr *expr);
  void visitListExpr(ListExpr *expr);
  void visitLiteralExpr(LiteralExpr *expr);
  void visitLogicalExpr(LogicalExpr *expr);
  void visitOpcodeExpr(OpcodeExpr *expr);
  void visitReturnExpr(ReturnExpr *expr);
  void visitStatementExpr(StatementExpr *expr);
  void visitSetExpr(SetExpr *expr);
  void visitSuperExpr(SuperExpr *expr);
  void visitTernaryExpr(TernaryExpr *expr);
  void visitThisExpr(ThisExpr *expr);
  void visitTypeExpr(TypeExpr *expr);
  void visitUnaryExpr(UnaryExpr *expr);
  void visitReferenceExpr(ReferenceExpr *expr);
  void visitSwapExpr(SwapExpr *expr);

  Type removeDeclaration();
  bool resolve(Compiler *compiler);
  void acceptGroupingExprUnits(GroupingExpr *expr);
  void acceptSubExpr(Expr *expr);
  Expr *parse(const char *source, int index, int replace, Expr *body);

  ParseStep getParseStep();
  int getParseDir();

  void processAttrs(UIDirectiveExpr *expr);
  void pushAreas(UIDirectiveExpr *expr);
  void recalcLayout(UIDirectiveExpr *expr);
  int adjustLayout(UIDirectiveExpr *expr);
  void paint(UIDirectiveExpr *expr);
  void onEvent(UIDirectiveExpr *expr);
};

#endif


