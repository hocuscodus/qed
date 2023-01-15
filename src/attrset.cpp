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
#include <string.h>
#include "attrset.hpp"

long ctz0(long *n) {
  if (n[0] == 0)
    return 0;

  long bit = n[0] & -n[0];

  n[0] ^= bit;
  return bit;
}

int ctz(long *n) {
  long bit = ctz0(n);
  int num = 0;

  if (bit == 0)
    return -1;

  if ((bit & 0x00000000FFFFFFFF) == 0) {
    num = 32;
    bit >>= 32;
  }

  if ((bit & 0x000000000000FFFF) == 0) {
    num += 16;
    bit >>= 16;
  }

  if ((bit & 0x00000000000000FF) == 0) {
    num += 8;
    bit >>= 8;
  }

  if ((bit & 0x000000000000000F) == 0) {
    num += 4;
    bit >>= 4;
  }

  if ((bit & 0x0000000000000003) == 0) {
    num += 2;
    bit >>= 2;
  }

  if ((bit & 0x0000000000000001) == 0)
    num++;

  return num;
}

long getLargestDim(long flags) {
  long largestDim = 0;

  for (long dimIndex = ctz(&flags); dimIndex != -1; dimIndex = ctz(&flags))
    largestDim = dimIndex;
//		largestDim |= dimIndex;

  return largestDim;
}

std::unordered_set<std::string> keys;

static void keys_fn();
static int keys_var __attribute((unused)) = (keys_fn(), 0);
static void keys_fn() {
  keys.insert("out");
}

Attr::Attr(int flags, void *returnType, int localIndex) {
  this->flags = flags;
  this->returnType = returnType;
  this->localIndex = localIndex;
}
/*
AttrSet::AttrSet(int *offset, Point &zoneOffsets, std::array<long, NUM_DIRS> &arrayDirFlags, ValueStack<ValueStackElement> &valueStack, UIDirectiveExpr *listExpr, int parentRefreshFlags, ObjFunction *function) {
//  / *arrayDirs = * /inputStream.read();
  int childDir = 0;//inputStream.read();
  int numAttrs = listExpr->attCount;
  std::string str = "";

  refreshFlags = 0;

  if (numAttrs != 0) {
    for (int index = 0; index < numAttrs; index++) {
      Token prop = listExpr->attributes[index]->name;
      std::string property = prop.getString();
      int dimFlags = 0;//inputStream.read();
      ExprType *returnType = NULL;//inputStream.read() != 0 ? listExpr->attributes[index]->type : NULL;

//      str = str + " @" + prop + "(" + handler->name + ")";
      initAttr(property, dimFlags, returnType, listExpr->attributes[index]->_index);
      refreshFlags |= dimFlags; // should not take into account useless attributes (make it two-pass in compiler)

      if (keys.find(property) != keys.end())
        valueStack.push(property, {dimFlags, false, listExpr->attributes[index]->_index});
    }

    refreshFlags ^= refreshFlags & parentRefreshFlags; // remove parentRefreshFlags part
  }

  int parseFlags2 = 0;
  int numAttrSets = listExpr->childrenCount;

  if (numAttrSets != 0) {
    children = new ChildAttrSets(offset, zoneOffsets, childDir, arrayDirFlags, valueStack, listExpr, refreshFlags | parentRefreshFlags, function);
    parseFlags = children->parseFlags;
    areaParseFlags = children->areaParseFlags;
  }
  else {
    parseFlags = 0;
    areaParseFlags = 0;
    this->offset = offset[0]++;

    for(auto& it: valueStack.map) {
      ValueStackElement &element = it.second.top();
      Type &type = function->fields[element.localIndex].type;

      parseFlags2 |= element.parseFlags;
      areaParseFlags |= 1 << element.parseFlags;
      element.b = true;

      if (type.valueType == VAL_OBJ && type.objType->type == OBJ_COMPILER_INSTANCE)
        function->instanceIndexes->set(element.localIndex);
    }
  }

  for (auto it = attrs.begin(); it != attrs.end();) {
    bool propFound = keys.find(it->first) != keys.end();

    if (propFound && !valueStack.pop(it->first).b)
      it = attrs.erase(it); // remove unnecessary attribute (shadowed by descendants)
    else {
      if (propFound)
        flagsMap.insert(it->first);

      parseFlags |= 1 << it->second->flags;
      it++;
    }
  }

  maxDirFlags = children != NULL ? 0 : refreshFlags | parentRefreshFlags;

  for (int dir = 0; dir < NUM_DIRS; dir++) {
    addDirs[dir] = arrayDirFlags[dir] & parseFlags2;
    maxDirs[dir] = addDirs[dir] ^ parseFlags2;
    zFlags[dir] = children != NULL ? children->zFlags[dir] : 1 << addDirs[dir];
    maxDirFlags &= ~addDirs[dir];
  }

//  print("restore " + str);
}
*/
void AttrSet::initAttr(std::string property, int flags, void*returnType, int index) {
  attrs.insert(std::pair<std::string, Attr *>(property, new Attr(flags, returnType, index)));
}

