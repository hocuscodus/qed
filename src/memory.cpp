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
