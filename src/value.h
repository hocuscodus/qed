/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_value_h
#define qed_value_h

#include <array>
#include "common.h"

#define IS_UNKNOWN(type)  ((type).valueType == VAL_UNKNOWN)
#define IS_VOID(type)     ((type).valueType == VAL_VOID)
#define IS_BOOL(type)     ((type).valueType == VAL_BOOL)
#define IS_INT(type)      ((type).valueType == VAL_INT)
#define IS_FLOAT(type)    ((type).valueType == VAL_FLOAT)
#define IS_OBJ(type)      ((type).valueType == VAL_OBJ)

#define AS_OBJ_TYPE(type1)    (IS_OBJ(type1) && (type1).objType ? (type1).objType->type : (ObjType) -1)

#define IS_INSTANCE(type)     isObjType(type, OBJ_INSTANCE)
#define IS_FUNCTION(type)     isObjType(type, OBJ_FUNCTION)
#define IS_PRIMITIVE(type)    isObjType(type, OBJ_PRIMITIVE)
#define IS_STRING(type)       isObjType(type, OBJ_STRING)
#define IS_ARRAY(type)        isObjType(type, OBJ_ARRAY)

#define AS_INSTANCE_TYPE(type)  (IS_INSTANCE(type) ? (ObjInstance *) (type).objType : NULL)
#define AS_FUNCTION_TYPE(type)  (IS_FUNCTION(type) ? (ObjFunction *) (type).objType : NULL)
#define AS_ARRAY_TYPE(type)     (IS_ARRAY(type) ? (ObjArray *) (type).objType : NULL)
#define AS_STRING_TYPE(type)    (IS_STRING(type) ? (ObjString *) (type).objType : NULL)
#define AS_PRIMITIVE_TYPE(type) (IS_PRIMITIVE(type) ? (ObjPrimitive *) (type).objType : NULL)

#define NUM_DIRS 2

template <typename type> using DirType = std::array<type, NUM_DIRS>;

typedef DirType<int> Point;

typedef struct Obj Obj;

typedef enum {
  VAL_UNKNOWN,
  VAL_VOID,
  VAL_BOOL,
  VAL_INT,
  VAL_FLOAT,
  VAL_OBJ,
  VAL_POINT,
} ValueType;

struct Type {
  ValueType valueType;
  Obj *objType;

  bool equals(Type &type);
  const char *toString();
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
void printValue(Value &value);

#endif