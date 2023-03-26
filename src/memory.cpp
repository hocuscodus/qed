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
    case OBJ_INTERNAL: {
      ObjInternal *internal = (ObjInternal *) object;

      if (internal->object)
        delete internal->object;

      FREE(ObjInstance, object);
      break;
    }

    case OBJ_THREAD: {
      CoThread *coThread = (CoThread *) object;

      if (coThread->getFormFlag())
        for (int ndx = 0; ndx < coThread->frameCount; ndx++)
          FREE(OBJ_THREAD, coThread->frames[ndx].uiValuesInstance);

//      delete[] coThread->fields;
      FREE_ARRAY(Value, coThread->fields, 64);
      FREE(CoThread, object);
      break;
    }

    case OBJ_RETURN:
    case OBJ_CLOSURE: {
      ObjClosure *closure = (ObjClosure *) object;
//      delete closure->uiClosure;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }

    case OBJ_FUNCTION: {
      ObjFunction *function = (ObjFunction *)object;
//      delete function->uiFunction;
      function->chunk.uninit();
      delete function->instanceIndexes;
      FREE(ObjFunction, object);
      break;
    }

    case OBJ_INSTANCE:
      FREE(ObjInstance, object);
      break;

    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;

    case OBJ_PRIMITIVE:
      FREE(ObjPrimitive, object);
      break;

    case OBJ_STRING: {
      ObjString *string = (ObjString *)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }

    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;

    case OBJ_ARRAY:
      FREE(ObjArray, object);
      break;

    case OBJ_FUNCTION_PTR:
      ObjFunctionPtr *functionPtr = (ObjFunctionPtr *)object;

      if (functionPtr->parms)
        delete[] functionPtr->parms;

      FREE(ObjFunctionPtr, object);
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
