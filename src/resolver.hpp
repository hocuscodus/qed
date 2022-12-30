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
    UI_PARSE_DEF( PARSE_VALUES, &Resolver::processAttrs, &Resolver::evalValue ),  \
    UI_PARSE_DEF( PARSE_AREAS, &Resolver::pushAreas, NULL ),  \
    UI_PARSE_DEF( PARSE_LAYOUT, &Resolver::recalcLayout, NULL ),  \
    UI_PARSE_DEF( PARSE_REFRESH, &Resolver::paint, NULL ), \
    UI_PARSE_DEF( PARSE_EVENTS, &Resolver::onEvent, NULL ), 

#define UI_PARSE_DEF( identifier, push, attr )  identifier
typedef enum { UI_PARSES_DEF } ParseStep;

#undef UI_PARSE_DEF

class Resolver : public ExprVisitor {
  Parser &parser;
  Expr *exp;
  int uiParseCount;
public:
  Resolver(Parser &parser, Expr *exp);

  void visitAssignExpr(AssignExpr *expr);
  void visitAttributeExpr(AttributeExpr *expr);
  void visitAttributeListExpr(AttributeListExpr *expr);
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

  void checkDeclaration(Token *name);
  Type removeLocal();
  bool resolve(Compiler *compiler);
  void acceptGroupingExprUnits(GroupingExpr *expr);
  void acceptSubExpr(Expr *expr);

  ParseStep getParseStep();
  int getParseDir();

  void processAttrs(AttributeListExpr *expr);
  void parseChildren(AttributeListExpr *expr);
  void evalValue(AttributeExpr *expr);
  void pushAreas(AttributeListExpr *expr);
  void recalcLayout(AttributeListExpr *expr);
  void paint(AttributeListExpr *expr);
  void onEvent(AttributeListExpr *expr);
};

#endif


