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
#include <cstdarg>

#include "memory.h"
#include "parser.hpp"
#include "attrset.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#include "astprinter.hpp"
#endif

Obj *objects = NULL;

Internal::~Internal() {
}

Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);

  object->type = type;
  object->next = objects;
  objects = object;
  return object;
}

bool ObjCallable::isClass() {
  return name == NULL || (name->chars[0] >= 'A' && name->chars[0] <= 'Z');
}

int ObjCallable::addUpvalue(uint8_t index, bool isLocal, Type type, Parser &parser) {
  for (int i = 0; i < upvalueCount; i++) {
    Upvalue *upvalue = &upvalues[i];

    if (upvalue->index == index && upvalue->isLocal == isLocal)
      return i;
  }

  if (upvalueCount == UINT8_COUNT) {
    parser.error("Too many closure variables in function.");
    return 0;
  }

  upvalues[upvalueCount].isLocal = isLocal;
  upvalues[upvalueCount].index = index;
  upvalues[upvalueCount].type = type;

  return upvalueCount++;
}

IndexList::IndexList() {
  size = -1;
  array = NULL;
}

IndexList::IndexList(long indexes) {
  size = 0;
  array = (long *) malloc(sizeof(long));
  *array = indexes;
}

IndexList::~IndexList() {
  if (array != NULL)
    free(array);
}

extern int ctz(long *n);

void IndexList::set(int index) {
  int indexSize = index >> 6;

  if (indexSize > size) {
    array = (long *) realloc(array, sizeof(long) * (indexSize + 1));
    memset(&array[size + 1], 0, sizeof(long) * (indexSize - size));
    size = indexSize;
  }

  array[indexSize] |= 1L << (index & 0x3F);
}

int IndexList::getNext(int oldIndex) {
  int arrayIndex = oldIndex != -1 ? oldIndex >> 6 : 0;
  long mask = oldIndex != -1 ? (1L << ((oldIndex & 0x3F) + 1)) - 1 : 0;
  int index = -1;

  if (array)
    while (arrayIndex <= size) {
      long num = array[arrayIndex] & ~mask;

      index = ctz(&num);

      if (index != -1) {
        index += arrayIndex << 6;
        break;
      }

      arrayIndex++;
      mask = -1L;
    }

  return index;
}

int IndexList::get(int index) {
  int indexSize = index >> 6;

  return indexSize <= size ? (array[indexSize] >> (index & 0x3F)) & 1 : 0;
}
/*
void ObjFunction::parseCreateUIValues(VM &vm, Object value, int flags, List<int> dimIndexes, ValueTree &valueTree) {
  if (attrSets != NULL)
    attrSets->parseCreateUIValues(vm, value, null, flags, dimIndexes, valueTree);
}
*/
LayoutUnitArea *ObjFunction::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, Value *values, IndexList *instanceIndexes, int dimFlags) {
  LocationUnit **subAreas = new LocationUnit *[attrSets->numAreas];

  memset(subAreas, 0, attrSets->numAreas * sizeof(LocationUnit *));
  attrSets->parseCreateAreasTree(vm, valueStack, 0, Path(), values, instanceIndexes, subAreas);

  DirType<IntTreeUnit *> sizeTrees;

  for (int dir = 0; dir < NUM_DIRS; dir++)
    sizeTrees[dir] = parseResize(dir, {0, 0}, *subAreas, 0);

  return new LayoutUnitArea(sizeTrees, subAreas);
}

IntTreeUnit *ObjFunction::parseResize(int dir, const Point &limits, LocationUnit *areas, int offset) {
  return topSizers[dir]->parseResize(&areas, Path(limits.size()), dir, limits);
}

LocationUnit *CallFrame::init(VM &vm, Value *values, IndexList *instanceIndexes, ValueStack<Value *> &valueStack) {
  if (closure->function->native->type == VAL_OBJ) {
//    obj.valueTree = new ValueTree();
//    closure->function->parseCreateUIValues(vm, obj, 0, [], obj.valueTree);
/*
    Point size;

    resize(size);

    for (int index = 0; index < NUM_DIRS; index++)
      totalSize[index] = std::max(size[index], totalSize[index]);
*/
  }

  if (closure->function->attrSets != NULL)
    return closure->function->parseCreateAreasTree(vm, valueStack, values, instanceIndexes, 0);
  else
    return NULL;
}

bool eventFlag;
ObjInstance *instance;

CoThread::CoThread(ObjInstance *instance) {
  this->instance = instance;
  fields = new Value[64];//ALLOCATE(Value, 64);
  resetStack();
  frameCount = 0;
  openUpvalues = NULL;
}

CoThread::~CoThread() {
  delete[] fields;
//      FREE_ARRAY(Value, instance->fields, 64);
}

Value *stackTop;

void CoThread::push(Value value) {
  *stackTop++ = value;
}

Value CoThread::pop() {
  return *--stackTop;
}

Value CoThread::peek(int distance) {
  return stackTop[-1 - distance];
}

bool CoThread::call(ObjClosure *closure, int argCount) {
  if (frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame *frame = &frames[frameCount++];
  ObjFunction *outFunction = closure->function->uiFunction;

  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = savedStackTop - argCount - 1;
  frame->uiClosure = outFunction ? newClosure(outFunction, instance) : NULL;

  if (outFunction)
    for (int i = 0; i < outFunction->upvalueCount; i++) {
      uint8_t isLocal = outFunction->upvalues[i].isLocal;
      uint8_t index = outFunction->upvalues[i].index;

      frame->uiClosure->upvalues[i] = isLocal ? captureUpvalue(frame->slots + index) : frame->closure->upvalues[index];
    }

  return true;
}

extern void postMessage(void *param);

bool CoThread::callValue(Value callee, int argCount) {
  switch (OBJ_TYPE(callee)) {
//    case OBJ_NATIVE_CLASS:
    case OBJ_RETURN: {
      stackTop -= argCount + 1;

      if (argCount)
        push(stackTop[1]);

      ObjClosure *closure = AS_CLOSURE(callee);
      postMessage(AS_CLOSURE(callee)->parent);
      break;
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), argCount);

    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(argCount, stackTop - argCount);

      stackTop -= argCount + 1;

      if (AS_CLOSURE(callee)->function->type.valueType != VAL_VOID)
        push(result);

      break;
    }
    default:
      break; // Non-callable object type.
  }
  return true;
}

