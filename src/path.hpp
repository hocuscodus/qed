/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_path_h
#define qed_path_h

#include <iostream>
#include <vector>

struct Path : public std::vector<int> {
  Path();
  Path(int value);
  Path(int *values);
  Path(const Path &source);

//  Path(int pathCount);

  static Path attach(int index, Path path);

  int get(int index) const;
  void set(int index, int value);
  void setFromPath(Path source);
  bool equals(Path path) const;
  Path concat(Path path) const;
  Path concat(int path) const;
  Path trim(int index) const;
  Path trim(int startIndex, int range) const;
  Path filter(int dimFlags) const;
  Path filterPathWrong(int mainFlags, int subFlags) const;
  Path filterPath(int mainFlags, int subFlags) const;
//  void *addValue(List list, void *value);
//  void *getValue(void *value);
//  static Path eval(VM &vm, Obj obj);
  std::string toString();
};

#endif