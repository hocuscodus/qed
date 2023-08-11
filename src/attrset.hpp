/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#ifndef qed_attrset_h
#define qed_attrset_h

#include <unordered_set>
#include <vector>
#include "sizer.hpp"
#include "object.hpp"

struct Attr {
	int flags;
	void *returnType;
  int localIndex;

	Attr(int flags, void *returnType, int localIndex);
};

struct ChildAttrSets;
struct VM;
struct UIDirectiveExpr;

typedef DirType<long> DirFlags;

struct AttrSet {
  std::unordered_map<std::string, Attr *> attrs;
	ChildAttrSets *children = NULL;
  std::unordered_set<std::string> flagsMap;
	int parseFlags;
	int refreshFlags;
	int areaParseFlags;
	int maxDirFlags;
  DirFlags addDirs;
  DirFlags maxDirs;
  DirFlags zFlags;
  Path posPaths[NUM_DIRS];
	int offset;

  void init(int *offset, Point &zoneOffsets, std::array<long, NUM_DIRS> &arrayDirFlags, ValueStack<ValueStackElement> &valueStack, UIDirectiveExpr *listExpr, int parentRefreshFlags, ObjFunction *function);
	void parseCreateSizers(DirSizers &topSizers, int dir, int *zone, Path path, Path flagsPath);
	void parseAdjustPaths(DirSizers &topSizers, int dir);
	void initAttr(std::string propertys, int flags, void *returnType, int index);
	int getNumAreas();
	int getZones(Point &zoneOffset, int dir, int childDir, std::array<long, NUM_DIRS> arrayDirFlags);
  int getChildZones(int dir);
  int getZones(int &zoneOffset, long arrayDirFlags);
	bool isEditable(VM &vm, Obj obj, Path path);
	bool getEditableFlag(VM &vm, Obj obj, Path path);
	int getNumChildren();
	void parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, int dimFlags, const Path &path, Value *values, IndexList *instanceIndexes, LocationUnit **areaUnits);
/*
	void parseCreateUIValues(VM &vm, void *value, int flags, int *path, Obj subValue, ValueTree &valueTree);
	void parseRefresh(VM &vm, ValueStack<Value *> valueStack, int *limits, Path path, ValueTree &valueTree, LocationUnit **areaUnits, Sizer *topSizers, LayoutContext *layoutContexts);
	void parseRefresh2(VM &vm, ValueStack<Value *> valueStack, int domain, int *limits, Path path, ValueTree &valueTree, LocationUnit **areaUnits, Sizer *topSizers, LayoutContext *layoutContexts);
	Path parseLocateEvent(VM &vm, Obj obj, int *limits, Path path, ValueTree &valueTree, void *subObj, LocationUnit **areaUnits, IntTreeUnit subTree, Sizer *sizers, LayoutContext *layoutContexts, int *location, Path currentFocusPath);
	Path parseLocateEvent2(VM &vm, Obj obj, int domain, int *limits, Path path, ValueTree &valueTree, void *subObj, LocationUnit **areaUnits, IntTreeUnit subTree, Sizer *sizers, LayoutContext *layoutContexts, int *location, Path currentFocusPath);
	void parseOut(VM &vm, Obj obj, Path focusPath, Path path, ValueTree &valueTree, void *subObj);
	void parseOut2(VM &vm, Obj obj, int domain, Path focusPath, Path path, ValueTree &valueTree, void *subObj);
*/
	void parseSetFocus(VM &vm, Obj obj, Path path);
	void *execcmd2(VM &vm, Obj obj, std::string modifier, void *args);
	bool hasContent(std::string modifier);
	Path parseFindId(VM &vm, std::string id);
};

struct ChildAttrSets : std::vector<AttrSet *> {
	int childDir;
	int childrenFlags;
	int parseFlags;
	int areaParseFlags;
	int numAreas;
  DirFlags zones;
  DirFlags flags;
  DirFlags zFlags;

	ChildAttrSets(int *offset, Point &zoneOffsets, int childDir, std::array<long, NUM_DIRS> &arrayDirFlags, ValueStack<ValueStackElement> &valueStack, UIDirectiveExpr *listExpr, int parentRefreshFlags, ObjFunction *function);

	SizerType getSizerType(int dir, int *zone);
	void parseCreateSizers(DirSizers &topSizers, int dir, int *zone, Path path, Path flagsPath);
	void parseAdjustPaths(DirSizers &topSizers, int dir);
	void parseCreateAreasTree(VM &vm, ValueStack<Value *> &valueStack, int dimFlags, const Path &path, Value *values, IndexList *instanceIndexes, LocationUnit **areaUnits);
/*
	void parseRefresh(VM &vm, ValueStack valueStack, int *limits, Path path, ValueTree &valueTree, void *areas, Sizer *topSizers, LayoutContext *layoutContexts);
	Path parseLocateEvent(VM &vm, Obj obj, int *limits, Path path, ValueTree &valueTree, void *subObj, void *areas, IntTreeUnit subTree, Sizer *sizers, LayoutContext *layoutContexts, int *location, Path currentFocusPath);
	void parseOut(VM &vm, Obj obj, Path focusPath, Path path, ValueTree &valueTree, void *subObj);
	void parseCreateUIValues(VM &vm, void *value, Obj subValue, int flags, int *dimIndexes, ValueTree &valueTree);
	Path parseFindId(VM &vm, std::string id);
	void parseSetFocus(VM &vm, Obj obj, Path path);
	std::string toString();*/
};

#endif