ObjUpvalue *CoThread::captureUpvalue(Value *local) {
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = openUpvalues;

  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local)
    return upvalue;

  ObjUpvalue *createdUpvalue = newUpvalue(local);

  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

void CoThread::closeUpvalues(Value *last) {
  while (openUpvalues != NULL && openUpvalues->location >= last) {
    ObjUpvalue *upvalue = openUpvalues;

    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    openUpvalues = upvalue->next;
  }
}

static bool isFalsey(Value value) {
  return (/*IS_BOOL(value) && */!AS_BOOL(value));
}

void CoThread::concatenate() {
  ObjString *b = AS_STRING(pop());
  ObjString *a = AS_STRING(pop());

  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  push(OBJ_VAL(result));
}

void CoThread::resetStack() {
  savedStackTop = fields;
  frameCount = 0;
}

void CoThread::runtimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = frameCount - 1; i >= 0; i--) {
    CallFrame *frame = &frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;

    fprintf(stderr, "[line %d] in %s\n",
            function->chunk.lines[instruction],
            function->name == NULL ? "script" : function->name->chars);
  }

  resetStack();
}

ObjNativeClass *newNativeClass(NativeClassFn classFn) {
  ObjNativeClass *native = ALLOCATE_OBJ(ObjNativeClass, OBJ_NATIVE_CLASS);

  native->arity = 0;
  native->upvalueCount = 0;
  native->name = NULL;
  native->classFn = classFn;
  return native;
}

void CoThread::reset() {
//  CallFrame *frame = &frames[0];
//
//  FREE(ObjClosure, frame->closure);
  frameCount = 0;
}

ObjClosure *CoThread::pushClosure(ObjFunction *function) {
  stackTop = savedStackTop;
//  if (mainClosure == NULL) {
    push(OBJ_VAL(function));
//  }
  ObjClosure *closure = newClosure(function, NULL);
//  if (mainClosure == NULL) {
    pop();
    push(OBJ_VAL(closure));
//    mainClosure = closure;
//  }
  savedStackTop = stackTop;

  return closure;
}

#ifdef DEBUG_TRACE_EXECUTION
void CoThread::printStack() {
  CallFrame *frame = &frames[frameCount - 1];

  printf("          ");
  for (Value *slot = fields; slot < stackTop; slot++) {
    printf("[ ");
    printValue(*slot);
    printf(" ]");
  }
  printf("\n");
}
#endif

extern VM *vm;

