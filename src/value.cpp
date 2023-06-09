/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "object.hpp"
#include "value.h"
#include "memory.h"

bool Type::equals(Type &type) {
  return valueType == type.valueType && (valueType != VAL_OBJ || objType == type.objType);
}

const char *Type::toString() {
  switch (valueType) {
    case VAL_UNKNOWN: return "!unknown!";
    case VAL_VOID: return "void";
    case VAL_BOOL: return "bool";
    case VAL_INT: return "int";
    case VAL_FLOAT: return "float";
    case VAL_POINT: return "!point!";
    case VAL_OBJ: return objType ? objType->toString() : "!weird!";
    default: return "!weird2!";
  }
}

int valuesCompare(Value a, Value b) {/*
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_INT:    return AS_INT(a) == AS_INT(b);
    case VAL_FLOAT:  return AS_FLOAT(a) == AS_FLOAT(b);
    case VAL_OBJ: {*/
      ObjString *aString = AS_STRING(a);
      ObjString *bString = AS_STRING(b);
      return //aString->length == bString->length &&
          memcmp(aString->chars, bString->chars, aString->length);
/*    }
    default:         return false; // Unreachable.
  }*/
}

void initValueArray(ValueArray *array) {
  array->count = 0;
  array->capacity = 0;
  array->values = NULL;
}

void writeValueArray(ValueArray *array, Value value) {
  int oldCapacity = array->capacity;

  if (array->count >= oldCapacity) {
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = RESIZE_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count++] = value;
}

void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

void printValue(Value &value) {
#ifdef DEBUG_TRACE_EXECUTION
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;

    case VAL_INT:
      printf("%ld", AS_INT(value));
      break;

    case VAL_FLOAT:
      printf("%g", AS_FLOAT(value));
      break;

    case VAL_VOID:
      printf("NULL");
      break;

    case VAL_OBJ:
      printObject(value);
      break;
  }
#endif
}
