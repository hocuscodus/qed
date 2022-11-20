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