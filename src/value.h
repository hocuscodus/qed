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
#ifndef qed_value_h
#define qed_value_h

#include "common.h"

typedef struct Obj Obj;

typedef enum {
  VAL_VOID,
  VAL_BOOL,
  VAL_INT,
  VAL_FLOAT,
  VAL_OBJ,
} ValueType;

struct Type {
  ValueType valueType;
  Obj *objType;

  bool equals(Type &type);
};

typedef union {
  bool boolean;
  long integer;
  double floating;
  Obj *obj;
} As;

#ifdef DEBUG_TRACE_EXECUTION
typedef struct {
  ValueType type;
  As as;
} Value;

//#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value)  ((value).as.boolean)
#define AS_INT(value)   ((value).as.integer)
#define AS_FLOAT(value) ((value).as.floating)
#define AS_OBJ(value)   ((value).as.obj)

#define VOID_VAL         ((Value){VAL_VOID, {.obj = NULL}})
#define BOOL_VAL(value)  ((Value){VAL_BOOL, {.boolean = value}})
#define INT_VAL(value)   ((Value){VAL_INT, {.integer = value}})
#define FLOAT_VAL(value) ((Value){VAL_FLOAT, {.floating = value}})
#define OBJ_VAL(object)  ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#define VALUE(type, value) ((Value){type, value})
#else
typedef As Value;

#define AS_BOOL(value)  ((value).boolean)
#define AS_INT(value)   ((value).integer)
#define AS_FLOAT(value) ((value).floating)
#define AS_OBJ(value)   ((value).obj)

#define VOID_VAL         ((Value){.obj = NULL})
#define BOOL_VAL(value)  ((Value){.boolean = value})
#define INT_VAL(value)   ((Value){.integer = value})
#define FLOAT_VAL(value) ((Value){.floating = value})
#define OBJ_VAL(object)  ((Value){.obj = (Obj*)object})
#define VALUE(type, value) (value)
#endif

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

int valuesCompare(Value a, Value b);
void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
#ifdef DEBUG_TRACE_EXECUTION
void printValue(Value &value);
#endif

#endif