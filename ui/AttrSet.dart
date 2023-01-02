/*
 *    Copyright (C) 2016 Hocus Codus Software inc. All rights reserved.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */

import 'dart:collection';
import 'dart:math';

import '../lang/Obj.dart';
import '../lang/Path.dart';
import '../lang/QEDProcess.dart';
import '../lang/declaration/LambdaDeclaration.dart';
import '../lang/declaration/FunctionDeclaration.dart';
import '../lang/declaration/ImplicitArrayDeclaration.dart';
import '../lang/op/Op.dart';
import '../lang/Util.dart';
import 'ListUnitAreas.dart';
import 'LayoutContext.dart';
import 'Sizer.dart';
import 'ValueTree.dart';

List<String> keys = ["out"];

class AttrSet {
	HashMap<String, Attr> attrs = new HashMap<String, Attr>();
	ChildAttrSets children = null;
	HashSet<String> flagsMap = HashSet<String>();
	int parseFlags;
	int refreshFlags;
	int areaParseFlags;
	int maxDirFlags;
	List<int> addDirs = List.filled(numDirs, 0);
	List<int> maxDirs = List.filled(numDirs, 0);
	List<int> zFlags = List.filled(numDirs, 0);
	List<Path> posPaths = List.filled(numDirs, null);
//	bool lValueFlag = false;
	int offset;

	AttrSet(List<int> offset, List<int> zoneOffsets, List<int> arrayDirFlags, ValueStack valueStack, FunctionDeclaration mainFunc, LambdaDeclaration func, FakeStream inputStream, int parentRefreshFlags) {
		/*arrayDirs = */inputStream.read();
		int childDir = inputStream.read();
		int numAttrs = inputStream.read();
		var str = "";

		refreshFlags = 0;

		if (numAttrs != 0) {
			for (var index = 0; index < numAttrs; index++) {
				String prop = LambdaDeclaration.readString(inputStream);
				LambdaDeclaration handler = func.getSubFunction(inputStream.read());
				int dimFlags = inputStream.read();
				LambdaDeclaration returnType = inputStream.read() != 0 ? LambdaDeclaration.evalStream(mainFunc, inputStream) : null;

				str = str + " @" + prop + "(" + handler.name + ")";
				initAttr(prop, handler, dimFlags, returnType);
				refreshFlags |= dimFlags; // should not take into account useless attributes (make it two-pass in compiler)

				if (keys.contains(prop))
					valueStack.push(prop, [dimFlags, false]);
			}

			refreshFlags ^= refreshFlags & parentRefreshFlags; // remove parentRefreshFlags part
		}

		int parseFlags2 = 0;
		int numAttrSets = inputStream.read();

		if (numAttrSets != 0) {
			children = new ChildAttrSets(offset, zoneOffsets, childDir, arrayDirFlags, valueStack, numAttrSets, mainFunc, func, inputStream, refreshFlags | parentRefreshFlags);
			parseFlags = children.parseFlags;
			areaParseFlags = children.areaParseFlags;
		}
		else {
			parseFlags = 0;
			areaParseFlags = 0;
			this.offset = offset[0]++;

			valueStack.map.forEach((key, value) {
				parseFlags2 |= (value.getValue() as List)[0];
				areaParseFlags |= 1 << (value.getValue() as List)[0];
				(value.getValue() as List)[1] = true;
			});
		}

		attrs.forEach((prop, value) {
			if (keys.contains(prop) && !(valueStack.pop(prop) as List)[1])
				attrs.remove(prop); // remove unnecessary attribute (shadowed by descendants)
			else {
				if (keys.contains(prop))
					flagsMap.add(prop);

				parseFlags |= 1 << value.flags;
			}
		});

		maxDirFlags = children != null ? 0 : refreshFlags | parentRefreshFlags;

		for (int dir = 0; dir < numDirs; dir++) {
			addDirs[dir] = arrayDirFlags[dir] & parseFlags2;
			maxDirs[dir] = addDirs[dir] ^ parseFlags2;
			zFlags[dir] = children != null ? children.zFlags[dir] : 1 << addDirs[dir];
			maxDirFlags &= ~addDirs[dir];
		}

		print("restore " + str);
	}

