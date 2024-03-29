/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_sizer_h
#define qed_sizer_h

#include "layout.hpp"

enum SizerType {
  getter,
  adder,
  maxer,
  zoneMaxer,
  multiplier
};

struct Sizer : std::vector<Sizer *> {
  int index;

  Sizer(int index);
  virtual ~Sizer();

  int getNumSizers();
  Path add(int zone, int addDirs, const Path &path, const Path &flagsPath, int offset, int maxDirs);
  Path _addFlat(Path path, const Path &flagsPath, int offset, int maxDirs, int dirs, const Path &sizerPath);
  void adjust(Path path);
  int findIndex(int index, bool locationFlag);
  Sizer *put(SizerType sizerType, int index, int offset, int maxDirs, int dirs, bool byIndex, Path *sizerPath);
  virtual IntTreeUnit *parseResize(LocationUnit **areaUnits, const Path &path, int dir, const Point &limits);
//  virtual void adjustPos(Path posPath, const Path &path, LayoutContext layoutContext);
  virtual int process(int area1, int area2);
  Sizer getZoneSizer(int zone);
//  virtual int findZone(LayoutContext layoutContext, const Point &location, int dir, const Path &path);
//  virtual int findIndexes(int location, IntTreeUnit tree);
//  virtual Sizer findSizer(int index, int domain, Obj obj, const Path &path, const Point &location, int dir, LayoutContext layoutContext);
};

typedef DirType<Sizer *> DirSizers;

struct Adder : Sizer {
  Adder(int index);

  int process(int area1, int area2);
//  int findZone(LayoutContext layoutContext, const Point &location, int dir, const Path &path);
  int findIndexes(int location, IntTreeUnit tree);
//  void adjustPos(Path posPath, const Path &path, LayoutContext layoutContext);
};

struct Maxer : Sizer {
  Maxer(int index);

  int process(int area1, int area2);
  int findIndexes(int location, IntTreeUnit tree);
};

struct ZoneMaxer : Sizer {
  Maxer tree;

  ZoneMaxer(int index);
  ~ZoneMaxer();

//  IntTreeUnit *parseResize(LocationUnit **areaUnits, const Path &path, int dir, const Point &limits);
//  IntTreeUnit *parseResize2(Sizer domainSizer, int domain, LocationUnit **areaUnits, const Path &path, int dir, const Point &limits);
//  void adjustPos(Path posPath, const Path &path, LayoutContext layoutContext);
//  int findIndexes(int location, IntTreeUnit tree);
//  Sizer findSizer(int index, int domain, Obj obj, const Path &path, const Point &location, int dir, LayoutContext layoutContext);
};

struct Getter : Sizer {
  int offset;
  int maxDirs;
  int dirs;

  Getter(int index, int offset, int maxDirs, int dirs);

  IntTreeUnit *parseResize(LocationUnit **areaUnits, const Path &path, int dir, const Point &limits);
  int _getMaxSize(int maxDirs, LocationUnit **areaUnits, const Path &path, int dir, const Point &limits);
  int _getSize(int dirs, LocationUnit *areaUnits, const Path &path, int dir);
};

struct Multiplier : Sizer {
  Multiplier(int index);
};

#endif
