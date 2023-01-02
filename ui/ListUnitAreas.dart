/*
 *    Copyright (C) 2020 Hocus Codus Software inc. All rights reserved.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */

import 'dart:collection';

class ListUnitAreas {
	int getSize(int dir) {
		return 0;
	}
}

class UnitArea extends ListUnitAreas {
	List<int> unitArea;

	UnitArea(List<int> unitArea) {
		this.unitArea = unitArea;
	}

	int getSize(int dir) {
		return unitArea[dir];
	}
}

class LayoutUnitArea extends ListUnitAreas {
	List<IntTreeUnit> trees;
	List unitAreas;

	LayoutUnitArea(List<IntTreeUnit> trees, List unitAreas) {
		this.trees = trees;
		this.unitAreas = unitAreas;
	}

	int getSize(int dir) {
		return trees[dir].value;
	}
}

class Stack<T> {
	final _stack = Queue<T>();

	void push(T element) {
		_stack.addLast(element);
	}

	T pop() {
		final T lastElement = _stack.last;
		_stack.removeLast();
		return lastElement;
	}

	T getValue() {
		return _stack.last;
	}

	void clear() {
		_stack.clear();
	}

	bool get isEmpty => _stack.isEmpty;
}

class ValueStack {
	HashMap<String, Stack<Object>> map = new HashMap<String, Stack<Object>>();

	void push(String key, Object value) {
		if (!map.containsKey(key))
			map[key] = new Stack<Object>();

		map[key].push(value);
	}

	Object pop(String key) {
		var rc = map[key].pop();

		if (map[key].isEmpty)
			map.remove(key);

		return rc;
	}

	Object get(String key) {
		return map[key].getValue();
	}
}

class IntTreeUnit {
	List<IntTreeUnit> set;
	int value;

	IntTreeUnit(int value) {
		set = List.filled(0, null, growable: true);
		this.value = value;
	}

	IntTreeUnit.fromList(int value, List<IntTreeUnit> children) {
		set = children;
		this.value = value;
	}

	void add(IntTreeUnit unit) {
		set.add(unit);
	}

	int getNumChildren() {
		return set != null ? set.length : 0;
	}
}