	void parseCreateSizers(List<Sizer> topSizers, int dir, List<int> zone, Path path, Path flagsPath) {
		if (children != null) {
			SizerType sizerType = children.getSizerType(dir, zone);

			children.parseCreateSizers(topSizers, dir, zone, path, flagsPath.concatInt(sizerType.index));
		}
		else {
			if (zone != null)// && !maxZoneFlag)
				zone[0] += ((zone[0] & 1) != 0) != (addDirs[dir] != 0) ? 1 : 0;

			posPaths[dir] = topSizers[dir].add(zone != null ? zone[0] : -2, addDirs[dir], path, flagsPath.concatInt(SizerType.getter.index), offset, maxDirs[dir]);
		}
	}

	void parseAdjustPaths(List<Sizer> topSizers, int dir) {
		if (children != null)
			children.parseAdjustPaths(topSizers, dir);
		else
			topSizers[dir].adjust(posPaths[dir]);
	}

	void initAttr(String property, LambdaDeclaration handler, int flags, LambdaDeclaration returnType) {
		attrs[property] = new Attr(handler, flags, returnType);
	}

	int getNumAreas() {
		return children != null ? children.numAreas : 1;
	}

	int getZones(List<int> zoneOffset, int dir, int childDir, List<int> arrayDirFlags) {
		if (children != null)
			return children.zones[dir];
		else {
			int ndx = 0;
			int dims = areaParseFlags;
			int zones = 0;

			while (dims != 0) {
				if ((dims & 1) != 0) {
					int arrayDir = (arrayDirFlags[dir] >> ndx) & (childDir >> dir); //((1 << mainNumDim) - 1);
					bool arrayDirFlag = (arrayDir & 1) != 0;

					if (arrayDirFlag ^ (zoneOffset[dir] == -1 || (zoneOffset[dir] & 1) == 0)) {
						zones = 1 << ++zoneOffset[dir];
						break;
					}
				}

				ndx++;
				dims >>= 1;
			}

			return zones;
		}
	}

	bool isEditable(VM &vm, Obj obj, Path path) {
		bool editable = false;//execCmdRaw(process, obj, MODIFeditable, null);

		return editable != null;// ? editable : getcontent(MODIFoninput) != null;
	}

	bool getEditableFlag(VM &vm, Obj obj, Path path) {
		return false;//isEditable(process, obj, path) && lValueFlag;
	}

	int getNumChildren() {
		return children != null ? children.length : 0;
	}

	void parseCreateUIValues(VM &vm, Object value, int flags, List<int> path, Obj subValue, ValueTree valueTree) {
			attrs.forEach((k, v) {
				if (v.flags == flags && !(k.startsWith("on") && k.length > 2 && k[2] == k[2].toUpperCase())) {
					Object childValue = (value as Obj).func.parseCreateUIValuesSub(process, value, null, v.flags, v.handler);

					valueTree.addAttr(k, path, childValue);
				}
			});

		if (children != null)
			children.parseCreateUIValues(process, value, subValue, flags, path, valueTree);
	}

	void parseCreateAreasTree(VM &vm, ValueStack valueStack, int dimFlags, Path path, ValueTree valueTree, List areas) {
		flagsMap.forEach((key) {
			int flags = attrs[key].flags;

			if ((flags & dimFlags) == flags) {
				Path filteredPath = path.filterPathWrong(dimFlags, flags);
				Object value = filteredPath.getValue(valueTree.getValue(key));

				valueStack.push(key, value);
			}
		});

		if (children != null)
			children.parseCreateAreasTree(process, valueStack, dimFlags, path, valueTree, areas);
		else {
			Object value = valueStack.get("out");
			LambdaDeclaration func = Op.getPredefinedType(value);

			func.parseCreateAreasTree(process, valueStack, value, dimFlags, path, areas, offset);
		}

		flagsMap.forEach((key) {
			int flags = attrs[key].flags;

			if ((flags & dimFlags) == flags)
				valueStack.pop(key);
		});
	}

	void parseRefresh(VM &vm, ValueStack valueStack, List<int> limits, Path path, ValueTree valueTree, List areas, List<Sizer> topSizers, List<LayoutContext> layoutContexts) {
		parseRefresh2(process, valueStack, refreshFlags, limits, path, valueTree, areas, topSizers, layoutContexts);
	}

