/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdlib.h>
#include "memory.h"
#include "object.hpp"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}

static void freeObject(Obj *object) {
  switch (object->type) {
    case OBJ_FUNCTION: {
      ObjFunction *function = (ObjFunction *)object;
      delete function->instanceIndexes;
      FREE(ObjFunction, object);
      break;
    }

    case OBJ_INSTANCE:
      FREE(ObjInstance, object);
      break;

    case OBJ_STRING: {
      ObjString *string = (ObjString *)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }

    case OBJ_ARRAY:
      FREE(ObjArray, object);
      break;
  }
}

void freeObjects() {
  Obj *object = objects;

  while (object != NULL) {
    Obj *next = object->next;
    freeObject(object);
    object = next;
  }
}
