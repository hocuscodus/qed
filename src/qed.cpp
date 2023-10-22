/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.hpp"

const char *qedLib;

static void repl() {
  char line[1024] = "";
  char buffer[32768] = "";
  int length = 0;

  strcpy(buffer, qedLib);
  Scanner scanner(buffer);
  Parser parser(scanner);
  ObjFunction *function = parser.compile();

  if (!function)
    return;

//  strcpy(buffer, "");
//  scanner.reset(buffer);
/*
  for (;;) {
    if (function != NULL && coThread->call(closure, coThread->savedStackTop - coThread->stack - 1)) {
      if (vm.run() == INTERPRET_OK)
        length += strlen(line);
      else {
        buffer[length] = '\0';
        scanner.reset(&buffer[length]);
      }

//      function->chunk.reset();
      coThread->reset();
    }

    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    strcat(buffer, line);

    parser.compile();
  }
*/
  freeObjects();
}

static char *readFile(const char *path) {
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);

  size_t fileSize = ftell(file);

  rewind(file);

  char *buffer = (char *) malloc(fileSize + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[bytesRead] = '\0';
  fclose(file);
  return buffer;
}

extern std::stringstream s;

std::string runSrc(const char *source) {
  qedLib = readFile("qedlib.qed");
  int qedLibLength = strlen(qedLib);
  int sourceLength = strlen(source);
  char *buffer = (char *) malloc(qedLibLength + sourceLength + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read the source file.\n");
    exit(74);
  }

  strcpy(buffer, qedLib);
  strcpy(&buffer[qedLibLength], source);

  buffer[qedLibLength + sourceLength] = '\0';
  s.str("");
  s.clear();

  Scanner scanner(buffer);
  Parser parser(scanner);
  ObjFunction *function = parser.compile();

  return s.str();
}

int main(int argc, const char *argv[]) {
  if (argc == 1)
    repl();
  else if (argc == 2) {
    char *source = readFile(argv[1]);

    std::cout << runSrc(source);
    free(source);
  }
  else {
    fprintf(stderr, "Usage: qed [path]\n");
    exit(64);
  }

  return 0;
}

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

using namespace emscripten;

std::string runSource(std::string source) {
  return runSrc(source.c_str());
}

EMSCRIPTEN_BINDINGS(module) {
  function("runSource", &runSource);
}
#endif
