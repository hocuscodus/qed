/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_layout_h
#define qed_layout_h

#include "path.hpp"
#include "listunitareas.hpp"

struct Obj;
struct VM;

struct LayoutObject {
	Obj *obj;
	ListUnitAreas unitAreas;
	void *areas;

	LayoutObject(Obj *obj);

	void init(VM &process);
	void refresh(VM &process, const Point &pos, const Point &size, const Point &clipPos, const Point &clipSize);
	Path locateEvent(VM &process, const Point &pos, const Point &size, const Point &location, Path currentFocusPath);
	const Point &getTotalSize();
};

struct LayoutContext {
	int pos;
	int size;
	IntTreeUnit treeUnit;

	LayoutContext(int pos, int size, IntTreeUnit treeUnit);

  LayoutContext clone();
	int adjustPos(int index);
	int findIndex(int location);
};

#endif