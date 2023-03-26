/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include "object.hpp"
#include "layout.hpp"

LayoutObject::LayoutObject(Obj *obj) {
  this->obj = obj;
}

void LayoutObject::init(VM &vm) {
  if (obj->type == VAL_OBJ) {
//    obj->valueTree = new ValueTree();
//    obj->func.parseCreateUIValues(vm, obj, 0, [], obj->valueTree);
  }
/*
  if (obj->func.attrSets != null) {
    areas = List.filled(1, null);
    obj->func.parseCreateAreasTree(vm, new ValueStack(), obj, 0, new Path(), areas, 0);
  }*/
}
#if 0
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

class LayoutContext {
	int pos;
	int size;
	IntTreeUnit treeUnit;

	LayoutContext(int pos, int size, IntTreeUnit treeUnit) {
		this.pos = pos;
		this.size = size;
		this.treeUnit = treeUnit;
	}

  LayoutContext clone() {
		return new LayoutContext(pos, size, treeUnit);
	}

	int adjustPos(int index) {
		int offset = index != 0 ? treeUnit.set[index - 1].value : 0;

		pos += offset;
		treeUnit = treeUnit.set[index];
		return offset;
	}

	int findIndex(int location) {
		bool startFlag = true;
		bool endFlag = false;
		int length = treeUnit.set != null ? treeUnit.set.length : 0; // limit initialized to length (we may add one)
		int limit = length;

		if (limit == 0)
			return endFlag ? -1 : 0;

		int offset = location;

		if (endFlag && offset < 0)
			return -1;

		if (startFlag && offset >= treeUnit.set[limit - 1].value)
			return endFlag ? -1 : limit;

		int comp = 1;
		int start = 0;

		while (limit != 0 && comp != 0) {
			int remainder = limit & 1;

			limit >>= 1;
			comp = offset - treeUnit.set[start + limit].value;

			if (comp > 0) {
				start += limit + 1;

				if (remainder == 0)
					limit--;
			}
		}

		return start + limit;
	}
}
#endif
