/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <iostream>
#include "vm.hpp"
#include "object.hpp"

struct ValueStack2 {
	std::stack<Value> map[ATTRIBUTE_END];

  ValueStack2() {
    push(ATTRIBUTE_ALIGN, FLOAT_VAL(0));
    push(ATTRIBUTE_POS, INT_VAL(0));
    push(ATTRIBUTE_OPACITY, FLOAT_VAL(1));
  }

	void push(int key, Value value) {
    map[key].push(value);
  }

	void pop(int key) {
    map[key].pop();
  }

	bool empty(int key) {
    return map[key].empty();
  }

	Value get(int key) {
    return empty(key) ? VOID_VAL : map[key].top();
  }
};

#define QNI_FN(name) \
  static Value qni_ ## name(int argCount, Value *args); \
  static bool qni_ ## name ## Var = addNativeFn("qni_" #name, qni_ ## name); \
  static Value qni_ ## name(int argCount, Value *args)

#define QNI_CLASS(name) \
  static InterpretResult qni_ ## name(VM &vm, int argCount, Value *args); \
  static bool qni_ ## name ## Var = addNativeClassFn("qni_" #name, qni_ ## name); \
  static InterpretResult qni_ ## name(VM &vm, int argCount, Value *args)

bool addNativeFn(const char *name, NativeFn nativeFn);
bool addNativeClassFn(const char *name, NativeClassFn nativeClassFn);
bool bindFunction(std::string prefix, ObjFunction *function);