InterpretResult CoThread::run() {
  CoThread *current = ::instance->coThread;
  CallFrame *frame = &current->frames[current->frameCount - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define BINARY_OP(valueConst, convertMacro, primitiveType, op)                 \
  do {                                                                         \
    primitiveType b = convertMacro(pop());                                     \
    primitiveType a = convertMacro(pop());                                     \
    push(valueConst(a op b));                                                  \
  } while (false)
#define STRING_OP(op)                                                          \
  do {                                                                         \
    Value b = pop();                                                           \
    Value a = pop();                                                           \
    push(BOOL_VAL(valuesCompare(a, b) op 0));                                  \
  } while (false)

  stackTop = current->savedStackTop;

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    current->printStack();
    disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
#endif
    uint8_t instruction = READ_BYTE();

    switch (instruction) {
    case OP_CONSTANT: {
//      Value constant = READ_CONSTANT();
      uint8_t byte = READ_BYTE();
      Value value = frame->closure->function->chunk.constants.values[byte];

      push(value);//frame->closure->function->type.valueType == VAL_OBJ && AS_OBJ(value)->type == OBJ_INTERNAL ? OBJ_VAL(newInternal()) : value);
      break;
    }
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_POP:
      pop();
      break;
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }
    case OP_GET_PROPERTY: {
      ObjInstance *instance = AS_INSTANCE(pop());
      push(instance->coThread->fields[READ_BYTE()]);
      break;
    }
    case OP_SET_PROPERTY: {
      Value value = pop();
      ObjInstance *instance = AS_INSTANCE(pop());
      instance->coThread->fields[READ_BYTE()] = value;
      push(value);
      break;
    }
    case OP_GET_LOCAL_DIR: {
      int8_t dir = READ_BYTE();
      int8_t slot = READ_BYTE();

      push(INT_VAL(AS_INT(frame->slots[slot])));
      break;
    }
    case OP_ADD_LOCAL: {
      int8_t a = READ_BYTE();
      int8_t b = READ_BYTE();

      push(INT_VAL(AS_INT(frame->slots[a]) + AS_INT(frame->slots[b])));
      break;
    }
    case OP_MAX_LOCAL: {
      int8_t a = READ_BYTE();
      int8_t b = READ_BYTE();

      push(INT_VAL(std::max(AS_INT(frame->slots[a]), AS_INT(frame->slots[b]))));
      break;
    }
    case OP_GET_LOCAL: {
      int8_t slot = READ_BYTE();

      push(frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      int8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }
    case OP_INT_TO_FLOAT:
      push(FLOAT_VAL((double) AS_INT(pop())));
      break;
    case OP_FLOAT_TO_INT:
      push(INT_VAL((long) AS_FLOAT(pop())));
      break;
    case OP_INT_TO_STRING: {
      char buffer [sizeof(long)*8+1];

      sprintf(buffer, "%ld", AS_INT(pop()));
      push(OBJ_VAL(copyString(buffer, strlen(buffer))));
      break;
    }
    case OP_FLOAT_TO_STRING: {
      char buffer[64];

      sprintf(buffer, "%g", AS_FLOAT(pop()));
      push(OBJ_VAL(copyString(buffer, strlen(buffer))));
      break;
    }
    case OP_BOOL_TO_STRING: {
      const char *buffer = AS_BOOL(pop()) ? "true" : "false";

      push(OBJ_VAL(copyString(buffer, strlen(buffer))));
      break;
    }
    case OP_EQUAL_STRING:
      STRING_OP(==);
      break;
    case OP_GREATER_STRING:
      STRING_OP(>);
      break;
    case OP_LESS_STRING:
      STRING_OP(<);
      break;
    case OP_EQUAL_FLOAT:
      BINARY_OP(BOOL_VAL, AS_FLOAT, double, ==);
      break;
    case OP_GREATER_FLOAT:
      BINARY_OP(BOOL_VAL, AS_FLOAT, double, >);
      break;
    case OP_LESS_FLOAT:
      BINARY_OP(BOOL_VAL, AS_FLOAT, double, <);
      break;
    case OP_EQUAL_INT:
      BINARY_OP(BOOL_VAL, AS_INT, long, ==);
      break;
    case OP_GREATER_INT:
      BINARY_OP(BOOL_VAL, AS_INT, long, >);
      break;
    case OP_LESS_INT:
      BINARY_OP(BOOL_VAL, AS_INT, long, <);
      break;
    case OP_ADD_STRING:
      concatenate();
      break;
    case OP_ADD_INT:
      BINARY_OP(INT_VAL, AS_INT, long, +);
      break;
    case OP_SUBTRACT_INT:
      BINARY_OP(INT_VAL, AS_INT, long, -);
      break;
    case OP_MULTIPLY_INT:
      BINARY_OP(INT_VAL, AS_INT, long, *);
      break;
    case OP_DIVIDE_INT:
      BINARY_OP(INT_VAL, AS_INT, long, /);
      break;
    case OP_ADD_FLOAT:
      BINARY_OP(FLOAT_VAL, AS_FLOAT, double, +);
      break;
    case OP_SUBTRACT_FLOAT:
      BINARY_OP(FLOAT_VAL, AS_FLOAT, double, -);
      break;
    case OP_MULTIPLY_FLOAT:
      BINARY_OP(FLOAT_VAL, AS_FLOAT, double, *);
      break;
    case OP_DIVIDE_FLOAT:
      BINARY_OP(FLOAT_VAL, AS_FLOAT, double, /);
      break;
    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;
    case OP_NEGATE_FLOAT:
      push(FLOAT_VAL(-AS_FLOAT(pop())));
      break;
    case OP_NEGATE_INT:
      push(INT_VAL(-AS_INT(pop())));
      break;
    case OP_BITWISE_OR:
      BINARY_OP(INT_VAL, AS_INT, long, |);
      break;
    case OP_BITWISE_AND:
      BINARY_OP(INT_VAL, AS_INT, long, &);
      break;
    case OP_BITWISE_XOR:
      BINARY_OP(INT_VAL, AS_INT, long, ^);
      break;
    case OP_LOGICAL_OR:
      BINARY_OP(BOOL_VAL, AS_BOOL, bool, ||);
      break;
    case OP_LOGICAL_AND:
      BINARY_OP(BOOL_VAL, AS_BOOL, bool, &&);
      break;
    case OP_SHIFT_LEFT:
      BINARY_OP(INT_VAL, AS_INT, long, <<);
      break;
    case OP_SHIFT_RIGHT:
      BINARY_OP(INT_VAL, AS_INT, long, >>);
      break;
    case OP_SHIFT_URIGHT:
#ifdef __EMSCRIPTEN__
      BINARY_OP(INT_VAL, AS_INT, long, >>);
#else
      BINARY_OP(INT_VAL, AS_INT, unsigned long, >>);
#endif
      break;
    case OP_PRINT: {
      Value value = pop();

      printObject(value);
      printf("\n");
      break;
    }
    case OP_JUMP: {
      int16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      int16_t offset = READ_SHORT();
      if (isFalsey(peek(0))) frame->ip += offset;
      break;
    }
    case OP_POP_JUMP_IF_FALSE: {
      int16_t offset = READ_SHORT();
      if (isFalsey(pop())) frame->ip += offset;
      break;
    }
    case OP_NEW: {
      int argCount = READ_BYTE();
      Value handlerConstant = pop();
      ObjInstance *instance = newInstance(::instance);
      int size = argCount + 1;

      instance->handler = AS_INT(handlerConstant) != -1 ? AS_CLOSURE(handlerConstant) : NULL;
      memcpy(instance->coThread->fields, stackTop - size, size * sizeof(Value));
      instance->coThread->savedStackTop += size;
      stackTop -= size;
      push(OBJ_VAL(instance));

      if (instance->coThread->callValue(*instance->coThread->fields, argCount)) {
        current->savedStackTop = stackTop;
        ::instance = instance;
        current = ::instance->coThread;
        frame = &current->frames[current->frameCount - 1];
        stackTop = current->savedStackTop;
      }
      else
        return INTERPRET_RUNTIME_ERROR;
      break;
    }
    case OP_CALL: {
      int argCount = READ_BYTE();

      current->savedStackTop = stackTop;

      if (!current->callValue(peek(argCount), argCount))
        return INTERPRET_RUNTIME_ERROR;

      frame = &current->frames[current->frameCount - 1];
      break;
    }
    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = newClosure(function, ::instance);

      push(OBJ_VAL(closure));

      for (int i = 0; i < closure->upvalueCount; i++) {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();

        closure->upvalues[i] = isLocal ? current->captureUpvalue(frame->slots + index) : frame->closure->upvalues[index];
      }
      break;
    }
    case OP_CLOSE_UPVALUE:
      current->closeUpvalues(stackTop - 1);
      pop();
      break;
    case OP_RETURN: {
      if (current->isFirstInstance() && current->isInInstance()) {
        current->closeUpvalues(current->frames[0].slots);
        pop();
        return INTERPRET_OK;
      }
      else {
        ValueType type = frame->closure->function->type.valueType;
        Value result = type != VAL_VOID ? pop() : (Value) {VAL_VOID};

        if (current->isInInstance())
          ::instance->onReturn(result);
        else {
          current->onReturn(result);
          frame = &current->frames[current->frameCount - 1];
          break;
        }
      }
    }
    // Is is wanted that we do not break here...
    case OP_HALT: {
      Obj *native = frame->closure->function->native;

      if (native && native->type == OBJ_NATIVE) {
        NativeFn nativeFn = ((ObjNative *) native)->function;
        int argCount = stackTop - frame->slots - 1;
        //TODO: verify result type with frame->closure->function->type.valueType before calling onReturn
//        ValueType type = frame->closure->function->type.valueType;
        Value result = nativeFn(argCount, stackTop - argCount);

        current->onReturn(result);
        frame = &current->frames[current->frameCount - 1];
      }
      else {
        Obj *native = frame->closure->function->native;

        if (native && native->type == OBJ_NATIVE_CLASS) {
          int argCount = stackTop - frame->slots - 1;
          NativeClassFn nativeClassFn = ((ObjNativeClass *) native)->classFn;
          InterpretResult result = nativeClassFn(*vm, argCount, stackTop - argCount).result;
        }
        else
          if (current->isInInstance() && (!eventFlag || !current->isFirstInstance()))
            if (current->isFirstInstance())
              return INTERPRET_OK;
            else {
    //        CallFrame *frame = &current->frames[--current->frameCount];

    //TODO: fix this        current->closeUpvalues(frame->slots);
    //        caller->coinstance = caller;
              if (current->isDone()) {
    //            FREE(CoThread, instance->coThread);
    //            instance->coThread = NULL;
              }
              current->savedStackTop = stackTop;
              ::instance = ::instance->caller;
              current = ::instance->coThread;
              frame = &current->frames[current->frameCount - 1];
              stackTop = current->savedStackTop;
            }
          else
            // suspend app
            return INTERPRET_SUSPEND;
      }
      break;
    }
    }
  }
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef STRING_OP
#undef BINARY_OP
}

