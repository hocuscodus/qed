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
#ifndef qed_listunitareas_h
#define qed_listunitareas_h

#include <string>
#include <stack>
#include <unordered_map>
#include <vector>
#include "path.hpp"
#include "value.h"

struct LocationUnit {
  virtual ~LocationUnit();

  static LocationUnit *addValue(LocationUnit **start, const Path &path, LocationUnit *value);
  virtual LocationUnit *getValue(const Path &path);
};

struct LocationTree : public LocationUnit, public std::vector<LocationUnit *> {
  LocationTree(int count);

  LocationUnit *addValue(const Path &path, LocationUnit *value);
  LocationUnit *getValue(const Path &path);
};

struct ListUnitAreas : public LocationUnit {
	virtual int getSize(int dir);
};

struct UnitArea : ListUnitAreas {
	Point unitArea;

	UnitArea(const Point &unitArea);

	int getSize(int dir);
};

struct IntTreeUnit : public std::vector<IntTreeUnit *> {
	int value;

	IntTreeUnit(int value);
	IntTreeUnit(int value, IntTreeUnit *children);

	void add(IntTreeUnit *unit);
	int getNumChildren();
};

typedef DirType<IntTreeUnit *> DirIntTreeUnits;

struct LayoutUnitArea : public ListUnitAreas {
	DirIntTreeUnits trees;
	void *unitAreas;

	LayoutUnitArea(const DirIntTreeUnits &trees, void *unitAreas);

	int getSize(int dir);
};

struct ValueStackElement {
  long parseFlags;
  bool b;
  int localIndex;
};

template <typename T> struct ValueStack {
  T defaultValue;
	std::unordered_map<std::string, std::stack<T>> map;

	ValueStack(T defaultValue = T()) {
    this->defaultValue = defaultValue;
  }

	void push(std::string key, T value) {
    if (map.find(key) == map.end())
      map.insert(std::pair<std::string, std::stack<T>>(key, std::stack<T>()));

    map[key].push(value);
  }

	T pop(std::string key) {
    auto it = map.find(key);

    if (it != map.end()) {
      T element = it->second.top();

      it->second.pop();

      if (it->second.empty())
        map.erase(key);

      return element;
    }

    return defaultValue;
  }

	T get(std::string key) {
    auto it = map.find(key);

    return it != map.end() ? it->second.top() : defaultValue;
  }
};

#endif