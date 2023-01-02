/*
 *    Copyright (C) 2016 Hocus Codus Software inc.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */

import 'Obj.dart';
import 'QEDProcess.dart';
import 'Util.dart';
import 'declaration/LambdaDeclaration.dart';

class Path {
  List<int> values;

  Path() {
    values = List.filled(0, 0);
  }

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

  Path concatInt(int path) {
    int length = values.length;
    Path result = zero(length + 1);

    result.values[length] = path;

    while (length-- > 0) result.values[length] = values[length];

    return (result);
  }

  Path trim(int index) {
    int numpath = getNumLevels();

    return (index >= 0 && index <= numpath
        ? trimRange(index, numpath - index)
        : new Path.fromPath(this));
  }

  Path trimRange(int startIndex, int range) {
    int ndx;
    int numpath = values.length;

    if (startIndex < 0) {
      range += startIndex;
      startIndex = 0;
    }

    if (startIndex + range > numpath) range = numpath - startIndex;

    Path resized = zero(numpath - range);
    for (ndx = 0; ndx < startIndex; ndx++) resized.values[ndx] = values[ndx];

    for (ndx = startIndex + range; ndx < numpath; ndx++)
      resized.values[ndx - range] = values[ndx];

    return (resized);
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

  Object getValue(Object value) {
    for (int index = 0; index < getNumLevels(); index++)
      value = (value as List)[get(index)];

    return value;
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