bool CoThread::isDone() {
  Chunk *chunk = &frames[0].closure->function->chunk;

  return frames[0].ip >= chunk->code + chunk->count;
}

bool CoThread::isFirstInstance() {
  return ::instance->caller == NULL;
}

bool CoThread::isInInstance() {
  return frameCount == 1;
}

CallFrame *CoThread::getFrame(int index) {
  return &frames[frameCount - index - 1];
}

void CoThread::onReturn(Value &returnValue) {
  CallFrame *frame = &frames[--frameCount];

  closeUpvalues(frame->slots);
  stackTop = frame->slots;

  if (frame->closure->function->type.valueType != VAL_VOID && frame->closure->function->type.valueType != VAL_HANDLER)
    push(returnValue);
}

bool CoThread::getFormFlag() {
  bool formFlag = false;

  for (int ndx = 0; !formFlag && ndx < frameCount; ndx++)
    formFlag = true;//layoutObjects[ndx]->obj.func.attrSets != NULL;

  return formFlag;
}

void ObjInstance::initValues() {
  uiValuesInstances = new ObjInstance *[coThread->frameCount];

  for (int ndx = 0; ndx < coThread->frameCount; ndx++) {
    CallFrame &frame = coThread->frames[ndx];
    ObjClosure *outClosure = frame.uiClosure;

    uiValuesInstances[numValuesInstances++] = newInstance(NULL);

    CoThread *instanceThread = uiValuesInstances[ndx]->coThread;

    *instanceThread->savedStackTop++ = OBJ_VAL(outClosure);
    instanceThread->call(outClosure, 0);
    instance = uiValuesInstances[ndx];
    instanceThread->run();
    instance->coThread->savedStackTop = stackTop;

    for (int ndx2 = -1; (ndx2 = outClosure->function->instanceIndexes->getNext(ndx2)) != -1;)
      ((ObjInstance *) AS_OBJ(instanceThread->fields[ndx2]))->initValues();
  }
}

void ObjInstance::uninitValues() {
  for (int ndx = 0; ndx < numValuesInstances; ndx++) {
    CallFrame &frame = uiValuesInstances[ndx]->coThread->frames[0];
    ObjClosure *outClosure = frame.closure;

    for (int ndx2 = -1; (ndx2 = outClosure->function->instanceIndexes->getNext(ndx2)) != -1;)
      ((ObjInstance *) AS_OBJ(uiValuesInstances[ndx]->coThread->fields[ndx2]))->uninitValues();
  }

  if (uiValuesInstances) {
    for (int ndx = 0; ndx < coThread->frameCount; ndx++)
      FREE(OBJ_INSTANCE, uiValuesInstances[ndx]);

    delete[] uiValuesInstances;
    uiValuesInstances = NULL;
    numValuesInstances = 0;
  }
}

Point ObjInstance::recalculateLayout() {
  Point size = {0, 0};
  uiLayoutInstances = new ObjInstance *[numValuesInstances];

  for (int ndx = 0; ndx < numValuesInstances; ndx++) {
    CoThread *valuesThread = uiValuesInstances[ndx]->coThread;
    CallFrame &valuesFrame = valuesThread->frames[0];
    ObjClosure *valuesClosure = AS_CLOSURE(valuesThread->fields[0]);
    ObjClosure *layoutClosure = AS_CLOSURE(valuesThread->fields[valuesClosure->function->fieldCount - 1]);
    ObjInstance *saved = instance;

    uiLayoutInstances[ndx] = newInstance(NULL);

    CoThread *instanceThread = uiLayoutInstances[ndx]->coThread;

    *instanceThread->savedStackTop++ = OBJ_VAL(layoutClosure);
    instanceThread->call(layoutClosure, 0);
    instance = uiLayoutInstances[ndx];
    instanceThread->run();
    instance->coThread->savedStackTop = stackTop;
    instance = saved;

    long frameSize = AS_INT(instanceThread->fields[layoutClosure->function->fieldCount - 3]);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      size[dir] = std::max(size[dir], (int) (dir ? frameSize & 0xFFFF : frameSize >> 16));
  }

  return size;
}

