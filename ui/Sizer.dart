/*
 *    Copyright (C) 2022 Hocus Codus Software inc. All rights reserved.
 *
 *    Author: Martin Savage (hocuscodus@gmail.com)
 */

import 'dart:math';

import '../lang/Obj.dart';
import '../lang/Path.dart';
import '../lang/Util.dart';
import 'LayoutContext.dart';
import 'ListUnitAreas.dart';

enum SizerType {
  getter,
  adder,
  maxer,
  zoneMaxer,
  multiplier
}

class Sizer {
  int index;
  List<Sizer> children;

  Sizer(int index) {
    this.index = index;
  }

  int getNumSizers() {
    return children != null ? children.length : 0;
  }

  Path add(int zone, int addDirs, Path path, Path flagsPath, int offset, int maxDirs) {
    List<Path> sizerPath = [zone == -2 ? new Path() : new Path.fromValue(zone == -1 ? 0 : 1)];
    Sizer start = zone == -2 ? this : zone == -1 ? children[0] : children[1].put(zone & 1 != 0 ? SizerType.zoneMaxer : SizerType.maxer, zone, offset, 0, 0, true, sizerPath);

    if (zone >= 0 && zone & 1 != 0) {
      (start as ZoneMaxer).tree._addFlat(path, flagsPath, offset, maxDirs, addDirs | maxDirs, sizerPath[0]);
      start = start.put(SizerType.maxer, addDirs, offset, maxDirs, addDirs | maxDirs, true, sizerPath);
    }

    return start._addFlat(path, flagsPath, offset, maxDirs, addDirs | maxDirs, sizerPath[0]);
  }

  Path _addFlat(Path path, Path flagsPath, int offset, int maxDirs, int dirs, Path sizerPath) {
    int index = path.get(0);
    List<Path> sizerPath2 = [sizerPath];
    Sizer child = put(SizerType.values[flagsPath.get(0)], index, offset, maxDirs, dirs, false, sizerPath2);

    if (path.getNumLevels() > 1)
      return child._addFlat(path.trimRange(0, 1), flagsPath.trimRange(0, 1), offset, maxDirs, dirs, sizerPath2[0]);
    else
      return sizerPath2[0];
  }

  void adjust(Path path) {
    if (path.values[0] == 1) {
      Sizer adder = children[1];
      int zone = path.values[1];
      int zoneIndex = adder.findIndex(zone, false);

      if (zoneIndex != -1) {
        path.values[1] = zoneIndex;

        if (zone & 1 != 0) {
          int domain = path.values[2];
          int domainIndex = adder.children[zoneIndex].findIndex(domain, false);

          path.values[2] = domainIndex;
        }
      }
    }
  }

  int findIndex(int index, bool locationFlag) {
    int ndx = -1;
    int numSizers = getNumSizers();

    while (++ndx < numSizers && children[ndx].index < index);

    return locationFlag || (ndx >= 0 && ndx < numSizers && children[ndx].index == index) ? ndx : -1;
  }

  Sizer put(SizerType sizerType, int index, int offset, int maxDirs, int dirs, bool byIndex, List<Path> sizerPath) {
    int ndx = findIndex(index, true);
    Sizer child = ndx < getNumSizers() && children[ndx].index == index ? children[ndx] : null;

    sizerPath[0] = sizerPath[0].concatInt(byIndex ? index : ndx);

    if (child == null) {
      switch (sizerType) {
        case SizerType.getter:
          child = new Getter(index, offset, maxDirs, dirs);
          break;

        case SizerType.adder:
          child = new Adder(index);
          break;

        case SizerType.maxer:
          child = new Maxer(index);
          break;

        case SizerType.zoneMaxer:
          child = new ZoneMaxer(index);
          break;

        case SizerType.multiplier:
          child = new Multiplier(index);
          break;
      }

      if (children == null)
        children = List.filled(0, null, growable: true);

      children.insert(ndx, child);
    }

    return child;
  }

