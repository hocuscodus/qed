/*
 *    Copyright (C) 2016 Hocus Codus Software inc.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */

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
