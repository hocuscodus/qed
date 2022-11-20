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
