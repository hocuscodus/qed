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