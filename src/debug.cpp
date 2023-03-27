/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include "debug.hpp"
#include "object.hpp"
#include "value.h"

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t index = chunk->code[++offset];
  printf("%-16s %4d '", name, index);
  printValue(chunk->constants.values[index]);
  printf("'\n");
  return ++offset;
}

static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return ++offset;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
  int8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; 
}

static int byte2Instruction(const char *name, Chunk *chunk, int offset) {
  int8_t a = chunk->code[offset + 1];
  int8_t b = chunk->code[offset + 2];
  printf("%-16s %4d %4d\n", name, a, b);
  return offset + 3; 
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset,
         offset + 3 + sign * jump);
  return offset + 3;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  uint8_t instruction = chunk->code[offset];

  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);

    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);

    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);

    case OP_POP:
      return simpleInstruction("OP_POP", offset);

    case OP_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", chunk, offset);

    case OP_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", chunk, offset);

    case OP_GET_PROPERTY:
      return byteInstruction("OP_GET_PROPERTY", chunk, offset);

    case OP_SET_PROPERTY:
      return byteInstruction("OP_SET_PROPERTY", chunk, offset);

    case OP_GET_LOCAL_DIR:
      return byte2Instruction("OP_GET_LOCAL_DIR", chunk, offset);

    case OP_ADD_LOCAL:
      return byte2Instruction("OP_ADD_LOCAL", chunk, offset);

    case OP_MAX_LOCAL:
      return byte2Instruction("OP_MAX_LOCAL", chunk, offset);

    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);

    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);

    case OP_INT_TO_FLOAT:
      return simpleInstruction("OP_INT_TO_FLOAT", offset);

    case OP_FLOAT_TO_INT:
      return simpleInstruction("OP_FLOAT_TO_INT", offset);

    case OP_INT_TO_STRING:
      return simpleInstruction("OP_INT_TO_STRING", offset);

    case OP_FLOAT_TO_STRING:
      return simpleInstruction("OP_FLOAT_TO_STRING", offset);

    case OP_BOOL_TO_STRING:
      return simpleInstruction("OP_BOOL_TO_STRING", offset);

    case OP_EQUAL_STRING:
      return simpleInstruction("OP_EQUAL_STRING", offset);

    case OP_GREATER_STRING:
      return simpleInstruction("OP_GREATER_STRING", offset);

    case OP_LESS_STRING:
      return simpleInstruction("OP_LESS_STRING", offset);

    case OP_EQUAL_INT:
      return simpleInstruction("OP_EQUAL_INT", offset);

    case OP_GREATER_INT:
      return simpleInstruction("OP_GREATER_INT", offset);

    case OP_LESS_INT:
      return simpleInstruction("OP_LESS_INT", offset);

    case OP_EQUAL_FLOAT:
      return simpleInstruction("OP_EQUAL_FLOAT", offset);

    case OP_GREATER_FLOAT:
      return simpleInstruction("OP_GREATER_FLOAT", offset);

    case OP_LESS_FLOAT:
      return simpleInstruction("OP_LESS_FLOAT", offset);

    case OP_ADD_STRING:
      return simpleInstruction("OP_ADD_STRING", offset);

    case OP_ADD_INT:
      return simpleInstruction("OP_ADD_INT", offset);

    case OP_SUBTRACT_INT:
      return simpleInstruction("OP_SUBTRACT_INT", offset);

    case OP_MULTIPLY_INT:
      return simpleInstruction("OP_MULTIPLY_INT", offset);

    case OP_DIVIDE_INT:
      return simpleInstruction("OP_DIVIDE_INT", offset);

    case OP_ADD_FLOAT:
      return simpleInstruction("OP_ADD_FLOAT", offset);

    case OP_SUBTRACT_FLOAT:
      return simpleInstruction("OP_SUBTRACT_FLOAT", offset);

    case OP_MULTIPLY_FLOAT:
      return simpleInstruction("OP_MULTIPLY_FLOAT", offset);

    case OP_DIVIDE_FLOAT:
      return simpleInstruction("OP_DIVIDE_FLOAT", offset);

    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);

    case OP_NEGATE_INT:
      return simpleInstruction("OP_NEGATE_INT", offset);

    case OP_NEGATE_FLOAT:
      return simpleInstruction("OP_NEGATE_FLOAT", offset);

    case OP_BITWISE_OR:
      return simpleInstruction("OP_BITWISE_OR", offset);

    case OP_BITWISE_AND:
      return simpleInstruction("OP_BITWISE_AND", offset);

    case OP_BITWISE_XOR:
      return simpleInstruction("OP_BITWISE_XOR", offset);

    case OP_LOGICAL_OR:
      return simpleInstruction("OP_LOGICAL_OR", offset);

    case OP_LOGICAL_AND:
      return simpleInstruction("OP_LOGICAL_AND", offset);

    case OP_SHIFT_LEFT:
      return simpleInstruction("OP_SHIFT_LEFT", offset);

    case OP_SHIFT_RIGHT:
      return simpleInstruction("OP_SHIFT_RIGHT", offset);

    case OP_SHIFT_URIGHT:
      return simpleInstruction("OP_SHIFT_URIGHT", offset);

    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);

    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);

    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);

    case OP_POP_JUMP_IF_FALSE:
      return jumpInstruction("OP_POP_JUMP_IF_FALSE", 1, chunk, offset);

    case OP_NEW:
      return byteInstruction("OP_NEW", chunk, offset);

    case OP_CALL:
      return byteInstruction("OP_CALL", chunk, offset);

    case OP_ARRAY_INDEX:
      return byteInstruction("OP_ARRAY_INDEX", chunk, offset);

    case OP_CLOSURE: {
      offset++;

      uint8_t constant = chunk->code[offset++];

      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printObject(chunk->constants.values[constant]);
      printf("\n");

      ObjCallable *function = AS_CALLABLE(chunk->constants.values[constant]);

      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];

        printf("%04d      |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
      }

      printf("\n");
      return offset;
    }

    case OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", offset);

    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);

    case OP_HALT:
      return simpleInstruction("OP_HALT", offset);

    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}