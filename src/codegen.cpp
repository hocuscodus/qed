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
#include <stdlib.h>
#include <string.h>
#include <array>
#include <set>
#include "codegen.hpp"
#include "attrset.hpp"
#include "debug.hpp"

CodeGenerator::CodeGenerator(Parser &parser, ObjFunction *function) : ExprVisitor(), parser(parser) {
  this->function = function;
}

void CodeGenerator::visitAssignExpr(AssignExpr *expr) {
//  accept<int>(expr->lvalue, 0);
  accept<int>(expr->value, 0);

  switch (expr->op.type) {
    case TOKEN_EQUAL:          emitBytes(expr->varExp->upvalueFlag ? OP_SET_UPVALUE : OP_SET_LOCAL, expr->varExp->index); break;
    default: return; // Unreachable.
  }
}

void CodeGenerator::visitUIAttributeExpr(UIAttributeExpr *expr) {
  parser.error("Cannot generate UI code from UI expression.");
}

void CodeGenerator::visitUIDirectiveExpr(UIDirectiveExpr *expr) {
  parser.error("Cannot generate UI code from UI expression.");
}

void CodeGenerator::visitBinaryExpr(BinaryExpr *expr) {
  if (expr->op.type == TOKEN_WHILE) {
    int loopStart = currentChunk()->count;

    accept<int>(expr->left, 0);

    int exitJump = emitJump(OP_POP_JUMP_IF_FALSE);

    accept<int>(expr->right, 0);
    emitLoop(loopStart);
    patchJump(exitJump);
    return;
  }

//  if (expr->left)
    accept<int>(expr->left, 0);
//  else
//    emitConstant(FLOAT_VAL(-1));

  if (expr->right)
    accept<int>(expr->right, 0);
  else
    emitConstant(FLOAT_VAL(-1));

  emitByte(expr->opCode);

  if (expr->notFlag)
    emitByte(OP_NOT);
}

void CodeGenerator::visitCallExpr(CallExpr *expr) {
  accept<int>(expr->callee, 0);

  for (int index = 0; index < expr->count; index++)
    accept<int>(expr->arguments[index]);

  if (expr->newFlag)
    if (expr->handler)
      accept<int>(expr->handler);
    else
      emitConstant(INT_VAL(-1));

  emitBytes(expr->newFlag ? OP_NEW : OP_CALL, expr->count);
}

void CodeGenerator::visitDeclarationExpr(DeclarationExpr *expr) {
  accept<int>(expr->initExpr, 0);
}

void CodeGenerator::visitFunctionExpr(FunctionExpr *expr) {
  CodeGenerator generator(parser, expr->function);
  generator.emitCode(expr->body);
  generator.endCompiler();

  if (parser.hadError)
    return;

  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(expr->function)));

  for (int i = 0; i < expr->function->upvalueCount; i++) {
    emitByte(expr->function->upvalues[i].isLocal ? 1 : 0);
    emitByte(expr->function->upvalues[i].index);
  }
}

void CodeGenerator::visitGetExpr(GetExpr *expr) {
  accept<int>(expr->object, 0);
  emitBytes(OP_GET_PROPERTY, expr->index);
}

