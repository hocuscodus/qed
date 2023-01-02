/*
 *    Copyright (C) 2016 Hocus Codus Software inc.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */


import 'ListUnitAreas.dart';

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