extern void initDisplay();
extern void uninitDisplay();

Point ObjInstance::repaint() {
  if (coThread->getFormFlag()) {
    uninitValues();
    initValues();
    Point totalSize = recalculateLayout();

    initDisplay();
//    onEvent(EVENT_RELEASE, {10, 10}, {150, 80});

    paint({0, 0}, totalSize);
    return totalSize;
  }

  return {0, 0};
}

void ObjInstance::paint(Point pos, Point size) {
  for (int ndx = 0; ndx < numValuesInstances; ndx++) {
    CoThread *layoutThread = uiLayoutInstances[ndx]->coThread;
    CallFrame &layoutFrame = layoutThread->frames[0];
    ObjClosure *layoutClosure = AS_CLOSURE(layoutThread->fields[0]);
    ObjClosure *paintClosure = AS_CLOSURE(layoutThread->fields[layoutClosure->function->fieldCount - 2]);
    Value value = {VOID_VAL};

    *layoutThread->savedStackTop++ = OBJ_VAL(paintClosure);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      *layoutThread->savedStackTop++ = INT_VAL(pos[dir]);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      *layoutThread->savedStackTop++ = INT_VAL(size[dir]);

    layoutThread->call(paintClosure, NUM_DIRS << 1);
    instance = uiLayoutInstances[ndx];
    layoutThread->run();
    layoutThread->onReturn(value);
  }
}

void ObjInstance::onReturn(Value &returnValue) {
  if (caller && handler) {
    CoThread *callerThread = caller->coThread;
    Value value = {VAL_VOID};

    *callerThread->savedStackTop++ = OBJ_VAL(handler);
    callerThread->call(handler, 0);
    callerThread->run();
    callerThread->onReturn(value);
  }
}

bool ObjInstance::onEvent(Event event, Point pos, Point size) {
  bool flag = false;

  for (int ndx = numValuesInstances - 1; !flag && ndx >= 0; ndx--) {
    CoThread *layoutThread = uiLayoutInstances[ndx]->coThread;
    CallFrame &layoutFrame = layoutThread->frames[0];
    ObjClosure *layoutClosure = AS_CLOSURE(layoutThread->fields[0]);
    ObjClosure *eventClosure = AS_CLOSURE(layoutThread->fields[layoutClosure->function->fieldCount - 1]);
    Value value = {VOID_VAL};
    int frameCount = layoutThread->frameCount;

    *layoutThread->savedStackTop++ = OBJ_VAL(eventClosure);
    *layoutThread->savedStackTop++ = INT_VAL((int) event);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      *layoutThread->savedStackTop++ = INT_VAL(pos[dir]);

    for (int dir = 0; dir < NUM_DIRS; dir++)
      *layoutThread->savedStackTop++ = INT_VAL(size[dir]);

    layoutThread->call(eventClosure, (NUM_DIRS << 1) + 1);
    instance = uiLayoutInstances[ndx];
    layoutThread->run();

    if (layoutThread->frameCount > frameCount)
      flag = AS_BOOL(*--layoutThread->savedStackTop);
    layoutThread->onReturn(value);
#ifdef DEBUG_TRACE_EXECUTION
    layoutThread->printStack();
#endif
  }

  return flag;
}
#if 0
Object parseCreateUIValuesSub(QEDProcess process, Object value, Path path, int flags, LambdaDeclaration handler) {
  Object childValue = process.execCmdRaw(value, handler, null);

  if (childValue is Obj) {
    childValue.valueTree = new ValueTree();

    childValue.func.parseCreateUIValues(process, childValue, 0, null, childValue.valueTree);
  }

  return childValue;
}

void parseRefresh(QEDProcess process, ValueStack valueStack, Object value, ListUnitAreas areas, List<int> pos) {
  LayoutUnitArea unitArea = areas;
  Obj obj = value;
  List<LayoutContext> layoutContexts = List.filled(numDirs, null);

  for (int dir = 0; dir < numDirs; dir++)
    layoutContexts[dir] = new LayoutContext(pos[dir], 0/*size[dir]*/, unitArea.trees[dir]);

  attrSets.parseRefresh(process, valueStack, [], new Path(), obj.valueTree, unitArea.unitAreas, topSizers, layoutContexts);
}

Path parseLocateEvent(QEDProcess process, Obj obj, ListUnitAreas areas, List<int> pos, List<int> size, List<int> location, Path currentFocusPath) {
  LayoutUnitArea unitArea = areas;
  List<LayoutContext> layoutContexts = List.filled(numDirs, null);

  for (int dir = 0; dir < numDirs; dir++)
    layoutContexts[dir] = new LayoutContext(pos[dir], size[dir], unitArea.trees[dir]);

  return attrSets.parseLocateEvent(process, obj, [], new Path(), obj.valueTree, null, unitArea.unitAreas, subAttrsets, topSizers, layoutContexts, location, currentFocusPath);
}

void out(QEDProcess process, Obj obj, Path focusPath) {
  attrSets.parseOut(process, obj, focusPath, new Path(), obj.valueTree, null);
}
#endif
ObjInternal *newInternal() {
  ObjInternal *internal = ALLOCATE_OBJ(ObjInternal, OBJ_INTERNAL);

  internal->object = NULL;
  return internal;
}

ObjInstance *newInstance(ObjInstance *caller) {
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);

  instance->coThread = new CoThread(instance);
  instance->numValuesInstances = 0;
  instance->uiValuesInstances= NULL;
  instance->caller = caller;
  return instance;
}