void CodeGenerator::visitGroupingExpr(GroupingExpr *expr) {
  for (int index = 0; index < expr->count; index++)
    accept<int>(expr->expressions[index], 0);

  for (int popLevels = expr->popLevels - 1; popLevels >= 0; popLevels--)
    emitByte(/*current->locals[popLevels].isCaptured ? OP_CLOSE_UPVALUE : */OP_POP);

  if (expr->name.type == TOKEN_RIGHT_PAREN) {
    emitByte(OP_POP);
    emitBytes(OP_GET_LOCAL, expr->popLevels);
  }

  if (expr->ui)
    accept<int>(expr->ui, 0);
/*
  if (expr->ui != NULL && expr->ui->childrenCount) {
    ObjFunction *function = this->function->uiFunctions->operator[]("$out");

    if (function) {
      {
        CodeGenerator generator(parser, function);

        outFlag = true;
        generator.emitCode(expr->ui);
        generator.endCompiler();
        outFlag = false;

        if (parser.hadError)
          return;

        ObjFunction *viewFunction = function->uiFunctions->operator[]("recalc");

        if (viewFunction) {
          CodeGenerator generator(parser, viewFunction);

          generator.emitCode(expr->ui);
          generator.endCompiler();
        }

        Point numZones;
        int offset = 0;
        std::array<long, NUM_DIRS> arrayDirFlags = {2L, 1L};
        ValueStack<ValueStackElement> valueStack;

        expr->ui->attrSets = new ChildAttrSets(&offset, numZones, 0, arrayDirFlags, valueStack, expr->ui, 0, function);
      }

      this->function->attrSets = expr->ui->attrSets;

      if (expr->ui->attrSets != NULL) {
        for (int dir = 0; dir < NUM_DIRS; dir++) {
          int *zone = NULL;

          this->function->topSizers[dir] = new Maxer(-1);
/ *
      if (parent is ImplicitArrayDeclaration) {
//					int zFlags = attrSets.zFlags[dir];

        zone = {0};//{zFlags > 0 ? zFlags != 1 ? 1 : 0 : ctz({-zFlags - 1})};
        topSizers[dir]->put(SizerType.maxer, -1, 0, 0, 0, false, {Path()});
        topSizers[dir]->children.add(new Adder(-1));
      }
* /
          expr->ui->attrSets->parseCreateSizers(this->function->topSizers, dir, zone, Path(), Path());
          expr->ui->attrSets->parseAdjustPaths(this->function->topSizers, dir);
//          numZones[dir] = zone != NULL ? zone[0] + 1 : 1;
        }

//    subAttrsets = parent is ImplicitArrayDeclaration ? parseCreateSubSets(topSizers, new Path(), numZones) : createIntersection(-1, topSizers);
//    subAttrsets = createIntersection(-1, topSizers);
      }

      // generate input functions code here
    }
  }*/
}

void CodeGenerator::visitListExpr(ListExpr *expr) {
    switch(expr->listType) {
    case EXPR_ASSIGN: {
      AssignExpr *subExpr = (AssignExpr *) expr->expressions[1];

      accept<int>(subExpr->value, 0);
      break;
    }
    case EXPR_CALL: {
      CallExpr *subExpr = (CallExpr *) expr->expressions[1];
      CodeGenerator generator(parser, expr->function);

      if (expr->count > 2)
        generator.emitCode(expr->expressions[2]);

      generator.endCompiler();

      if (parser.hadError)
        return;
#ifdef DEBUG_PRINT_CODE
      else
        disassembleChunk(&expr->function->chunk, expr->function->name->chars);
#endif
      emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(expr->function)));

      for (int i = 0; i < expr->function->upvalueCount; i++) {
        emitByte(expr->function->upvalues[i].isLocal ? 1 : 0);
        emitByte(expr->function->upvalues[i].index);
      }
      break;
    }
    default:
      for (int index = 0; index < expr->count; index++)
        accept<int>(expr->expressions[index], 0);
      break;
    }
}

void CodeGenerator::visitLiteralExpr(LiteralExpr *expr) {
  emitConstant(VALUE(expr->type, expr->as));
}

void CodeGenerator::visitLogicalExpr(LogicalExpr *expr) {
  accept(expr->left, 0);
  int thenJump = emitJump(OP_JUMP_IF_FALSE);

  switch(expr->op.type) {
    case TOKEN_AND_AND:
      emitByte(OP_POP);
      accept(expr->right, 0);
      patchJump(thenJump);
      break;

    case TOKEN_OR_OR: {
        int endJump = emitJump(OP_JUMP);

        patchJump(thenJump);
        emitByte(OP_POP);
        accept(expr->right, 0);
        patchJump(endJump);
      }
      break;

    default: return; // Unreachable.
  }
}