	void parseRefresh2(VM &vm, ValueStack valueStack, int domain, List<int> limits, Path path, ValueTree valueTree, List areas, List<Sizer> topSizers, List<LayoutContext> layoutContexts) {
		var dom = [domain];
		int dim = ctz(dom);

		if (dim != -1)
			for (int index = 0; index < limits[dim]; index++) {
				path.values[dim] = index;
				parseRefresh2(process, valueStack, dom[0], limits, path, valueTree, areas, topSizers, layoutContexts);
			}
		else {
			flagsMap.forEach((key) {
				int flags = attrs[key].flags;
				Path filteredPath = path.filter(flags);
				Object value = filteredPath.getValue(valueTree.getValue(key));

				valueStack.push(key, value);
			});

			if (children != null)
				children.parseRefresh(process, valueStack, limits, path, valueTree, areas, topSizers, layoutContexts);
			else {
				Object value = valueStack.get("out");
				LambdaDeclaration func = Op.getPredefinedType(value);
				List<int> unitPos = List.filled(numDirs, 0);

				for (int dir = 0; dir < numDirs; dir++) {
					LayoutContext posLayoutContext = layoutContexts[dir].clone();

					topSizers[dir].adjustPos(posPaths[dir], path, posLayoutContext);
					unitPos[dir] = posLayoutContext.pos;
				}

				func.parseRefresh(process, valueStack, value, path.filter(ctz([areaParseFlags])).getValue(areas[offset]), unitPos);
			}

			flagsMap.forEach((key) {
				valueStack.pop(key);
			});
		}
	}

	Path parseLocateEvent(VM &vm, Obj obj, List<int> limits, Path path, ValueTree valueTree, Object subObj, List areas, IntTreeUnit subTree, List<Sizer> sizers, List<LayoutContext> layoutContexts, List<int> location, Path currentFocusPath) {
		return parseLocateEvent2(process, obj, maxDirFlags, limits, path, valueTree, subObj, areas, subTree, sizers, layoutContexts, location, currentFocusPath);
	}

	Path parseLocateEvent2(VM &vm, Obj obj, int domain, List<int> limits, Path path, ValueTree valueTree, Object subObj, List areas, IntTreeUnit subTree, List<Sizer> sizers, List<LayoutContext> layoutContexts, List<int> location, Path currentFocusPath) {
		var dom = [domain];
		int dim = ctz(dom);
		Path focusPath = null;
		bool currentFocusPathFlag = currentFocusPath != null;

		if (dim != -1) {
			int srcIndex = currentFocusPathFlag ? currentFocusPath.get(0) : -1;

			for (int index = limits[dim] - 1; focusPath == null && index >= 0; index--) {
				Path subCurrentFocusPath = currentFocusPathFlag && srcIndex == index ? currentFocusPath.trimRange(0, 1) : null;

				if (currentFocusPathFlag && srcIndex != index) {
					path.values[dim] = srcIndex;
					(obj.func as ImplicitArrayDeclaration).setIndex(obj, dim, srcIndex);
					parseOut2(process, obj, dom[0], currentFocusPath.trimRange(0, 1), path, valueTree, subObj);
				}

				path.values[dim] = index;
				(obj.func as ImplicitArrayDeclaration).setIndex(obj, dim, index);
				focusPath = Path.attach(index, parseLocateEvent2(process, obj, dom[0], limits, path, valueTree, subObj, areas, subTree, sizers, layoutContexts, location, subCurrentFocusPath));
			}
		}
		else {
			Object newSubObj = subObj;

			if (attrs.containsKey("out")) {
				int flags = attrs["out"].flags;
				Path filteredPath = path.filter(flags);

				newSubObj = filteredPath.getValue(valueTree.getValue("out"));
			}

			if (children != null)
				focusPath = children.parseLocateEvent(process, obj, limits, path, valueTree, newSubObj, areas, subTree, sizers, layoutContexts, location, currentFocusPath);
			else {
				LambdaDeclaration func = Op.getPredefinedType(newSubObj);
				List<int> pos = List.filled(numDirs, 0);
				List<int> size = List.filled(numDirs, 0);

				for (int dir = 0; dir < numDirs; dir++) {
					pos[dir] = layoutContexts[dir].pos;
					size[dir] = layoutContexts[dir].size;
				}

				if (func is PredefinedDeclaration) {
					process.currentAttrSet = this;
					process.currentObj = obj.func.getRealObj(obj, maxDirFlags);
					focusPath = new Path();//!(childValue is bool) || childValue;
				}
				else
					focusPath = func.parseLocateEvent(process, newSubObj, path.filter(ctz([areaParseFlags])).getValue(areas[offset]), pos, size, location, currentFocusPath);
			}
		}

		return focusPath;
	}

