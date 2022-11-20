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
#include "listunitareas.hpp"

LocationUnit::~LocationUnit() {
}
/*
  Object addValue(List list, Object value) {
    if (getNumLevels() != 0) {
      if (list == null) list = List.empty(growable: true);

      Object oldValue = get(0) < list.length ? list[get(0)] : null;
      Object newValue = trimRange(0, 1).addValue(oldValue, value);

      if (oldValue == null && get(0) != list.length) oldValue = oldValue;
      if (oldValue != newValue) list.add(newValue);

      return list;
    } else
      return value;
  }
*/
LocationUnit *LocationUnit::addValue(LocationUnit **start, const Path &path, LocationUnit *value) {
  if (path.size() != 0) {
    LocationTree *tree = *start ? (LocationTree *) *start : new LocationTree(1);

    *start = tree;

    LocationTree *oldValue = path[0] < tree->size() ? (LocationTree *) tree->operator[](path[0]) : NULL;
    LocationUnit *newValue = addValue((LocationUnit **) &oldValue, path.trim(0, 1), value);

    if (oldValue != newValue)
      oldValue->push_back(newValue);

    return oldValue;
  }
  else
    return value;
}

LocationUnit *LocationUnit::getValue(const Path &path) {
  return this;
}

LocationTree::LocationTree(int count) {
}

LocationUnit *LocationTree::getValue(const Path &path) {
  return path.size() ? (LocationTree *) operator[](path[0])->getValue(path.trim(0, 1)) : this;
}

int ListUnitAreas::getSize(int dir) {
  return 0;
}

UnitArea::UnitArea(const Point &unitArea) {
  this->unitArea = unitArea;
}

int UnitArea::getSize(int dir) {
  return unitArea[dir];
}

IntTreeUnit::IntTreeUnit(int value) {
  this->value = value;
}

IntTreeUnit::IntTreeUnit(int value, IntTreeUnit *children) : std::vector<IntTreeUnit *>(*children) {
  this->value = value;
}

void IntTreeUnit::add(IntTreeUnit *unit) {
  push_back(unit);
}

int IntTreeUnit::getNumChildren() {
  return size();
}

LayoutUnitArea::LayoutUnitArea(const DirIntTreeUnits &trees, void *unitAreas) {
  this->trees = trees;
  this->unitAreas = unitAreas;
}

int LayoutUnitArea::getSize(int dir) {
  return trees[dir]->value;
}
/*
ValueStackElement::ValueStackElement(long parseFlags, bool b) {
  this->parseFlags = parseFlags;
  this->b = b;
}
*/