void CodeGenerator::visitOpcodeExpr(OpcodeExpr *expr) {
  if (expr->right != NULL)
    expr->right->accept(this);

  emitByte(expr->op);
}

void CodeGenerator::visitReturnExpr(ReturnExpr *expr) {
  if (expr->value != NULL)
    accept<int>(expr->value, 0);

  emitByte(OP_RETURN);
}

void CodeGenerator::visitSetExpr(SetExpr *expr) {
  accept<int>(expr->object, 0);
  accept<int>(expr->value, 0);
  emitBytes(OP_SET_PROPERTY, expr->index);
}

void CodeGenerator::visitStatementExpr(StatementExpr *expr) {
  expr->expr->accept(this);
}

void CodeGenerator::visitSuperExpr(SuperExpr *expr) {
}

void CodeGenerator::visitTernaryExpr(TernaryExpr *expr) {
  expr->left->accept(this);

  int thenJump = emitJump(OP_POP_JUMP_IF_FALSE);
  expr->middle->accept(this);

  if (expr->right) {
    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    expr->right->accept(this);
    patchJump(elseJump);
  }
  else
    patchJump(thenJump);
}

void CodeGenerator::visitThisExpr(ThisExpr *expr) {
}

void CodeGenerator::visitTypeExpr(TypeExpr *expr) {
}

void CodeGenerator::visitUnaryExpr(UnaryExpr *expr) {
  accept<int>(expr->right, 0);

  switch (expr->op.type) {
    case TOKEN_PRINT:         emitByte(OP_PRINT); break;
//    case TOKEN_MINUS:         emitByte(OP_PRINT); break;
    case TOKEN_BANG:          emitByte(OP_NOT); break;
    case TOKEN_PERCENT:       emitConstant(FLOAT_VAL(100)); emitByte(OP_DIVIDE_FLOAT); break;
    default: return; // Unreachable.
  }
}

void CodeGenerator::visitVariableExpr(VariableExpr *expr) {
  emitBytes(expr->upvalueFlag ? OP_GET_UPVALUE : OP_GET_LOCAL, expr->index);
}

void CodeGenerator::visitSwapExpr(SwapExpr *expr) {
  accept<int>(expr->_expr);
}

Chunk *CodeGenerator::currentChunk() {
  return &function->chunk;
}

void CodeGenerator::emitCode(Expr *expr) {
  accept<int>(expr);
//  endCompiler();
}

void CodeGenerator::emitByte(uint8_t byte) {
  currentChunk()->writeChunk(byte, 1);//parser.previous.line);
}

void CodeGenerator::emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

void CodeGenerator::emitLoop(int loopStart) {
  emitByte(OP_JUMP);

  int offset = currentChunk()->count - loopStart + 2;

  if (offset > INT16_MAX)
    parser.error("Loop body too large.");

  emitByte((-offset >> 8) & 0xff);
  emitByte(-offset & 0xff);
}

int CodeGenerator::emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

void CodeGenerator::emitHalt() {/*
  bool isClass = function->isClass();

  if (isClass || function->type.valueType == VAL_VOID)
    emitByte(isClass ? OP_HALT : OP_RETURN);*/
  emitByte(OP_HALT);
}

uint8_t CodeGenerator::makeConstant(Value value) {
  int constant = currentChunk()->addConstant(value);

  if (constant > UINT8_MAX) {
    parser.error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

void CodeGenerator::emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

void CodeGenerator::patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > INT16_MAX) {
    parser.error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

void CodeGenerator::endCompiler() {
  emitHalt();
}
/*
fun·fib(n)·{
··fun·mult(a)·{
····n·=·n·*·a;
··}
··fun·prin()·{
····print·n;
··}
··class·c·{
····init(a,·b)·{
······this.mult·=·a;
······this.prin·=·b;
····}
··}
··return·c(mult,·prin);
}

var·n·=·fib(6);

n.prin();
n.mult(6);
n.prin();
*/