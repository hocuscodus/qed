/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_object_h
#define qed_object_h

#include "common.h"
#include "chunk.hpp"
#include "value.h"
#include "scanner.hpp"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define AS_THREAD(value)       ((CoThread*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_CALLABLE(value)     ((ObjCallable*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)
#define AS_NATIVE_CLASS(value) (((ObjNativeClass*)AS_OBJ(value))->classFn)
#define AS_PRIMITIVE(value)    ((ObjPrimitive*)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define AS_ARRAY(value)        ((ObjArray*)AS_OBJ(value))

typedef struct ObjString ObjString;

typedef enum {
  OBJ_THREAD,
  OBJ_INSTANCE,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_NATIVE_CLASS,
  OBJ_PRIMITIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
  OBJ_ARRAY,
  OBJ_FUNCTION_PTR,
  OBJ_INTERNAL
} ObjType;

struct Obj {
  ObjType type;
  struct Obj *next;

  const char *toString();
};

typedef struct {
  uint8_t index;
  bool isField;
  Type type;
} Upvalue;

class Parser;

struct ObjFunctionPtr {
  Obj obj;
  Type type;
  int arity;
  Type *parms;
};

struct ObjNamed {
  Obj obj;
  ObjString *name;
};

typedef struct {
  Type type;
  Token name;
  bool isField;
  int realIndex;
} Declaration;

struct ObjCallable : ObjNamed {
  Type type;
  int arity;
  int *declarationCount;
  Declaration *declarations;

  bool isClass();
};

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

struct Expr;
struct DeclarationExpr;

struct ObjFunction : ObjCallable {
  int upvalueCount;
  Upvalue upvalues[UINT8_COUNT];
  Expr *bodyExpr;
  Chunk chunk;
  Obj *native;
  IndexList *instanceIndexes;
  long eventFlags;
  ObjFunction *uiFunction;

  int addUpvalue(uint8_t index, bool isField, Type type, Parser &parser);
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative {
  Obj obj;
  NativeFn function;
};

struct ObjPrimitive : ObjNamed {
  Type type;
};

struct ObjString {
  Obj obj;
  int length;
  char *chars;
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

struct VM;
struct CoThread;

typedef InterpretResult (*NativeClassFn)(VM &vm, int argCount, Value *args);

struct ObjNativeClass {
  Obj obj;
  NativeClassFn classFn;
  void *arg;
};

struct ObjClosure {
  Obj obj;
  CoThread *parent;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  int upvalueCount;
};

struct CallFrame {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
  ObjClosure *uiClosure;
  CoThread *uiValuesInstance;
  CoThread *uiLayoutInstance;
};

struct CoThread {
  Obj obj;
  CoThread *caller;
  Value *fields;
  int frameCount;
  CallFrame frames[FRAMES_MAX];
  ObjUpvalue *openUpvalues;
  Value *savedStackTop;

  bool call(ObjClosure *closure, int argCount);
  bool callValue(Value callee, int argCount);
  ObjUpvalue *captureUpvalue(Value *field);
  void closeUpvalues(Value *last);

  ObjClosure *pushClosure(ObjFunction *function);
  void reset();

  void resetStack();
  void runtimeError(const char *format, ...);
#ifdef DEBUG_TRACE_EXECUTION
  void printStack();
#endif
  bool isDone();
  bool isInInstance();
  CallFrame *getFrame(int index = 0);
  void onReturn(Value &returnValue);

  bool getFormFlag();

  void initValues();
  void uninitValues();
  Point recalculateLayout();
  Point repaint();
  void paint(Point pos, Point size);
  bool onEvent(Event event, Point pos, Point size);
  bool runHandler(ObjClosure *closure);
};

typedef struct {
  Obj obj;
  Type elementType;
} ObjArray;

struct ObjInstance {
  Obj obj;
  ObjCallable *callable;
};

struct Internal {
  virtual ~Internal();
};

struct ObjInternal {
  Obj obj;
  Internal *object;
};

#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)allocateObject(sizeof(type), objectType)

Obj *allocateObject(size_t size, ObjType type);
ObjInternal *newInternal();
CoThread *newThread(CoThread *caller);
ObjInstance *newInstance(ObjCallable *callable);
ObjClosure *newClosure(ObjFunction *function, CoThread *parent);
ObjFunction *newFunction(Type type, ObjString *name, int arity);
ObjNative *newNative(NativeFn function);
ObjPrimitive *newPrimitive(char *name, Type type);
ObjFunctionPtr *newFunctionPtr(Type type, int arity, Type *parms);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(Value *slot);
ObjArray *newArray();
void printObject(Value value);
void freeObjects();

extern Obj *objects;

static inline bool isObjType(Type &type, ObjType objType) {
  return AS_OBJ_TYPE(type) == objType;
}

#endif