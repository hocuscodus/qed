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
#include "path.hpp"

Path::Path() : std::vector<int>() {
}

Path::Path(int value) : std::vector<int>() {
  push_back(value);
}

Path::Path(const Path &source) : std::vector<int>(source) {
}

Path Path::concat(int path) const {
  Path p(*this);

  p.push_back(path);
  return p;
}

Path Path::trim(int index) const {
  int numpath = size();

  if (index >= 0 && index <= numpath)
    return trim(index, numpath - index);

  return *this;
}

Path Path::trim(int startIndex, int range) const {
  int ndx;
  int numpath = size();

  if (startIndex < 0) {
    range += startIndex;
    startIndex = 0;
  }

  if (startIndex + range > numpath)
    range = numpath - startIndex;

  Path resized;
  int count = 0;

  resized.reserve(numpath - range);

  for (ndx = 0; ndx < startIndex; ndx++)
    resized[count++] = operator[](ndx);

  for (ndx = startIndex + range; ndx < numpath; ndx++)
    resized[count++] = operator[](ndx);

  return (resized);
}
#if 0
import 'Obj.dart';
import 'QEDProcess.dart';
import 'Util.dart';
import 'declaration/LambdaDeclaration.dart';

class Path {
  List<int> values;

  Path.fromNumDim(int numDim) {
    values = List.filled(numDim, 0);
  }

  Path.fromValue(int value) {
    values = [value];
  }

  Path.fromValueList(List<int> values) {
    this.values = values;
  }

  Path.fromPath(final Path source) {
    setFromPath(source);
  }

  static Path zero(int pathCount) {
    Path newPath = new Path();
    newPath.values = List<int>.filled(pathCount, 0);

    return (newPath);
  }

  static Path attach(int index, Path path) {
    return path != null ? new Path.fromValue(index).concat(path) : null;
  }

  int getNumLevels() {
    return values.length;
  }

  int get(int index) {
    return (/*index < 0 || index >= values.length ? 0 : */ values[index]);
  }

  void set(int index, int value) {
    values[index] = value;
  }

  void setFromPath(Path source) {
    values = List.from(source.values);
  }

  Path concat(Path path) {
    int index;
    Path result = zero(values.length + path.values.length);

    for (index = values.length - 1; index >= 0; index--)
      result.values[index] = values[index];

    for (index = path.values.length - 1; index >= 0; index--)
      result.values[index + values.length] = path.values[index];

    return (result);
  }

  bool equals(Path path) {
    bool rc = path != null && path.getNumLevels() == getNumLevels();

    for (var index = 0; rc && index < getNumLevels(); index++)
      rc = path.get(index) == get(index);

    return (rc);
  }

  Path filter(int dimFlags) {
    int numLevels = getNumLevels();
    Path path = new Path();

    for (var index = 0; index < numLevels; index++)
      if ((dimFlags & (1 << index)) != 0) path = path.concatInt(values[index]);

    return path;
  }

  Path filterPathWrong(int mainFlags, int subFlags) {
    int numLevels = getNumLevels();
    Path path = new Path();

    for (var index = 0; index < numLevels; index++) {
      int bit = mainFlags & -mainFlags;

      mainFlags ^= bit;

      if ((subFlags & bit) != 0) path = path.concatInt(values[index]);
    }

    return path;
  }

  Path filterPath(int mainFlags, int subFlags) {
    var domain = [mainFlags];
    Path path = new Path();

    for (var index = ctz(domain); index != -1; index = ctz(domain))
      if ((subFlags & (1 << index)) != 0) path = path.concatInt(values[index]);

    return path;
  }

  static Path read(FakeStream fakeStream) {
    Path path = Path.zero(fakeStream.read());

    for (var index = 0; index < path.getNumLevels(); index++)
      path.set(index, fakeStream.read());

    return path;
  }

  static Path eval(VM &vm, Obj obj) {
    int size = obj.read(process);
    Path path = Path.zero(size);

    for (var index = 0; index < size; index++)
      path.set(index, obj.read(process));

    return path;
  }

  String toString() {
    StringBuffer buffer = new StringBuffer();

    for (var index = 0; index < getNumLevels(); index++) {
      if (index > 0) buffer.write(" ");
      buffer.write(get(index));
    }

    return ("(" + buffer.toString() + ")");
  }
}
#endif