	void parseOut(VM &vm, Obj obj, Path focusPath, Path path, ValueTree valueTree, Object subObj) {
		parseOut2(process, obj, maxDirFlags, focusPath, path, valueTree, subObj);
	}

	void parseOut2(VM &vm, Obj obj, int domain, Path focusPath, Path path, ValueTree valueTree, Object subObj) {
		var dom = [domain];
		int dim = ctz(dom);

		if (dim != -1) {
			int index = focusPath.get(0);

			path.values[dim] = index;
			(obj.func as ImplicitArrayDeclaration).setIndex(obj, dim, index);
			parseOut2(process, obj, dom[0], focusPath.trimRange(0, 1), path, valueTree, subObj);
		}
		else {
			Object newSubObj = subObj;

			if (attrs.containsKey("out")) {
				int flags = attrs["out"].flags;
				Path filteredPath = path.filter(flags);

				newSubObj = filteredPath.getValue(valueTree.getValue("out"));
			}

			if (children != null)
				children.parseOut(process, obj, focusPath, path, valueTree, newSubObj);
			else {
				LambdaDeclaration func = Op.getPredefinedType(newSubObj);

				if (func is PredefinedDeclaration) {
					Obj currentObj = obj.func.getRealObj(obj, maxDirFlags);

					execcmd2(process, currentObj, "onHoverOut", null);
				}
				else
					func.out(process, newSubObj, focusPath);
			}
		}
	}

	void parseSetFocus(VM &vm, Obj obj, Path path) {
		if (children != null)
			children.parseSetFocus(process, obj, path);
		else {
			List value = execcmd2(process, obj, "onInput", null);

			process.inputAttrSet = this;
			process.inputValues = value[0];
			process.inputIndex = value[1];
		}
	}

	Object execcmd2(VM &vm, Obj obj, String modifier, List args) {
		Attr attr = attrs[modifier];

		return attr != null ? process.execCmdRaw(obj, attr.handler, args) : null;
	}

	bool hasContent(String modifier) {
		return attrs.containsKey(modifier);
	}

	Path parseFindId(VM &vm, String id) {
		String value = execcmd2(process, null, "id", null);

		return value != null && value == id ? new Path() : children != null ? children.parseFindId(process, id) : null;
	}
/*
	String toString() {
		String str = ("L:" + (contents != null ? 1.toString() : 0.toString())) + " ";

		if (contents != null)
			for (var index = 0; index < contents.length; index++)
				if (contents[index] != null)
					str = (str.length == 0 ? "" : str + " ") + contents[index].name;

		return str;
	}*/
}

class ChildAttrSets extends ListBase<AttrSet> {
	final List<AttrSet> _l = [];
	int childDir;
	int childrenFlags = 0;
	int parseFlags = 0;
	int areaParseFlags;
	int numAreas;
	List<int> zones = List.filled(numDirs, 0);
	List<int> flags = List.filled(numDirs, 0);
	List<int> zFlags = List.filled(numDirs, 0);

	void set length(int newLength) { _l.length = newLength; }
	int get length => _l.length;
	AttrSet operator [](int index) => _l[index];
	void operator []=(int index, AttrSet value) { _l[index] = value; }

