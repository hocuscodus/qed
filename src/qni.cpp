/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <map>
#include <string.h>
#include "qni.hpp"

static std::map<std::string, NativeFn> &getQniFnMap() {
  static std::map<std::string, NativeFn> qniFnMap;

  return qniFnMap;
}

static std::map<std::string, NativeClassFn> &getQniClassMap() {
  static std::map<std::string, NativeClassFn> qniClassMap;

  return qniClassMap;
}

bool addNativeFn(const char *name, NativeFn nativeFn) {
  getQniFnMap()[name] = nativeFn;
  return true;
}

bool addNativeClassFn(const char *name, NativeClassFn nativeClassFn) {
  getQniClassMap()[name] = nativeClassFn;
  return true;
}

bool bindFunction(std::string prefix, ObjFunction *function) {
  std::string name = prefix + "_" + function->name->chars;
  std::map<std::string, NativeFn>::iterator i = getQniFnMap().find(name);
  bool rc = i != getQniFnMap().end();

  if (rc)
    function->native = &newNative(i->second)->obj;
  else {
    std::map<std::string, NativeClassFn>::iterator i = getQniClassMap().find(name);

    rc = i != getQniClassMap().end();

    if (rc)
      function->native = &newNativeClass(i->second)->obj;
  }

  return rc;
}