ObjCompilerInstance *newCompilerInstance(ObjCallable *callable) {
  ObjCompilerInstance *instance = ALLOCATE_OBJ(ObjCompilerInstance, OBJ_COMPILER_INSTANCE);

  instance->callable = callable;
  return instance;
}

ObjClosure *newClosure(ObjFunction *function, ObjInstance *parent) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalueCount);

  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjType objType = function->name && !strcmp(function->name->chars, "return") ? OBJ_RETURN : OBJ_CLOSURE;
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, objType);

  closure->parent = parent;
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFunction *newFunction() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  function->chunk.init();
  function->native = NULL;
  function->instanceIndexes = new IndexList();
  function->eventFlags = 0L;
  function->uiFunction = NULL;
//  function->uiFunctions = new std::unordered_map<std::string, ObjFunction*>();
  return function;
}

ObjNative *newNative(NativeFn function) {
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);

  native->arity = 0;
  native->upvalueCount = 0;
  native->name = NULL;
  native->function = function;
  return native;
}

ObjPrimitive *newPrimitive(char *name, Type type) {
  ObjPrimitive *primitive = ALLOCATE_OBJ(ObjPrimitive, OBJ_PRIMITIVE);

  primitive->name = takeString(name, strlen(name));
  primitive->type = type;
  return primitive;
}

ObjFunctionPtr *newFunctionPtr(Type type, int arity, Type *parms) {
  ObjFunctionPtr *functionPtr = ALLOCATE_OBJ(ObjFunctionPtr, OBJ_FUNCTION_PTR);

  functionPtr->type = type;
  functionPtr->arity = arity;
  functionPtr->parms = arity ? new Type[arity] : NULL;

  for (int index = 0; index < arity; index++)
    functionPtr->parms[index] = parms[index];

  return functionPtr;
}

static ObjString *allocateString(char *chars, int length) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

ObjString *takeString(char *chars, int length) {
  return allocateString(chars, length);
}

ObjString *copyString(const char *chars, int length) {
  char *heapChars = ALLOCATE(char, length + 1);

  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length);
}

ObjUpvalue *newUpvalue(Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);

  upvalue->closed = OBJ_VAL(NULL);
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void printFunction(ObjCallable *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }

  printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_INTERNAL:
    printf("<internal>");
    break;

  case OBJ_INSTANCE: {
    ObjString *name = AS_CLOSURE(AS_INSTANCE(value)->coThread->fields[0])->function->name;

    if (name)
      printf("<%.*s instance>", name->length, name->chars);
    else
      printf("<instance>");
    break;
  }
  case OBJ_COMPILER_INSTANCE: {
    ObjString *name = AS_COMPILER_INSTANCE(value)->callable->name;

    printf("<%.*s instance>", name->length, name->chars);
    break;
  }
  case OBJ_CLOSURE:
  case OBJ_RETURN:
    printFunction(AS_CLOSURE(value)->function);
    break;
  case OBJ_FUNCTION:
  case OBJ_NATIVE:
    printFunction(AS_CALLABLE(value));
    break;
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  }
}
#if 0
/*
 *    Copyright (C) 2016 Hocus Codus Software inc.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */


import '../Obj.dart';
import '../Path.dart';
import '../QEDProcess.dart';
import '../../ui/AttrSet.dart';
import '../../ui/LayoutContext.dart';
import '../../ui/ListUnitAreas.dart';
import '../../ui/Sizer.dart';
import '../../ui/ValueTree.dart';
import '../Util.dart';
import 'ArrayRefDeclaration.dart';
import 'ExplicitArrayDeclaration.dart';
import 'ImplicitArrayDeclaration.dart';
import 'Executer.dart';
import 'CodeExecuter.dart';
import 'FunctionDeclaration.dart';
import 'LambdaWithParamsDeclaration.dart';
import 'dart:typed_data';
import 'dart:convert';

class LambdaDeclaration {
	static int spaceHeader = 0;

	String name;
	int dataSize;
	Executer executer;
	List<LambdaDeclaration> _functionDeclarations = List.filled(0, null, growable: true);
	ChildAttrSets attrSets;
	List<Sizer> topSizers;
	Object subAttrsets = null;

	LambdaDeclaration(String name) {
		this.name = name;
	}

	int getNumDim() {
		return 0;
	}

	int getNumParams() {
		return 0;
	}

	int getLimit(List data, int index) {
		return 0;
	}

	void setIndex(Obj obj, int index, int value) {
	}

	Object addItem(List data, int index, Object item) {
		return null;
	}

	Object removeItem(List data, int index) {
		return null;
	}

	void addFunctionDeclaration(LambdaDeclaration declaration) {
		_functionDeclarations.add(declaration);
	}

	int getNumFunctionDeclarations() {
		return _functionDeclarations.length;
	}

	LambdaDeclaration getFunctionDeclarationPath(Path path, int level) {
		return level < path.getNumLevels() ? _functionDeclarations[path.get(level)].getFunctionDeclarationPath(path, level + 1) : this;
	}

	LambdaDeclaration getFunctionDeclaration(int index) {
		if (index >= _functionDeclarations.length)
			index=index;
		return _functionDeclarations[index];
	}

	LambdaDeclaration getFunctionByName(String name) {
		for (int index = 0; index < _functionDeclarations.length; index++)
			if (_functionDeclarations[index].name == name)
				return _functionDeclarations[index];

		return null;
	}

	LambdaDeclaration getSubFunction(int index) {
		if (index >= _functionDeclarations.length)
			index=index;
		return _functionDeclarations[index];
	}

	LambdaDeclaration findReference(List<String> names, int level) {
		if (level < names.length) {
			String name = names[level];

			for (var index = 0; index < getNumFunctionDeclarations(); index++) {
				LambdaDeclaration unit = getFunctionDeclaration(index);

				if (unit.name != null && name == unit.name)
					return unit.findReference(names, level + 1);
			}

			print("Cannot find " + name);
			return null;
		}
		else
			return this;
	}

