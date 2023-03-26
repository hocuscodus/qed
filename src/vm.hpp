/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_vm_h
#define qed_vm_h

#include "layout.hpp"
#include "chunk.hpp"
#include "scanner.hpp"
#include "object.hpp"

struct VM {
  CoThread *coThread;

  VM(CoThread *coThread, bool eventFlag);

  InterpretResult run();
  InterpretResult interpret(ObjClosure *closure);
};

ObjNativeClass *newNativeClass(NativeClassFn classFn);

#endif