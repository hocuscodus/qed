/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_chunk_h
#define qed_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL_DIR,
  OP_ADD_LOCAL,
  OP_MAX_LOCAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_INT_TO_FLOAT,
  OP_FLOAT_TO_INT,
  OP_INT_TO_STRING,
  OP_FLOAT_TO_STRING,
  OP_BOOL_TO_STRING,
  OP_EQUAL_STRING,
  OP_GREATER_STRING,
  OP_ADD_STRING,
  OP_LESS_STRING,
  OP_EQUAL_INT,
  OP_GREATER_INT,
  OP_LESS_INT,
  OP_ADD_INT,
  OP_SUBTRACT_INT,
  OP_MULTIPLY_INT,
  OP_DIVIDE_INT,
  OP_EQUAL_FLOAT,
  OP_GREATER_FLOAT,
  OP_LESS_FLOAT,
  OP_ADD_FLOAT,
  OP_SUBTRACT_FLOAT,
  OP_MULTIPLY_FLOAT,
  OP_DIVIDE_FLOAT,
  OP_NOT,
  OP_NEGATE_FLOAT,
  OP_NEGATE_INT,
  OP_BITWISE_OR,
  OP_BITWISE_AND,
  OP_BITWISE_XOR,
  OP_LOGICAL_OR,
  OP_LOGICAL_AND,
  OP_SHIFT_LEFT,
  OP_SHIFT_RIGHT,
  OP_SHIFT_URIGHT,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_POP_JUMP_IF_FALSE,
  OP_NEW,
  OP_CALL,
  OP_ARRAY_INDEX,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
  OP_HALT
} OpCode;

struct Chunk {
  int count;
  int capacity;
  uint8_t *code;
  int *lines;
  ValueArray constants;

  void init();
  void uninit();
  void reset();

  void writeChunk(uint8_t byte, int line);
  int addConstant(Value value);
};

#endif