  IntTreeUnit *parseResize(List areas, Path path, int dir, List<int> limits) {
    IntTreeUnit tree = new IntTreeUnit(0);
    IntTreeUnit previous = null;

    for (int index = 0; index < getNumSizers(); index++) {
      Sizer sizer = children[index];
      IntTreeUnit subTree = sizer.parseResize(areas, path, dir, limits);

      if (previous != null)
        subTree.value = process(previous.value, subTree.value);

      tree.add(subTree);
      previous = subTree;
    }

    tree.value = previous != null ? previous.value : 0;

    return tree;
  }

  void adjustPos(Path posPath, Path path, LayoutContext layoutContext) {
    int index = posPath.getNumLevels() != 0 ? posPath.values[0] : -1;

    if (index != -1) {
      layoutContext.treeUnit = layoutContext.treeUnit.set[index];
      children[index].adjustPos(posPath.trimRange(0, 1), path, layoutContext);
    }
  }

  int process(int area1, int area2) {
    return 0;
  }

  Sizer getZoneSizer(int zone) {
    if (zone >= 0) {
      int ndx = children[1].findIndex(zone, false);
      Sizer sizer = ndx != -1 ? children[1].children[ndx] : null;

      return sizer != null && sizer is ZoneMaxer ? sizer.tree : sizer;
    }
    else
      return this;
  }

  int findZone(LayoutContext layoutContext, List<int> location, int dir, Path path) {
    return -1;
  }

  int findIndexes(int location, IntTreeUnit tree) {
    return 0;
  }

  Sizer findSizer(int index, int domain, Obj obj, Path path, List<int> location, int dir, LayoutContext layoutContext) {
    int ndx = findIndex(index, false);

    if (ndx != -1) {
      layoutContext.treeUnit = layoutContext.treeUnit.set[ndx];
      return children[ndx];
    }

    return null;
  }
}

class Adder extends Sizer {
  Adder(int index) : super(index) {}

  int process(int area1, int area2) {
    return area1 + area2;
  }

  int findZone(LayoutContext layoutContext, List<int> location, int dir, Path path) {
    // crude implementation for now but might be ok, the number of zones should be minimal
    for (int index = 0; index < getNumSizers(); index++)
      if (layoutContext.treeUnit.set[index].value >= location[dir]) {
        location[dir] -= layoutContext.adjustPos(index);
        return index;//.findZone(layoutContext, location, dir, path);
      }

    return -1;
  }

  int findIndexes(int location, IntTreeUnit tree) {
    return -1; // wrong implementation, should isolate the a single index
  }

  void adjustPos(Path posPath, Path path, LayoutContext layoutContext) {
    int index = posPath.getNumLevels() != 0 ? posPath.values[0] : -1;

    if (index != -1) {
      layoutContext.adjustPos(index);
      children[index].adjustPos(posPath.trimRange(0, 1), path, layoutContext);
    }
  }
}

class Maxer extends Sizer {
  Maxer(int index) : super(index) {}

  int process(int area1, int area2) {
    return max(area1, area2);
  }

  int findIndexes(int location, IntTreeUnit tree) {
    return -1;
  }
}

class ZoneMaxer extends Sizer {
  Sizer tree = new Maxer(-1);

  ZoneMaxer(int index) : super(index) {}

  IntTreeUnit *parseResize(List areas, Path path, int dir, List<int> limits) {
    IntTreeUnit tree = new IntTreeUnit(0);
    IntTreeUnit previous = null;

    for (int index = 0; index < getNumSizers(); index++) {
      Sizer domainSizer = children[index];
      IntTreeUnit subTree = parseResize2(domainSizer, domainSizer.index, areas, path, dir, limits);

      if (previous != null)
        subTree.value = max(subTree.value, previous.value); // TODO do something better than this...

      tree.add(subTree);
      previous = subTree;
    }

    tree.value = previous != null ? previous.value : 0;

    return tree;
  }

