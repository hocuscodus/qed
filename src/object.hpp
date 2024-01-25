/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_object_h
#define qed_object_h

#include <sstream>
#include "scanner.hpp"

#include <array>
#include "common.h"

#define UNKNOWN_TYPE        (Type) {VAL_UNKNOWN}
#define VOID_TYPE           (Type) {VAL_VOID}
#define BOOL_TYPE           (Type) {VAL_BOOL}
#define INT_TYPE            (Type) {VAL_INT}
#define FLOAT_TYPE          (Type) {VAL_FLOAT}
#define OBJ_TYPE(objType)   (Type) {VAL_OBJ, &(objType)->obj}

#define IS_UNKNOWN(type)  ((type).valueType == VAL_UNKNOWN)
#define IS_VOID(type)     ((type).valueType == VAL_VOID)
#define IS_BOOL(type)     ((type).valueType == VAL_BOOL)
#define IS_INT(type)      ((type).valueType == VAL_INT)
#define IS_FLOAT(type)    ((type).valueType == VAL_FLOAT)
#define IS_OBJ(type)      ((type).valueType == VAL_OBJ)

#define AS_OBJ_TYPE(type1)    (IS_OBJ(type1) && (type1).objType ? (type1).objType->type : (ObjType) -1)

#define IS_ANY(type)          isObjType(type, OBJ_ANY)
#define IS_INSTANCE(type)     isObjType(type, OBJ_INSTANCE)
#define IS_FUNCTION(type)     isObjType(type, OBJ_FUNCTION)
#define IS_STRING(type)       isObjType(type, OBJ_STRING)
#define IS_ARRAY(type)        isObjType(type, OBJ_ARRAY)

#define AS_INSTANCE_TYPE(type)  (IS_INSTANCE(type) ? (ObjInstance *) (type).objType : NULL)
#define AS_FUNCTION_TYPE(type)  (IS_FUNCTION(type) ? (ObjFunction *) (type).objType : NULL)
#define AS_ARRAY_TYPE(type)     (IS_ARRAY(type) ? (ObjArray *) (type).objType : NULL)
#define AS_STRING_TYPE(type)    (IS_STRING(type) ? (ObjString *) (type).objType : NULL)

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
  VAL_OBJ
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

#define GET_OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_CALLABLE(value)     ((ObjCallable*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_ARRAY(value)        ((ObjArray*)AS_OBJ(value))
#define AS_CSTRING(value)      (AS_STRING(value)->str)

typedef struct ObjString ObjString;

typedef enum {
  OBJ_ANY,
  OBJ_STRING,
  OBJ_INSTANCE,
  OBJ_FUNCTION,
  OBJ_ARRAY,
} ObjType;

struct Obj {
  ObjType type;

  bool equals(Obj *obj);
  const char *toString();
};

struct Expr;
struct FunctionExpr;

struct Declaration {
  Type type;
  Token name;
  Expr *expr;
  FunctionExpr *function;
  bool isInternalField;
  Declaration *peer;
  Declaration *next;
  bool parentFlag;

  Declaration();

  std::string getRealName();
};

typedef struct {
  uint8_t index;
  bool isField;
  Declaration *declaration;
} Upvalue;

struct IndexList {
  int size;
  long *array;

  IndexList();
  IndexList(long indexes);
  ~IndexList();

  void set(int index);
  int getNext(int oldIndex);
  int get(int index);
};

struct Compiler;

struct ObjCallable {
  Obj obj;
};

struct FunctionExpr;
class Parser;

struct ObjFunction : ObjCallable {
  FunctionExpr *expr;
  IndexList *instanceIndexes;
  long eventFlags;
  ObjFunction *uiFunction;
  bool hasInternalFields;
  ObjFunction();

  std::string getThisVariableName();
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative {
  Obj obj;
  NativeFn function;
};

struct ObjString {
  Obj obj;
  char *str;
};

typedef struct ObjUpvalue {
  Obj obj;
  Value *location;
  Value closed;
  struct ObjUpvalue *next;
} ObjUpvalue;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_SUSPEND
} InterpretResult;

typedef struct {
  Obj obj;
  Type elementType;
} ObjArray;

struct ObjInstance {
  Obj obj;
  ObjCallable *callable;
};

#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)allocateObject(sizeof(type), objectType)

Obj *allocateObject(size_t size, ObjType type);
ObjInstance *newInstance(ObjCallable *callable);
ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(Value *slot);
ObjArray *newArray(Type elementType);
void printObject(Value value);
void freeObjects();

extern Obj *objects;

static inline bool isObjType(Type &type, ObjType objType) {
  return AS_OBJ_TYPE(type) == objType;
}

#endif