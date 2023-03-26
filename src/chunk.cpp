/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdlib.h>
#include "chunk.hpp"
#include "memory.h"

void Chunk::init() {
  count = 0;
  capacity = 0;
  code = NULL;
  lines = NULL;
  initValueArray(&constants);
}

void Chunk::uninit() {
  FREE_ARRAY(uint8_t, code, capacity);
  FREE_ARRAY(int, lines, capacity);
  freeValueArray(&constants);
}

void Chunk::reset() {
  uninit();
  init();
}

void Chunk::writeChunk(uint8_t byte, int line) {
  int oldCapacity = capacity;

  if (count >= oldCapacity) {
    capacity = GROW_CAPACITY(oldCapacity);
    code = RESIZE_ARRAY(uint8_t, code, oldCapacity, capacity);
    lines = RESIZE_ARRAY(int, lines, oldCapacity, capacity);
  }

  code[count] = byte;
  lines[count++] = line;
}

int Chunk::addConstant(Value value) {
  int index = constants.count;

  writeValueArray(&constants, value);
  return index;
}
