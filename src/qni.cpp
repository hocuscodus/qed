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