	ChildAttrSets(List<int> offset, List<int> zoneOffsets, int childDir, List<int> arrayDirFlags, ValueStack valueStack, int numAttrSets, FunctionDeclaration mainFunc, LambdaDeclaration func, FakeStream inputStream, int parentRefreshFlags) : super() {
		List<int> maxZoneOffsets = List.filled(numDirs, 0);
		List<bool> zBoolFlags = List.filled(numDirs, true);

		this.childDir = childDir;
		parseFlags = 0;
		areaParseFlags = 0;
		numAreas = 0;

		for (int index = 0; index < numAttrSets; index++) {
			AttrSet attrSet = new AttrSet(offset, zoneOffsets, arrayDirFlags, valueStack, mainFunc, func, inputStream, parentRefreshFlags);

			if (attrSet.areaParseFlags != 0) {
				_l.add(attrSet);
				parseFlags |= attrSet.parseFlags;
				areaParseFlags |= attrSet.areaParseFlags;
				numAreas += attrSet.getNumAreas();

				for (int dir = 0; dir < numDirs; dir++) {
					zBoolFlags[dir] &= attrSet.zFlags[dir] > 0;

					int currentZoneOffset = zoneOffsets[dir];

					zones[dir] |= attrSet.getZones(zoneOffsets, dir, childDir, arrayDirFlags);

					if ((childDir & (1 << dir)) == 0) {
						maxZoneOffsets[dir] = max(zoneOffsets[dir], maxZoneOffsets[dir]);
						zoneOffsets[dir] = currentZoneOffset;
					}
				}
			}
		}

		for (int index = 0; index < _l.length; index++)
			for (int dir = 0; dir < numDirs; dir++) {
				int childZFlags = _l[index].zFlags[dir];

				if ((childDir & (1 << dir)) == 0)
					zFlags[dir] |= zBoolFlags[dir] ? childZFlags : childZFlags > 0 ? childZFlags != 1 ? 2 : 1 : -childZFlags - 1;
				else {
					// accumulate zones
//					int childZones = childZFlags > 0 ? 1 << getLargestDim(childZFlags) : -childZFlags - 1;
					int childZones = childZFlags > 0 ? childZFlags != 1 ? 2 : 1 : -childZFlags - 1;
					int firstChildZone = ctz([childZones]);
					int maxZone = zFlags[dir] > 0 ? getLargestDim(zFlags[dir]) : firstChildZone;

					zFlags[dir] |= childZones << (maxZone + (firstChildZone & 1 != maxZone & 1 ? 1 : 0));
				}
			}

		for (int dir = 0; dir < numDirs; dir++) {
			if (!zBoolFlags[dir])
				zFlags[dir] = -zFlags[dir] - 1;

			if ((childDir & (1 << dir)) == 0)
				zoneOffsets[dir] = maxZoneOffsets[dir];
		}
	}

	SizerType getSizerType(int dir, List<int> zone) {
		return (childDir & (1 << dir)) != 0 ? SizerType.adder : /*zone[0] & 1 != 0 ? SizerType.zoneMaxer : */SizerType.maxer;
	}

	void parseCreateSizers(List<Sizer> topSizers, int dir, List<int> zone, Path path, Path flagsPath) {
		bool maxZoneFlag = zone != null && (childDir & (1 << dir)) == 0;
		int maxZone = maxZoneFlag ? zone[0] : 0;

		for (int index = 0; index < _l.length; index++) {
			int currentZoneOffset = maxZoneFlag ? zone[0] : 0;

			_l[index].parseCreateSizers(topSizers, dir, zone, path.concatInt(index), flagsPath);

			if (maxZoneFlag) {
				maxZone = max(zone[0], maxZone);
				zone[0] = currentZoneOffset;
			}
		}

		if (maxZoneFlag)
			zone[0] = maxZone;
	}

	void parseAdjustPaths(List<Sizer> topSizers, int dir) {
		for (int index = 0; index < _l.length; index++)
			_l[index].parseAdjustPaths(topSizers, dir);
	}

	void parseCreateAreasTree(VM &vm, ValueStack valueStack, int dimFlags, Path path, ValueTree valueTree, List areas) {
		for (int index = 0; index < length; index++) {
			AttrSet attrSet = _l[index];

			if ((attrSet.areaParseFlags & (1 << dimFlags)) != 0)
				attrSet.parseCreateAreasTree(process, valueStack, dimFlags, path, valueTree.getValueTree(index), areas);
		}
	}

