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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.hpp"
#include "resolver.hpp"
#include "codegen.hpp"
#include "debug.hpp"
#include "memory.h"
#include "object.hpp"
#include "attrset.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#include "astprinter.hpp"
#endif

// sdl
#include <SDL.h>
#include <SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <SDL_timer.h>
#endif

/*
class ReturnHandler {
public:
	void onPause(VM &process, ObjInstance &obj) {
		if (obj.caller != NULL)
			obj.caller.onPause(process);
	}

	virtual void onHalt(VM &process, Obj obj);
	virtual void onReturn(VM &process, Obj obj, void *value) {}
};

class CallReturnHandler : ReturnHandler {
	static CallReturnHandler obj;
public:
	void onHalt(VM &process, Obj obj) {
		obj.caller.onPause(process);
	}

	void onReturn(VM &process, Obj obj, void *value) {
		process.remove(obj, value);
		obj.func.executer.cleanup(process, obj);
	}
};

class NewReturnHandler : ReturnHandler {
	static NewReturnHandler obj;
public:
	void onPause(VM &process, Obj obj) {
		onNew(process, obj);
	}

	void onHalt(VM &process, Obj obj) {
		onNew(process, obj);
	}

	void onReturn(VM &process, Obj obj, void *value) {
		if (obj.handlerEnv != NULL) {
			process.push(new Obj(process.current, obj.handlerEnv, obj.handlerIndex, EventReturnHandler.obj));
			process.setCallInfo(this);

			if (value != NULL)
				process.setCallInfo(value);
		}
	}

	void onNew(VM &process, Obj obj) {
		process.remove(obj, obj.caller != NULL ? obj : NULL);
		obj.caller = NULL;
	}
};
*/
#if 0
	void parseCreateUIValues(QEDProcess process, Object value, int flags, List<int> dimIndexes, ValueTree valueTree) {
		if (attrSets != null)
			attrSets.parseCreateUIValues(process, value, null, flags, dimIndexes, valueTree);
	}

	Object parseCreateUIValuesSub(QEDProcess process, Object value, Path path, int flags, LambdaDeclaration handler) {
		Object childValue = process.execCmdRaw(value, handler, null);

		if (childValue is Obj) {
			childValue.valueTree = new ValueTree();

			childValue.func.parseCreateUIValues(process, childValue, 0, null, childValue.valueTree);
		}

		return childValue;
	}

	void parseCreateAreasTree(QEDProcess process, ValueStack valueStack, Object value, int dimFlags, Path path, List areas, int offset) {
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
////// implicit

//	ValueTree parseCreateUIValuesSub(QEDProcess process, Object value, Path path, int flags, LambdaDeclaration handler) {
	void parseCreateUIValues(QEDProcess process, Object value, int flags, List<int> dimIndexes, ValueTree valueTree) {
		Obj obj = value;
		int numDim = getNumDim();
		int dimLimit = 1 << numDim;
		List<int> limits = getLimits(obj.data, new List<bool>(1));
		List<int> flags = [returnType.isPredefined() ? 1 << (numDim - 1) : returnType.attrSets.parseFlags];
		Obj nullSubObj = new Obj.withFunc(null, obj, returnType, null);

		for (int dimIndex = ctz(flags); dimIndex != -1; dimIndex = ctz(flags))
			for (bool ovFlag = initLoop(obj, limits, dimIndex); !ovFlag; ovFlag = incLoop(obj, limits, dimIndex)) {
				Path path = getPath(obj, dimIndex);
				Object subObj = dimIndex == (1 << numDim) - 1 ? getSubObj(obj, path) : nullSubObj;

				if (subObj is Obj)
					subObj.func.parseCreateUIValues(process, subObj, dimIndex, path.values, valueTree);
			}
	}

	void parseCreateAreasTree(QEDProcess process, ValueStack valueStack, Object value, int dimFlags, Path path, List areas, int offset) {
		int numDim = getNumDim();
		Obj obj = value;
		List<int> limits = getLimits(obj.data, new List<bool>(1));
		List<int> flags = [returnType.isPredefined() ? 1 << (numDim - 1) : returnType.attrSets.areaParseFlags];
		bool isSubFunction = returnType == getFunctionDeclaration(0);
		List subAreas = List.filled(isSubFunction ? returnType.attrSets.numAreas : 1, null);

		for (int dimIndex = ctz(flags); dimIndex != -1; dimIndex = ctz(flags))
			for (bool ovFlag = initLoop(obj, limits, dimIndex); !ovFlag; ovFlag = incLoop(obj, limits, dimIndex)) {
				Path path = getPath(obj, dimIndex);

				if (isSubFunction)
					returnType.attrSets.parseCreateAreasTree(process, valueStack, dimIndex, path, (value as Obj).valueTree, subAreas);
				else
					returnType.parseCreateAreasTree(process, valueStack, getSubObj(obj, path), dimIndex, path, subAreas, 0);
			}

		List<IntTreeUnit> sizeTrees = List.filled(numDirs, null);

		for (int dir = 0; dir < numDirs; dir++)
			sizeTrees[dir] = returnType.parseResize(dir, limits, subAreas, 0);

		areas[offset] = path.addValue(areas[offset], new LayoutUnitArea(sizeTrees, subAreas));
	}

	void parseRefresh(QEDProcess process, ValueStack valueStack, Object value, ListUnitAreas areas, List<int> pos) {
		Obj obj = value;
		List<int> limits = getLimits(obj.data, new List<bool>(1));
		bool isSubFunction = returnType == getFunctionDeclaration(0);

		if (isSubFunction) {
			LayoutUnitArea unitArea = areas;
			List<LayoutContext> layoutContexts = List.filled(numDirs, null);

			for (int dir = 0; dir < numDirs; dir++)
				layoutContexts[dir] = new LayoutContext(pos[dir], 0, unitArea.trees[dir]);

			returnType.attrSets.parseRefresh(process, valueStack, limits, new Path.fromNumDim(getNumDim()), obj.valueTree, unitArea.unitAreas, returnType.topSizers, layoutContexts);
		}
		else
			for (bool ovFlag = initLoop(obj, limits, -1); !ovFlag; ovFlag = incLoop(obj, limits, -1)) {
				Path path = getFullPath(obj, -1); // could be simplified (no use of parm as dim index)

				returnType.parseRefresh(process, valueStack, getSubObj(obj, path), areas, pos);
			}
	}

	Path parseLocateEvent(QEDProcess process, Obj obj, ListUnitAreas areas, List<int> pos, List<int> size, List<int> location, Path currentFocusPath) {
		List<int> limits = getLimits(obj.data, new List<bool>(1));
		bool isSubFunction = returnType == getFunctionDeclaration(0);
		Path focusPath = null;

		if (isSubFunction) {
			Path path = new Path.fromNumDim(numDirs);
			List<Sizer> sizers = List.filled(numDirs, null);
			LayoutUnitArea unitArea = areas;
			List<LayoutContext> layoutContexts = List.filled(numDirs, null);
			var subLocation = new List<int>.from(location);
			var subAttrsets = returnType.subAttrsets;
			List<int> zones = List.filled(numDirs, null);

			for (int dir = 0; dir < numDirs; dir++) {
				layoutContexts[dir] = new LayoutContext(pos[dir], size[dir], unitArea.trees[dir].set[1]);
				zones[dir] = returnType.topSizers[dir].children[1].findZone(layoutContexts[dir], subLocation, dir, path);
				sizers[dir] = returnType.topSizers[dir].children[1].children[zones[dir]];
				subAttrsets = (subAttrsets as List)[zones[dir]];
			}

			if (subAttrsets != null)
				focusPath = returnType.attrSets.parseLocateEvent(process, obj, limits, new Path.fromNumDim(getNumDim()), obj.valueTree, null, unitArea.unitAreas, subAttrsets, sizers, layoutContexts, subLocation, currentFocusPath);
		}
		else
			for (bool ovFlag = initLoop(obj, limits, -1); focusPath == null && !ovFlag; ovFlag = incLoop(obj, limits, -1)) {
				Path path = getFullPath(obj, -1); // could be simplified (no use of parm as dim index)

				focusPath = returnType.parseLocateEvent(process, getSubObj(obj, path), areas, pos, size, location, currentFocusPath);

				if (focusPath != null)
					focusPath = path.concat(focusPath);
			}

		return focusPath;
	}

	Object parseCreateUIValuesDim(QEDProcess process, Obj parent, int flags, LambdaDeclaration handler, int dimIndex) {
		Object val = null;

		if (dimIndex < getNumDim())
			if ((flags & (1 << dimIndex)) != 0) {
				int size = getLimit(parent.data, dimIndex);
				List list = List.filled(0, true);

				for (int index = 0; index < size; index++) {
//					getLimit(data, dimIndex);
					list.add(parseCreateUIValuesDim(process, parent, flags, handler, dimIndex + 1));
				}

				val = list;
			}
			else
				val = parseCreateUIValuesDim(process, parent, flags, handler, dimIndex + 1);
		else
			val = super.parseCreateUIValuesSub(process, parent, null, flags, handler);

		return val;
	}

import '../lang/Obj.dart';
import '../lang/Path.dart';
import '../lang/QEDProcess.dart';
import '../lang/Util.dart';
import 'ListUnitAreas.dart';
import 'ValueTree.dart';

class LayoutObject {
	Obj obj;
	ListUnitAreas unitAreas;
	List areas;

	LayoutObject(Obj obj) {
		this.obj = obj;
	}

	void init(VM &vm) {
		if (obj is Obj) {
			obj.valueTree = new ValueTree();
			obj.func.parseCreateUIValues(process, obj, 0, [], obj.valueTree);
		}

		if (obj.func.attrSets != null) {
			areas = List.filled(1, null);
			obj.func.parseCreateAreasTree(process, new ValueStack(), obj, 0, new Path(), areas, 0);
		}
	}

	void refresh(VM &vm, List<int> pos, List<int> size, List<int> clipPos, List<int> clipSize) {
		if (obj.func.attrSets != null)
			obj.func.parseRefresh(process, ValueStack(), obj, areas[0], createArea());
	}

	Path locateEvent(VM &vm, List<int> pos, List<int> size, List<int> location, Path currentFocusPath) {
		if (obj.func.attrSets != null)
			return obj.func.parseLocateEvent(process, obj, areas[0], List.filled(numDirs, 0), createArea(), location, currentFocusPath);

		return null;
	}

	List<int> getTotalSize() {
		List<int> size = createArea();

		for (int dir = numDirs - 1; dir >= 0; dir--)
			size[dir] = (areas[0] as ListUnitAreas).getSize(dir);

		return (size);
	}
}
#endif
VM::VM(ObjInstance *instance, bool eventFlag) {
  this->eventFlag = eventFlag;
  this->instance = instance;
  instance->coThread->resetStack();
  instance->coThread->frameCount = 0;
  instance->coThread->openUpvalues = NULL;
}

InterpretResult VM::run() {
  for (;;) {
    InterpretValue value = instance->coThread->run();

    if (value.result == INTERPRET_HALT) {
      CallFrame *frame = getFrame();
      Obj *native = frame->closure->function->native;

      if (native && native->type == OBJ_NATIVE_CLASS) {
        int argCount = instance->coThread->stackTop - frame->slots - 1;
        NativeClassFn nativeClassFn = ((ObjNativeClass *) native)->classFn;

        value = nativeClassFn(*this, argCount, instance->coThread->stackTop - argCount);
      }
    }

    switch(value.result) {
    case INTERPRET_SWITCH_THREAD:
      instance = value.as.coThread->instance;
      break;

    case INTERPRET_RETURN:
      onReturn(instance->coThread, value.as.returnValue);
      break;

    case INTERPRET_HALT: {
      if (instance->coThread->isInInstance() && (!eventFlag || !instance->coThread->isFirstInstance()))
        if (instance->coThread->isFirstInstance())
          return INTERPRET_OK;
        else {
//        CallFrame *frame = &current->frames[--current->frameCount];

//TODO: fix this        current->closeUpvalues(frame->slots);
//        caller->coinstance = caller;
          if (instance->coThread->isDone()) {
//            FREE(CoThread, instance->coThread);
//            instance->coThread = NULL;
          }
          instance = instance->coThread->caller->instance;
        }
      else
        // suspend app
        suspend();

//      stackTop = frame->slots;
      break;
    }

    case INTERPRET_CONTINUE:
      value.result = value.result;
//      return INTERPRET_RUNTIME_ERROR;
      break;

    default:
      return value.result;
    }
  }
}

void VM::push(Obj *obj) {
  layoutObjects.push_back(new LayoutObject(obj));
  current2 = obj;
}

InterpretResult VM::interpret(ObjClosure *closure) {
  instance->coThread->call(closure, instance->coThread->stackTop - instance->coThread->fields - 1);
  return run();
}

CallFrame *VM::getFrame(int index) {
  return instance->coThread->getFrame(index);
}

void VM::onReturn(CoThread *current, Value &returnValue) {
  ObjInstance *instance = current->instance;

  current = current->caller;

  if (instance->handler) {
    Value value = {VAL_VOID};

    *current->stackTop++ = OBJ_VAL(instance->handler);
    current->call(instance->handler, 0);
    current->run();
    current->onReturn(value);
  }
}
/*
  bool resize(List<int> size) {/ *
    Dimension dim = getSize();
    AttrSet.log3("Resize called.");
    setSize(Math.max((int) dim.getWidth(), size[0] + 20), Math.max((int) dim.getHeight(), size[1] + 40));
    canvas.setSize(Math.max((int) dim.getWidth(), size[0] + 20), Math.max((int) dim.getHeight(), size[1] + 40));
    refresh(new int[2], size);
    //        	deviceform.getui().mainFrame.canvas.repaint(pos[0], pos[1], size[0], size[1]);
* /    return (true);
  }
*/
extern void initDisplay();
extern void uninitDisplay();

bool VM::recalculate() {
  if (instance->coThread->getFormFlag()) {
    instance->uninitValues();
    instance->initValues();
    totalSize = instance->recalculateLayout();

    initDisplay();

    instance->paint({0, 0}, totalSize);
  }
  return true;
}
#if 0
void VM::redraw() {
  clipRefresh(createArea(), createAreaWithValue(10000));
}

void VM::clipRefresh(List<int> clipPos, List<int> clipSize) {
  if (getFormFlag()) {
    init2(totalSize);

    for (var ndx = 0; ndx < layoutObjects.length; ndx++)
      if (layoutObjects[ndx].obj is Obj) {
        Obj obj = layoutObjects[ndx].obj;

        if (obj.returnHandler is CallReturnHandler) {
          FunctionDeclaration func = obj.func;

          if (layoutObjects[ndx]/*.nodeIndex*/ != null && func.executer is CodeExecuter &&
              obj.instPointer == (func.executer as CodeExecuter).cleanupOffset)
            layoutObjects[ndx].refresh(this, createArea(), totalSize, clipPos, clipSize);
//						call.layoutObject.refresh(this);
        }
      }

    uninit();
/*
    init2(totalSize);

    for (var ndx = 0; ndx < calls.length; ndx++)
      if (calls[ndx] is QEDCall) {
        Obj obj = calls[ndx];

        if (call.returnHandler is CallReturnHandler)
          (call.env.getFunc() as FunctionDeclaration).nodeIndex.refresh(this, call.layoutObject);
//						call.layoutObject.refresh(this);
      }

    uninit();*/
  }
}

void VM::paint(Canvas canvas, Size size) {
//    final paint = Paint()..color = Colors.white.withAlpha(50);
  process.canvas = canvas;
  process.paint = Paint();
  process.redraw();
  process.canvas = null;
  process.paint = null;
}
#endif
void VM::repaint() {
//  setState(() {
    /*process.*/recalculate();
//  });
}

extern SDL_Renderer *rend2;
void VM::onEvent(const char *name, Point pos) {
  if (instance->onEvent(pos, totalSize)) {
    repaint();
    SDL_RenderPresent(rend2);
  }
}

void VM::suspend() {
  repaint();
  SDL_RenderPresent(rend2);

  SDL_Event event;

  // Events management
  while (true) {
    if (!SDL_WaitEvent(&event)) {
      printf("%s\n", SDL_GetError());
      exit(0);
    }

    switch (event.type) {
    case SDL_USEREVENT: {
        Value value = VOID_VAL;
        ObjInstance *instance = (ObjInstance *) event.user.data1;

        onReturn(instance->coThread, value);
        repaint();
        SDL_RenderPresent(rend2);
      }
      break;

    case SDL_MOUSEBUTTONUP:
      onEvent("onRelease", {event.button.x, event.button.y});
      break;

    case SDL_QUIT:
      exit(0);
      break;

    case SDL_KEYDOWN:
      // keyboard API for key pressed
      switch (event.key.keysym.scancode) {
      case SDL_SCANCODE_W:
      case SDL_SCANCODE_UP:
//        dest.y -= speed / 30;
        break;

      case SDL_SCANCODE_A:
      case SDL_SCANCODE_LEFT:
//        dest.x -= speed / 30;
        break;

      case SDL_SCANCODE_S:
      case SDL_SCANCODE_DOWN:
//        dest.y += speed / 30;
        break;

      case SDL_SCANCODE_D:
      case SDL_SCANCODE_RIGHT:
//        dest.x += speed / 30;
        break;

      default:
        break;
      }
  }
  }
}
