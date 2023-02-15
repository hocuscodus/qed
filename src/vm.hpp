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
#include "compiler.hpp"

struct AttrSet;

struct VM {
	Point totalSize;/*
  std::vector<LayoutObject *> layoutObjects;
	Obj *current2;
	std::vector<void *> stack;
	///////
	Path currentPath;
	Obj currentObj;
	AttrSet *currentAttrSet;
//	List<int> currentPos;
//	List<int> currentSize;
	///////
	Path inputPath;
	Obj inputObj;
	AttrSet *inputAttrSet;
//	List inputValues;
	int inputIndex;
	Point inputPos;
	Point inputSize;
	int inputColor;
	int inputFontSize;
	int inputStyle;
*/
  VM(CoThread *instance, bool eventFlag);

  InterpretResult run();
  InterpretResult interpret(ObjClosure *closure);
  CallFrame *getFrame(int index = 0);
  bool recalculate();
  void repaint();
  bool onEvent(Event event, Point pos);
  void push(Obj *obj);
};

ObjNativeClass *newNativeClass(NativeClassFn classFn);

#endif