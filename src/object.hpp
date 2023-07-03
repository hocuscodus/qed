/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_object_h
#define qed_object_h

#include <sstream>
#include "common.h"
#include "chunk.hpp"
#include "value.h"
#include "scanner.hpp"

#define GET_OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define AS_THREAD(value)       ((CoThread*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_OBJECT(value)       ((ObjObject*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_CALLABLE(value)     ((ObjCallable*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)
#define AS_NATIVE_CLASS(value) (((ObjNativeClass*)AS_OBJ(value))->classFn)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
#define AS_ARRAY(value)        ((ObjArray*)AS_OBJ(value))

typedef struct ObjString ObjString;

typedef enum {
  OBJ_STRING,
  OBJ_INSTANCE,
  OBJ_FUNCTION,
  OBJ_ARRAY,
/////////////////////////////
  OBJ_THREAD,
  OBJ_OBJECT,
  OBJ_CLOSURE,
  OBJ_NATIVE,
  OBJ_NATIVE_CLASS,
  OBJ_UPVALUE,
  OBJ_INTERNAL
} ObjType;

struct Obj {
  ObjType type;
  struct Obj *next;

  bool equals(Obj *obj);
  const char *toString();
};

struct ObjFunction;

struct Declaration {
  Type type;
  Token name;
  bool isField;
  ObjFunction *function;
  Declaration *previous;
  int parentFlag;

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
  Type type;
  Compiler *compiler;
};

struct FunctionExpr;
class Parser;

struct ObjFunction : ObjCallable {
  FunctionExpr *expr;
  int upvalueCount;
  Upvalue upvalues[UINT8_COUNT];
  Chunk chunk;
  Obj *native;
  IndexList *instanceIndexes;
  long eventFlags;
  ObjFunction *uiFunction;
  ObjFunction *firstChild;
  ObjFunction *lastChild;
  ObjFunction *next;

  ObjFunction();

  int addUpvalue(uint8_t index, bool isField, Declaration *declaration, Parser &parser);
  void add(ObjFunction *function);
  void print();
  bool isClass();
  bool isUserClass();
  std::string getThisVariableName();
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative {
  Obj obj;
  NativeFn function;
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

struct ObjObject {
  Obj obj;
  Value *fields;

  ObjClosure *getClosure() {return AS_CLOSURE(fields[0]);}
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
  Value *stack;
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
ObjObject *newObject(ObjClosure *closure);
ObjClosure *newClosure(ObjFunction *function, CoThread *parent);
ObjNative *newNative(NativeFn function);
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