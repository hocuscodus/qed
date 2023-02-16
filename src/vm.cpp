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
#include "attrset.hpp"

#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#include "astprinter.hpp"
#endif

/*
class ReturnHandler {
public:
	void onPause(VM &process, CoThread &obj) {
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

extern bool eventFlag;
extern CoThread *current;
extern VM *vm;
extern void suspend();
extern Value *stackTop;

VM::VM(CoThread *coThread, bool eventFlag) {
  ::vm = this;
  ::eventFlag = eventFlag;
  ::current = coThread;
}

InterpretResult VM::run() {
  InterpretResult result = CoThread::run();

  if (result == INTERPRET_SUSPEND)
    suspend();

  return result;
}

void VM::push(Obj *obj) {
//  layoutObjects.push_back(new LayoutObject(obj));
//  current2 = obj;
}

InterpretResult VM::interpret(ObjClosure *closure) {
  current->call(closure, current->savedStackTop - current->fields - 1);
  return run();
}

CallFrame *VM::getFrame(int index) {
  return current->getFrame(index);
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
bool VM::recalculate() {
  CoThread *coThread = ::current;

  totalSize = coThread->repaint();
  ::current = coThread;
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

bool VM::onEvent(Event event, Point pos) {
  CoThread *coThread = ::current;

  coThread->onEvent(event, pos, totalSize);
  ::current = coThread;
  return true;
}