int AttrSet::getZones(Point &zoneOffset, int dir, int childDir, std::array<long, NUM_DIRS> arrayDirFlags) {
  if (children != NULL)
    return children->zones[dir];
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

int AttrSet::getNumAreas() {
  return children != NULL ? children->numAreas : 1;
}

void AttrSet::parseCreateSizers(DirSizers &topSizers, int dir, int *zone, Path path, Path flagsPath) {
  if (children != NULL) {
    SizerType sizerType = children->getSizerType(dir, zone);

    children->parseCreateSizers(topSizers, dir, zone, path, flagsPath.concat(sizerType));
  }
  else {
    if (zone != NULL)// && !maxZoneFlag)
      zone[0] += ((zone[0] & 1) != 0) != (addDirs[dir] != 0) ? 1 : 0;

    posPaths[dir] = topSizers[dir]->add(zone != NULL ? zone[0] : -2, addDirs[dir], path, flagsPath.concat(getter), offset, maxDirs[dir]);
  }
}

void AttrSet::parseAdjustPaths(DirSizers &topSizers, int dir) {
  if (children != NULL)
    children->parseAdjustPaths(topSizers, dir);
  else
    topSizers[dir]->adjust(posPaths[dir]);
}
/*
void AttrSet::parseCreateUIValues(VM &vm, Object value, int flags, List<int> path, Obj subValue, ValueTree valueTree) {
    attrs.forEach((k, v) {
      if (v.flags == flags && !(k.startsWith("on") && k.length > 2 && k[2] == k[2].toUpperCase())) {
        Object childValue = (value as Obj).func.parseCreateUIValuesSub(process, value, null, v.flags, v.handler);

        valueTree.addAttr(k, path, childValue);
      }
    });

  if (children != NULL)
    children->parseCreateUIValues(vm, value, subValue, flags, path, valueTree);
}*/

void AttrSet::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, int dimFlags, const Path &path, Value *values, IndexList *instanceIndexes, LocationUnit **areaUnits) {
  for (std::string key : flagsMap) {
//    int flags = attrs[key]->flags;

//    if ((flags & dimFlags) == flags) {
//      Path filteredPath = path.filterPathWrong(dimFlags, flags);
      int localIndex = attrs[key]->localIndex;
      Value *value = &values[localIndex];//filteredPath.getValue(valueTree.getValue(key));

      valueStack.push(key, value);

      if (!key.compare("out"))
        (dimFlags <<= 1) |= instanceIndexes->get(localIndex);
//    }
  }

  if (children != NULL)
    children->parseCreateAreasTree(vm, valueStack, dimFlags, path, values, instanceIndexes, areaUnits);
  else {
    Value *value = valueStack.get("out");

    if (value != NULL) {
      LocationUnit *unitArea;

      if (dimFlags & 1)
;//        unitArea = AS_INSTANCE(*value)->recalculateLayout();
      else {
//        LambdaDeclaration func = Op.getPredefinedType(value);

        unitArea = new UnitArea({10, 10});
//vm.getStringSize(AS_OBJ(*value), valueStack);
      }

      areaUnits[offset] = LocationUnit::addValue(&areaUnits[offset], path, unitArea);
    }
  }

  for (std::string key : flagsMap) {
//    int flags = attrs[key]->flags;

//    if ((flags & dimFlags) == flags)
      valueStack.pop(key);
  }
}
#if 0
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
#endif/*
ChildAttrSets::ChildAttrSets(int *offset, Point &zoneOffsets, int childDir, std::array<long, NUM_DIRS> &arrayDirFlags, ValueStack<ValueStackElement> &valueStack, UIDirectiveExpr *listExpr, int parentRefreshFlags, ObjFunction *function) : std::vector<AttrSet *>() {
  Point maxZoneOffsets;
  bool zBoolFlags[NUM_DIRS]{true};

  this->childDir = childDir;
  parseFlags = 0;
  areaParseFlags = 0;
  numAreas = 0;

  for (int index = 0; index < listExpr->childrenCount; index++) {
    AttrSet *attrSet = new AttrSet(offset, zoneOffsets, arrayDirFlags, valueStack, listExpr->children[index], parentRefreshFlags, function);

    if (attrSet->areaParseFlags != 0) {
      push_back(attrSet);
      parseFlags |= attrSet->parseFlags;
      areaParseFlags |= attrSet->areaParseFlags;
      numAreas += attrSet->getNumAreas();

      for (int dir = 0; dir < NUM_DIRS; dir++) {
        zBoolFlags[dir] &= attrSet->zFlags[dir] > 0;

        int currentZoneOffset = zoneOffsets[dir];

        zones[dir] |= attrSet->getZones(zoneOffsets, dir, childDir, arrayDirFlags);

        if ((childDir & (1 << dir)) == 0) {
          maxZoneOffsets[dir] = std::max(zoneOffsets[dir], maxZoneOffsets[dir]);
          zoneOffsets[dir] = currentZoneOffset;
        }
      }
    }
  }

  for (int index = 0; index < size(); index++)
    for (int dir = 0; dir < NUM_DIRS; dir++) {
      int childZFlags = operator[](index)->zFlags[dir];

      if ((childDir & (1 << dir)) == 0)
        zFlags[dir] |= zBoolFlags[dir] ? childZFlags : childZFlags > 0 ? childZFlags != 1 ? 2 : 1 : -childZFlags - 1;
      else {
        // accumulate zones
//					int childZones = childZFlags > 0 ? 1 << getLargestDim(childZFlags) : -childZFlags - 1;
        long childZones = childZFlags > 0 ? childZFlags != 1 ? 2 : 1 : -childZFlags - 1;
        long childZonesCopy = childZones;
        long firstChildZone = ctz(&childZonesCopy);
        long maxZone = zFlags[dir] > 0 ? getLargestDim(zFlags[dir]) : firstChildZone;

        zFlags[dir] |= childZones << (maxZone + (firstChildZone & 1 != maxZone & 1 ? 1 : 0));
      }
    }

  for (int dir = 0; dir < NUM_DIRS; dir++) {
    if (!zBoolFlags[dir])
      zFlags[dir] = -zFlags[dir] - 1;

    if ((childDir & (1 << dir)) == 0)
      zoneOffsets[dir] = maxZoneOffsets[dir];
  }
}
*/
SizerType ChildAttrSets::getSizerType(int dir, int *zone) {
  return (childDir & (1 << dir)) != 0 ? SizerType::adder : /*zone[0] & 1 != 0 ? SizerType.zoneMaxer : */SizerType::maxer;
}