	void parseRefresh(VM &vm, ValueStack valueStack, List<int> limits, Path path, ValueTree valueTree, List areas, List<Sizer> topSizers, List<LayoutContext> layoutContexts) {
		for (int index = 0; index < length; index++) {
			AttrSet attrSet = _l[index];

			attrSet.parseRefresh(process, valueStack, limits, path, valueTree.getValueTree(index), areas, topSizers, layoutContexts);
		}
	}

	Path parseLocateEvent(VM &vm, Obj obj, List<int> limits, Path path, ValueTree valueTree, Object subObj, List areas, IntTreeUnit subTree, List<Sizer> sizers, List<LayoutContext> layoutContexts, List<int> location, Path currentFocusPath) {
		int setBits = -1;
		Path focusPath = null;
		bool currentFocusPathFlag = currentFocusPath != null;
		int srcIndex = currentFocusPathFlag ? currentFocusPath.get(0) : -1;

		for (int dir = 0; dir < numDirs; dir++)
			setBits &= sizers[dir].findIndexes(location[dir], layoutContexts[dir].treeUnit);

		for (int ndx = subTree.getNumChildren() - 1; focusPath == null && setBits != 0 && ndx >= 0; ndx--) {
			int index = subTree.set[ndx].value;

			if (setBits & (1 << index) != 0) {
				AttrSet attrSet = _l[index];
				List<Sizer> subSizers = List.filled(numDirs, null);
				List<LayoutContext> subLayoutContexts = List.filled(numDirs, null);
				var subLocation = new List<int>.from(location);

				if (currentFocusPathFlag && srcIndex != index)
					_l[srcIndex].parseOut(process, obj, currentFocusPath.trimRange(0, 1), path, valueTree.getValueTree(srcIndex), subObj);

				for (int dir = 0; dir < numDirs; dir++) {
					subLayoutContexts[dir] = layoutContexts[dir].clone();
					subSizers[dir] = sizers[dir].findSizer(index, sizers[dir].children[0].index/*attrSet.addDirs[dir]*/, obj, path, subLocation, dir, subLayoutContexts[dir]);
				}

				Path subCurrentFocusPath = currentFocusPathFlag && srcIndex == index ? currentFocusPath.trimRange(0, 1) : null;

				focusPath = Path.attach(index, attrSet.parseLocateEvent(process, obj, limits, path, valueTree.getValueTree(index), subObj, areas, subTree.set[ndx], subSizers, subLayoutContexts, subLocation, subCurrentFocusPath));
				setBits &= ~(1 << index);
			}
		}

		return focusPath;
	}

	void parseOut(VM &vm, Obj obj, Path focusPath, Path path, ValueTree valueTree, Object subObj) {
		int index = focusPath.get(0);
		AttrSet attrSet = _l[index];

		attrSet.parseOut(process, obj, focusPath.trimRange(0, 1), path, valueTree.getValueTree(index), subObj);
	}

	void parseCreateUIValues(VM &vm, Object value, Obj subValue, int flags, List<int> dimIndexes, ValueTree valueTree) {
		if ((parseFlags & (1 << flags)) != 0)
			for (int index = 0; index < length; index++) {
				ValueTree tree = valueTree.getValueTree(index);

				if (tree == null)
					tree = new ValueTree();

				_l[index].parseCreateUIValues(process, value, flags, dimIndexes, subValue, tree);

				if (!tree.isEmpty())
					valueTree.putValueTree(index, tree);
			}
	}

	Path parseFindId(VM &vm, String id) {
		Path path = null;

		for (int index = 0; path == null && index < length; index++) {
			path = _l[index].parseFindId(process, id);

			if (path != null)
				path = new Path.fromValue(index).concat(path);
		}

		return path;
	}

	void parseSetFocus(VM &vm, Obj obj, Path path) {
		_l[path.get(0)].parseSetFocus(process, obj, path.trimRange(0, 1));
	}

	String toString() {
		StringBuffer string = new StringBuffer();

		for (int index = 0; index < length; index++)
			string.writeln(_l[index]);

		return string.toString();
	}
}

class Attr {
	LambdaDeclaration handler;
	int flags;
	LambdaDeclaration returnType;

	Attr(LambdaDeclaration handler, int flags, LambdaDeclaration returnType) {
		this.handler = handler;
		this.flags = flags;
		this.returnType = returnType;
	}
}
