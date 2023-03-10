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
    case VAL_VOID: return "void";
    case VAL_BOOL: return "bool";
    case VAL_INT: return "int";
    case VAL_FLOAT: return "float";
    case VAL_VAR: return "!var!";
    case VAL_HANDLER: return "!handler!";
    case VAL_POINT: return "!point!";
    case VAL_OBJ: return objType->toString();
    default: return "!unknown!";
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
