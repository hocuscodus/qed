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
#ifndef qed_resolver_h
#define qed_resolver_h

#include "compiler.hpp"

#define UI_PARSES_DEF \
    UI_PARSE_DEF( PARSE_VALUES, &Resolver::processAttrs ), \
    UI_PARSE_DEF( PARSE_AREAS, &Resolver::pushAreas ), \
    UI_PARSE_DEF( PARSE_LAYOUT, &Resolver::recalcLayout ), \
    UI_PARSE_DEF( PARSE_REFRESH, &Resolver::paint ), \
    UI_PARSE_DEF( PARSE_EVENTS, &Resolver::onEvent ), \

#define UI_PARSE_DEF( identifier, directiveFn )  identifier
typedef enum { UI_PARSES_DEF } ParseStep;
#undef UI_PARSE_DEF

// put above ATTRIBUTE_COLOR all size-related attributes
#define UI_ATTRIBUTES_DEF \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_OUT, "out" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_FONTSIZE, "fontSize" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_SIZE, "size" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_ZOOM, "zoom" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_COLOR, "color" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_ALIGN, "align" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_EXPAND, "expand" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_TRANSPARENCY, "transparency" ), \
    UI_ATTRIBUTE_DEF( ATTRIBUTE_END, NULL ), \

#define UI_ATTRIBUTE_DEF( identifier, text )  identifier
typedef enum { UI_ATTRIBUTES_DEF } Attribute;
#undef UI_ATTRIBUTE_DEF

#define UI_EVENTS_DEF \
    UI_EVENT_DEF( EVENT_PRESS,"onPress" ), \
    UI_EVENT_DEF( EVENT_RELEASE, "onRelease" ), \
    UI_EVENT_DEF( EVENT_END, NULL ), \

#define UI_EVENT_DEF( identifier, text )  identifier
typedef enum { UI_EVENTS_DEF } Event;
#undef UI_EVENT_DEF

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
  void visitDeclarationExpr(DeclarationExpr *expr);
  void visitFunctionExpr(FunctionExpr *expr);
  void visitGetExpr(GetExpr *expr);
  void visitGroupingExpr(GroupingExpr *expr);
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
  void visitVariableExpr(VariableExpr *expr);
  void visitSwapExpr(SwapExpr *expr);

  void checkDeclaration(Token *name);
  Type removeLocal();
  bool resolve(Compiler *compiler);
  void acceptGroupingExprUnits(GroupingExpr *expr);
  void acceptSubExpr(Expr *expr);
  Expr *parse(const char *source, int index, int replace, Expr *body);

  ParseStep getParseStep();
  int getParseDir();

  void processAttrs(UIDirectiveExpr *expr);
  void pushAreas(UIDirectiveExpr *expr);
  void recalcLayout(UIDirectiveExpr *expr);
  void paint(UIDirectiveExpr *expr);
  void onEvent(UIDirectiveExpr *expr);
};

#endif


