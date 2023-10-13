/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_memory_h
#define qed_memory_h

#include "common.h"

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : (capacity) * 2)

#define RESIZE_ARRAY(type, pointer, oldCount, newCount) \
  (type *) reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif