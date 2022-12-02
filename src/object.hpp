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
#ifndef qed_object_h
#define qed_object_h

#include <string>
#include <unordered_map>
#include "common.h"
#include "chunk.hpp"
#include "value.h"
#include "sizer.hpp"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_COMPILER_INSTANCE(value)     isObjType(value, OBJ_COMPILER_INSTANCE)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_COMPILER_INSTANCE(value)     ((ObjCompilerInstance*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_CALLABLE(value)     ((ObjCallable*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)
#define AS_NATIVE_CLASS(value) (((ObjNativeClass*)AS_OBJ(value))->classFn)
#define AS_PRIMITIVE(value)    ((ObjPrimitive*)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef struct ObjString ObjString;

typedef enum {
  OBJ_INSTANCE,
  OBJ_COMPILER_INSTANCE,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_NATIVE_CLASS,
  OBJ_PRIMITIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
  OBJ_FUNCTION_PTR,
  OBJ_INTERNAL
} ObjType;

struct Obj {
  ObjType type;
  struct Obj *next;
};

typedef struct {
  uint8_t index;
  bool isLocal;
  Type type;
} Upvalue;

class Parser;

struct ObjFunctionPtr {
  Obj obj;
  Type type;
  int arity;
  Type *parms;
};

typedef struct {
  Type type;
  ObjString *name;
} Field;

struct ObjNamed {
  Obj obj;
  ObjString *name;
};

struct ObjCallable : ObjNamed {
  Type type;
  int arity;
  int fieldCount;
  Field *fields;
  int upvalueCount;
  Upvalue upvalues[UINT8_COUNT];

  bool isClass();
  int addUpvalue(uint8_t index, bool isLocal, Type type, Parser &parser);
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

struct ChildAttrSets;

struct ObjFunction : ObjCallable {
  Chunk chunk;
  Obj *native;
  IndexList *instanceIndexes;
  ChildAttrSets *attrSets;
  DirSizers topSizers;
  ObjFunction *uiFunction;
//  std::unordered_map<std::string, ObjFunction*> *uiFunctions;
//  void parseCreateUIValues(VM &vm, Object value, int flags, List<int> dimIndexes, ValueTree valueTree);

  LayoutUnitArea *parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, Value *values, IndexList *instanceIndexes, int dimFlags);
  IntTreeUnit *parseResize(int dir, const Point &limits, LocationUnit *areas, int offset);
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative : ObjCallable {
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
  INTERPRET_CONTINUE,
  INTERPRET_HALT,
  INTERPRET_RETURN,
  INTERPRET_SWITCH_THREAD
} InterpretResult;

struct VM;
struct CoThread;

typedef struct {
  InterpretResult result;
  union {
    CoThread *coThread;
    Value returnValue;
  } as;
} InterpretValue;

typedef InterpretValue (*NativeClassFn)(VM &vm, int argCount, Value *args);

struct ObjNativeClass : ObjCallable {
  NativeClassFn classFn;
  void *arg;
};

struct ObjClosure {
  Obj obj;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  int upvalueCount;
};

struct CallFrame {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
  int handlerIp;
  ObjClosure *uiClosure;

	LocationUnit *init(VM &vm, Value *values, IndexList *instanceIndexes, ValueStack<Value *> &valueStack);/*
	void refresh(VM &process, const Point &pos, const Point &size, const Point &clipPos, const Point &clipSize);
	Path locateEvent(VM &process, const Point &pos, const Point &size, const Point &location, Path currentFocusPath);
	const Point &getTotalSize();*/
};

struct ObjInstance;

struct CoThread {
  CoThread *caller;
  Value *fields;
  int frameCount;
  CallFrame frames[FRAMES_MAX];
  ObjUpvalue *openUpvalues;
  ObjInstance *instance;
  Value *stackTop;

  CoThread(ObjInstance *instance);
  ~CoThread();

  void push(Value value);
  Value pop();
  Value peek(int distance);
  bool call(ObjClosure *closure, int argCount, int handlerIp);
  InterpretValue callValue(Value callee, int argCount, bool newFlag, int handlerIp);
  ObjUpvalue *captureUpvalue(Value *local);
  void closeUpvalues(Value *last);

  ObjClosure *pushClosure(ObjFunction *function);
  void reset();

  void concatenate();
  void resetStack();
  void runtimeError(const char *format, ...);
  InterpretValue run();

  bool isDone();
  bool isFirstInstance();
  bool isInInstance();
  CallFrame *getFrame(int index = 0);
  void onReturn(Value &returnValue);

  bool getFormFlag();
};

struct ObjCompilerInstance {
  Obj obj;
  ObjCallable *callable;
};

struct ObjInstance {
  Obj obj;
  CoThread *coThread;
  int numValuesInstances;
  ObjInstance **uiValuesInstances;

  void initValues();
  void uninitValues();
  UnitArea *recalculate(VM &vm, ValueStack<Value *> &valueStack);
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
ObjInstance *newInstance(CoThread *caller);
ObjCompilerInstance *newCompilerInstance(ObjCallable *callable);
ObjClosure *newClosure(ObjFunction *function);
ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);
ObjPrimitive *newPrimitive(char *name, Type type);
ObjFunctionPtr *newFunctionPtr(Type type, int arity, Type *parms);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(Value *slot);
void printObject(Value value);
void freeObjects();

extern Obj *objects;

#ifdef DEBUG_TRACE_EXECUTION
static inline bool isObjType(Value &value, ObjType objType) {
  return value.type == VAL_OBJ && value.as.obj->type == objType;
}
#else
static inline bool isObjType(Value &value, ObjType objType) {
  return value.obj->type == objType;
}
#endif

#endif