	void read(FunctionDeclaration mainFunc, LambdaDeclaration parent, FakeStream inputStream) {
		dataSize = readS(inputStream);

		int length = readS(inputStream);
		ByteData byteCode = new ByteData(length);

		inputStream.readBytes(byteCode);
		int cleanupOffset = readS(inputStream);
		int numSubFunctions = inputStream.read();

		executer = executer != null ? executer : setExecuter(byteCode, cleanupOffset);

		for (var index = 0; index < numSubFunctions; index++) {
			int type = inputStream.read();
			String name = readString(inputStream);
			LambdaDeclaration func;

			switch (type) {
				case 0:
					func = new LambdaDeclaration(name);
					break;

				case 1:
					func = new LambdaWithParamsDeclaration(name);
					break;

				case 2:
					func = new ImplicitArrayDeclaration(name);
					break;

				case 3:
					func = new ExplicitArrayDeclaration(name);
					break;

				case 4:
					func = this.name.startsWith("_lib") ? new PredefinedDeclaration(name) : new FunctionDeclaration(name);
					break;
			}

			if (func != null) {
				addFunctionDeclaration(func);
				func.read(mainFunc, this, inputStream);
			}
			else
				print("Cannot add function");
		}

		int numAttrSets = inputStream.read();
		List<int> numZones = List.filled(numDirs, 0);

		attrSets = numAttrSets != 0 ? new ChildAttrSets([0], numZones, 0, [2, 1], new ValueStack(), numAttrSets, mainFunc, this, inputStream, 0) : null;

		if (attrSets != null) {
			topSizers = List.filled(numDirs, null);

			for (int dir = 0; dir < numDirs; dir++) {
				List<int> zone = null;

				topSizers[dir] = new Maxer(-1);

				if (parent is ImplicitArrayDeclaration) {
//					int zFlags = attrSets.zFlags[dir];

					zone = [0];//[zFlags > 0 ? zFlags != 1 ? 1 : 0 : ctz([-zFlags - 1])];
					topSizers[dir].put(SizerType.maxer, -1, 0, 0, 0, false, [Path()]);
					topSizers[dir].children.add(new Adder(-1));
				}
				else
					zone=zone;

				attrSets.parseCreateSizers(topSizers, dir, zone, new Path(), new Path());
				attrSets.parseAdjustPaths(topSizers, dir);
				numZones[dir] = zone != null ? zone[0] + 1 : 1;
			}

			subAttrsets = parent is ImplicitArrayDeclaration ? parseCreateSubSets(topSizers, new Path(), numZones) : createIntersection(-1, topSizers);
		}
	}

	Object parseCreateSubSets(List<Sizer> topSizers, Path path, List<int> numZones) {
		int dir = path.getNumLevels();

		if (dir < numDirs) {
			List values = List.filled(0, null, growable: true);

			for (int index = 0; index < numZones[dir]; index++)
				values.add(parseCreateSubSets(topSizers, path.concatInt(index), numZones));

			return values;
		}
		else {
			List<Sizer> zoneSizers = List.filled(numDirs, null);
			bool valid = true;

			for (dir = 0; valid && dir < numDirs; dir++) {
				zoneSizers[dir] = topSizers[dir].getZoneSizer(path.values[dir]);
				valid = zoneSizers[dir] != null;
			}

			return valid ? createIntersection(-1, zoneSizers) : null;
		}
	}

	Obj getRealObj(Obj obj, int domain) {
		return obj;
	}

	static IntTreeUnit createIntersection(int index, List<Sizer> sizers) {
		bool isLeaf = sizers[0].children == null;

		for (int dir = 1; dir < numDirs; dir++)
			if (isLeaf ? sizers[dir].children != null : sizers[dir].children == null) {
				print("Inconsistent data provided, stopping analysis...");
				return null;
			}

		if (isLeaf)
			return new IntTreeUnit(index);

		List<int> indexes = List.filled(numDirs - 1, 0);
		List<IntTreeUnit> list = List.filled(0, null, growable: true);

		for (int ndx = 0; ndx < sizers[0].children.length; ndx++) {
			int dir = 1;
			int index = sizers[0].children[ndx].index;

			for (; dir < numDirs; dir++) {
				List<Sizer> children = sizers[dir].children;

				while (indexes[dir - 1] < children.length && children[indexes[dir - 1]].index < index)
					indexes[dir - 1]++;

				if (indexes[dir - 1] == children.length)
					break;
			}

			if (dir < numDirs)
				break;

			List<Sizer> subSizers = List.filled(numDirs, null);
			bool found = true;

			for (dir = 0; found && dir < numDirs; dir++) {
				subSizers[dir] = sizers[dir].children[dir != 0 ? indexes[dir - 1] : ndx];
				found = subSizers[dir].index == index;
			}

			if (found) {
				IntTreeUnit value = createIntersection(index, subSizers);

				if (value != null)
					list.add(value);
			}
		}

		return list.length != 0 ? new IntTreeUnit.fromList(index, list) : null;
	}

	Executer setExecuter(ByteData byteCode, int cleanupOffset) {
		return new CodeExecuter(byteCode, cleanupOffset);
	}

	void parseCreateUIValues(VM &vm, Object value, int flags, List<int> dimIndexes, ValueTree valueTree) {
		if (attrSets != null)
			attrSets.parseCreateUIValues(process, value, null, flags, dimIndexes, valueTree);
	}

	Object parseCreateUIValuesSub(VM &vm, Object value, Path path, int flags, LambdaDeclaration handler) {
		Object childValue = process.execCmdRaw(value, handler, null);

		if (childValue is Obj) {
			childValue.valueTree = new ValueTree();

			childValue.func.parseCreateUIValues(process, childValue, 0, null, childValue.valueTree);
		}

		return childValue;
	}