  IntTreeUnit *parseResize2(Sizer domainSizer, int domain, List areas, Path path, int dir, List<int> limits) {
    var dom = [domain];
    int dim = ctz(dom);
    IntTreeUnit tree;

    if (dim != -1) {
      IntTreeUnit previous = null;

      tree = new IntTreeUnit(0);

      for (int index = 0; index < limits[dim]; index++) {
        path.values[dim] = index;
        IntTreeUnit subTree = parseResize2(domainSizer, dom[0], areas, path, dir, limits);

        if (previous != null)
          subTree.value += previous.value;

        tree.add(subTree);
        previous = subTree;
      }

      tree.value = previous != null ? previous.value : 0;
    }
    else
      tree = domainSizer.parseResize(areas, path, dir, limits);

    return tree;
  }

  void adjustPos(Path posPath, Path path, LayoutContext layoutContext) {
    int index = posPath.values[0];
    Path subPosPath = posPath.trimRange(0, 1);
    var dom = [children[index].index];

    layoutContext.treeUnit = layoutContext.treeUnit.set[index];

    for (int dim = ctz(dom); dim != -1; dim = ctz(dom))
      layoutContext.adjustPos(path.values[dim]);

    children[index].adjustPos(subPosPath, path, layoutContext);
  }

  int findIndexes(int location, IntTreeUnit tree) {
    return -1;
  }
/*
	int findIndexes(int location, IntTreeUnit tree) {
		int indexes = 0;

		for (int index = 0; index < getNumSizers(); index++) {
			Sizer domainSizer = children[index];

			for (int index2 = 0; index2 < domainSizer.getNumSizers(); index2++)
				indexes |= 1 << domainSizer.children[index2].index;
		}

		return indexes;
	}
*/
  Sizer findSizer(int index, int domain, Obj obj, Path path, List<int> location, int dir, LayoutContext layoutContext) {
    int ndx = findIndex(domain, false);
    Sizer domainSizer = ndx != -1 ? children[ndx] : null;
    int ndx2 = domainSizer != null ? domainSizer.findIndex(index, false) : -1;

    if (ndx2 != -1) {
      var dom = [domain];

      layoutContext.treeUnit = layoutContext.treeUnit.set[ndx];

      for (int dim = ctz(dom); dim != -1; dim = ctz(dom)) {
        int index = layoutContext.findIndex(location[dir]);

        location[dir] -= layoutContext.adjustPos(index);
        path.set(dim, index);
        obj.func.setIndex(obj, dim, index);
      }

      layoutContext.treeUnit = layoutContext.treeUnit.set[ndx2];
      return domainSizer.children[ndx2];
    }

    return null;
  }
}

class Getter extends Sizer {
  int offset;
  int maxDirs;
  int dirs;

  Getter(int index, int offset, int maxDirs, int dirs) : super(index) {
    this.offset = offset;
    this.maxDirs = maxDirs;
    this.dirs = dirs;
  }

  IntTreeUnit *parseResize(List areas, Path path, int dir, List<int> limits) {
    return new IntTreeUnit(_getMaxSize(maxDirs, areas[offset], path, dir, limits));
  }

  int _getMaxSize(int maxDirs, Object areas, Path path, int dir, List<int> limits) {
    var dom = [maxDirs];
    int dim = ctz(dom);
    int size = 0;

    if (dim != -1)
      for (int index = 0; index < limits[dim]; index++) {
        path.values[dim] = index;
        size = max(_getMaxSize(dom[0], areas, path, dir, limits), size);
      }
    else
      size = _getSize(dirs, areas, path, dir);

    return size;
  }

  int _getSize(int dirs, Object areas, Path path, int dir) {
    var dom = [dirs];
    int dim = ctz(dom);

    return dim != -1 ? _getSize(dom[0], (areas as List)[path.values[dim]], path, dir) : (areas as ListUnitAreas).getSize(dir);
  }
}

class Multiplier extends Sizer {
  Multiplier(int index) : super(index) {}
}
