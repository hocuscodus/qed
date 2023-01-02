/*
 *    Copyright (C) 2022 Hocus Codus Software inc. All rights reserved.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */

import 'dart:collection';

import '../lang/Path.dart';

class ValueTree {
  HashMap<String, Object> attrs;
  HashMap<int, ValueTree> children;

  bool isEmpty() {
    return attrs == null && children == null;
  }

  void addAttr(String key, List<int> path, Object value) {
    if (attrs == null)
      attrs = new HashMap<String, Object>();

    Object oldValue = attrs[key];
    Path thePath = path != null ? new Path.fromValueList(path) : new Path();
    Object newValue = thePath.addValue(oldValue, value);

    if (oldValue != newValue)
      attrs[key] = newValue;
  }

  void putValueTree(int index, ValueTree tree) {
    if (children == null)
      children = new HashMap<int, ValueTree>();

    children[index] = tree;
  }

  int getNumValueTrees() {
    return children != null ? children.length : 0;
  }

  ValueTree getValueTree(int index) {
    return children != null ? children[index] : null;
  }

  Object getValue(String k) {
    return attrs[k];
  }

  static String header = "";

  String toString() {
    bool first = true;
    StringBuffer str = new StringBuffer();

    if (attrs != null) {
      str..write(header)..write("(");
      attrs.forEach((k, v) {
        str..write(first ? "" : " ")..write(k)..write(": ")..write(v);
        first = false;
      });
      str.writeln(")");
    }

    if (children != null) {
      String oldHeader = header;
      header += "  ";
      children.forEach((k, v) {
        str..write(header)..writeln(k)..writeln(v);
      });
      header = oldHeader;
    }

    return str.toString();
  }
}