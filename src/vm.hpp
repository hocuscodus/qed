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