void ChildAttrSets::parseCreateSizers(DirSizers &topSizers, int dir, int *zone, Path path, Path flagsPath) {
  bool maxZoneFlag = zone != NULL && (childDir & (1 << dir)) == 0;
  int maxZone = maxZoneFlag ? zone[0] : 0;

  for (int index = 0; index < size(); index++) {
    int currentZoneOffset = maxZoneFlag ? zone[0] : 0;

    operator[](index)->parseCreateSizers(topSizers, dir, zone, path.concat(index), flagsPath);

    if (maxZoneFlag) {
      maxZone = std::max(zone[0], maxZone);
      zone[0] = currentZoneOffset;
    }
  }

  if (maxZoneFlag)
    zone[0] = maxZone;
}

void ChildAttrSets::parseAdjustPaths(DirSizers &topSizers, int dir) {
  for (int index = 0; index < size(); index++)
    operator[](index)->parseAdjustPaths(topSizers, dir);
}
/*
void ChildAttrSets::parseCreateUIValues(VM &vm, Object value, Obj subValue, int flags, List<int> dimIndexes, ValueTree valueTree) {
  if ((parseFlags & (1 << flags)) != 0)
    for (int index = 0; index < size(); index++) {
      ValueTree *tree = valueTree.getValueTree(index);

      if (tree == NULL)
        tree = new ValueTree();

      operator[](index)->parseCreateUIValues(vm, value, flags, dimIndexes, subValue, tree);

      if (!tree->isEmpty())
        valueTree.putValueTree(index, tree);
    }
}
*/
void ChildAttrSets::parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, int dimFlags, const Path &path, Value *values, IndexList *instanceIndexes, LocationUnit **areaUnits) {
  for (int index = 0; index < size(); index++) {
    AttrSet *attrSet = operator[](index);

    if ((attrSet->areaParseFlags & (1 << dimFlags)) != 0)
      attrSet->parseCreateAreasTree(vm, valueStack, dimFlags, path, values, instanceIndexes, areaUnits);
  }
}
#if 0
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
#endif
