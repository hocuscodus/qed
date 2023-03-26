/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
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