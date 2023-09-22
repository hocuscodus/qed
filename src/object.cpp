/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include <cstdarg>
#include <iostream>

#include "memory.h"
#include "compiler.hpp"

bool Type::equals(Type &type) {
  return valueType == type.valueType && (valueType != VAL_OBJ || objType->equals(type.objType));
}

const char *Type::toString() {
  switch (valueType) {
    case VAL_UNKNOWN: return "!unknown!";
    case VAL_VOID: return "void";
    case VAL_BOOL: return "bool";
    case VAL_INT: return "int";
    case VAL_FLOAT: return "float";
    case VAL_OBJ: return objType ? objType->toString() : "!weird!";
    default: return "!weird2!";
  }
}

Value *stackTop;

bool eventFlag;

Obj *objects = NULL;

Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);

  object->type = type;
  object->next = objects;
  objects = object;
  return object;
}

bool Obj::equals(Obj *obj) {
  return obj && type == obj->type;
}

const char *Obj::toString() {
  static char buf[256];

  switch (type) {
    case OBJ_STRING: return "String";
    case OBJ_ARRAY: {
      char buf2[256] = "??";

      sprintf(buf2, "%s[]", ((ObjArray *) this)->elementType.toString());
      strcpy(buf, buf2);
      return buf;
    }
    case OBJ_FUNCTION: {
      Token name = ((ObjFunction *) this)->expr->_declaration.name;

      sprintf(buf, "%.*s", name.length, name.start);
      return buf;
    }
    case OBJ_INSTANCE:
      ((ObjInstance *) this)->callable->obj.toString();
      strcat(buf, "*");
      return buf;
    default:
      return "!unknownObj!";
  }
}

Declaration::Declaration() {
  expr = NULL;
  isInternalField = false;
  next = NULL;
  peer = NULL;
  parentFlag = false;
}

std::string Declaration::getRealName() {
  Token &name = getDeclaration(expr)->name;

  return peer ? peer->getRealName() + (parentFlag ? "$" : "$_") : std::string(name.start, name.length);
}

bool Declaration::isInRegularFunction() {
  return function->body->name.type == TOKEN_LEFT_BRACE;
}

bool Declaration::isExternalField() {
  return function && isClass(function) && isInRegularFunction() && !getDeclaration(expr)->name.isInternal();
}

bool Declaration::isField() {
  return isExternalField() || isInternalField;
}

ObjFunction::ObjFunction() {
  obj.type = OBJ_FUNCTION;
  instanceIndexes = new IndexList();
  eventFlags = 0L;
  uiFunction = NULL;
}

bool ObjFunction::isClass() {
  return expr->_declaration.name.isClass();
}

bool ObjFunction::isUserClass() {
  return expr->_declaration.name.isUserClass();
}

std::string ObjFunction::getThisVariableName() {
  return strcmp(expr->_declaration.name.start, "Main") ? std::string(expr->_declaration.name.start, expr->_declaration.name.length) + "$this" : "globalThis$";
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

LayoutUnitArea *ObjFunction::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, Value *values, IndexList *instanceIndexes, int dimFlags) {
  LocationUnit **subAreas = new LocationUnit *[attrSets->numAreas];

  memset(subAreas, 0, attrSets->numAreas * sizeof(LocationUnit *));
  attrSets->parseCreateAreasTree(vm, valueStack, 0, Path(), values, instanceIndexes, subAreas);

  DirType<IntTreeUnit *> sizeTrees;

  for (int dir = 0; dir < NUM_DIRS; dir++)
    sizeTrees[dir] = parseResize(dir, {0, 0}, *subAreas, 0);

  return NULL;//new LayoutUnitArea(sizeTrees, subAreas);
}

IntTreeUnit *ObjFunction::parseResize(int dir, const Point &limits, LocationUnit *areas, int offset) {
  return topSizers[dir]->parseResize(&areas, Path(limits.size()), dir, limits);
}

LocationUnit *CallFrame::init(VM &vm, Value *values, IndexList *instanceIndexes, ValueStack<Value *> &valueStack) {
  if (closure->function->native->type == VAL_OBJ) {
//    obj.valueTree = new ValueTree();
//    closure->function->parseCreateUIValues(vm, obj, 0, [], obj.valueTree);

    Point size;

    resize(size);

    for (int index = 0; index < NUM_DIRS; index++)
      totalSize[index] = std::max(size[index], totalSize[index]);

  }

  if (closure->function->attrSets != NULL)
    return closure->function->parseCreateAreasTree(vm, valueStack, values, instanceIndexes, 0);
  else
    return NULL;
}
*/
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
ObjInstance *newInstance(ObjCallable *callable) {
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);

  instance->callable = callable;
  return instance;
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

ObjArray *newArray(Type elementType) {
  ObjArray *array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
  array->elementType = elementType;
  return array;
}

static void printFunction(ObjFunction *function) {
  printf("<fn %s>", function->expr->_declaration.name.start);
}

void printObject(Value value) {
  switch (GET_OBJ_TYPE(value)) {
  case OBJ_INSTANCE: {
    ObjString *name = NULL;//AS_INSTANCE(value)->callable->name;

    printf("<%.*s instance>", name->length, name->chars);
    break;
  }
  case OBJ_FUNCTION:
    printFunction(AS_FUNCTION(value));
    break;
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case OBJ_ARRAY:
    printf("[]");
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