	void parseCreateAreasTree(VM &vm, ValueStack valueStack, Object value, int dimFlags, Path path, List areas, int offset) {
		List subAreas = List.filled(attrSets.numAreas, null);
		Obj obj = value;
		List<IntTreeUnit> sizeTrees = List.filled(numDirs, null);

		attrSets.parseCreateAreasTree(process, valueStack, 0, new Path(), obj.valueTree, subAreas);

		for (int dir = 0; dir < numDirs; dir++)
			sizeTrees[dir] = parseResize(dir, [], subAreas, 0);

		areas[offset] = path.addValue(areas[offset], new LayoutUnitArea(sizeTrees, subAreas));
	}

	IntTreeUnit *parseResize(int dir, List<int> limits, List areas, int offset) {
		return topSizers[dir].parseResize(areas, new Path.fromNumDim(limits.length), dir, limits);
	}

	void parseRefresh(VM &vm, ValueStack valueStack, Object value, ListUnitAreas areas, List<int> pos) {
		LayoutUnitArea unitArea = areas;
		Obj obj = value;
		List<LayoutContext> layoutContexts = List.filled(numDirs, null);

		for (int dir = 0; dir < numDirs; dir++)
			layoutContexts[dir] = new LayoutContext(pos[dir], 0/*size[dir]*/, unitArea.trees[dir]);

		attrSets.parseRefresh(process, valueStack, [], new Path(), obj.valueTree, unitArea.unitAreas, topSizers, layoutContexts);
	}

	Path parseLocateEvent(VM &vm, Obj obj, ListUnitAreas areas, List<int> pos, List<int> size, List<int> location, Path currentFocusPath) {
		LayoutUnitArea unitArea = areas;
		List<LayoutContext> layoutContexts = List.filled(numDirs, null);

		for (int dir = 0; dir < numDirs; dir++)
			layoutContexts[dir] = new LayoutContext(pos[dir], size[dir], unitArea.trees[dir]);

		return attrSets.parseLocateEvent(process, obj, [], new Path(), obj.valueTree, null, unitArea.unitAreas, subAttrsets, topSizers, layoutContexts, location, currentFocusPath);
	}

	void out(VM &vm, Obj obj, Path focusPath) {
		attrSets.parseOut(process, obj, focusPath, new Path(), obj.valueTree, null);
	}

	LambdaDeclaration dec() {
		return null;
	}

	static LambdaDeclaration evalProcess(VM &vm, Obj obj) {
		return eval(process.layoutObjects[0].obj.func.getFunctionDeclarationPath(Path.eval(process, obj), 0), obj.read(process));
	}

	static LambdaDeclaration eval(LambdaDeclaration declaration, int numDim) {
		return numDim != 0 ? new ArrayRefDeclaration(declaration, numDim) : declaration;
	}

	static LambdaDeclaration evalStream(FunctionDeclaration mainFunc, FakeStream inputStream) {
		return eval(mainFunc.getFunctionDeclarationPath(Path.read(inputStream), 0), inputStream.read());
	}

	static int readS(FakeStream inputStream) {
		int value = 0;

		for (var index = 0; index < 2; index++) {
			value <<= 8;
			value |= inputStream.read().toUnsigned(8);
		}

		return value;
	}

	static int readL(FakeStream inputStream) {
		int value = 0;
		int count = inputStream.read();

		for (var index = 0; index < count; index++) {
			value <<= 8;
			value |= inputStream.read().toUnsigned(8);
		}

		return value;
	}

	static String readString(FakeStream inputStream) {
		int length = readS(inputStream);
		ByteData bytes = new ByteData(length);

		inputStream.readBytes(bytes);
		var list = bytes.buffer.asUint8List(bytes.offsetInBytes, bytes.lengthInBytes);
		return utf8.decode(list);
	}

	String toString() {
		return name;
	}

	StringBuffer toString2(StringBuffer buffer) {
		appendDecStr(buffer);

		buffer.write("{\n");
		spaceHeader++;

		for (var index = 0; index < getNumFunctionDeclarations(); index++)
			getFunctionDeclaration(index).toString2(addSpaceHeader(buffer)).write("\n");

		spaceHeader--;
		addSpaceHeader(buffer).write("}");
		return buffer;
	}

	StringBuffer appendDecStr(StringBuffer buffer) {
		if (name != null)
			buffer.write(name);

		buffer.write("(");

		for (var index = 0; index < getNumParams(); index++) {
			if (index != 0)
				buffer.write(", ");

//			buffer.write(getFieldDeclaration(index).getdecstr());
			buffer.write("p");
			buffer.write(index);
		}

		buffer.write(")");
		buffer.write(0 < getNumFunctionDeclarations() ? "" : " ");
		return buffer;
	}

	static StringBuffer addSpaceHeader(StringBuffer buffer) {
		for (var index = 0; index < spaceHeader; index++)
			buffer.write("  ");

		return buffer;
	}

	bool isPredefined() {
		return !(executer is CodeExecuter);// || executer is ArrayExecuter);
	}
	//
	//		Object cast(Type valueType, Object value) {
	//			CodeObject codeObject = (CodeObject) value;
	//
	//			return codeObject.findFunc(this);
	//		}
}

class PredefinedDeclaration extends FunctionDeclaration {
	PredefinedDeclaration(String name) : super(name) {
	}

	void parseCreateAreasTree(VM &vm, ValueStack valueStack, Object value, int dimFlags, Path path, List areas, int offset) {
		areas[offset] = path.addValue(areas[offset], new UnitArea(process.getTextSize(value.toString(), -1)));
	}

	void parseRefresh(VM &vm, ValueStack valueStack, Object value, ListUnitAreas areas, List<int> pos) {
		UnitArea unitArea = areas;

		process.drawText(value.toString(), [0,0], [unitArea.unitArea[0], unitArea.unitArea[1]], [pos[0], pos[1]], -1, -1, -1, -1);
	}
}

abstract class FakeStream {
	int read();
	void readBytes(ByteData byteCode);
}
#endif