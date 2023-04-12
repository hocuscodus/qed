/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.hpp"
#include "parser.hpp"
#include "vm.hpp"
#include "qni.hpp"

const char *qedLib =
"void println(String str);"
"//void print(String str)\n"
"void post(int handlerFn);"
"int max(int a, int b);"
""
"var WIDTH = 1;"
"var HEIGHT = 2;"
"var OBLIQUE = 3;"
"int COLOR_RED = 0xFF0000;"
"int COLOR_GREEN = 0x00FF00;"
"int COLOR_YELLOW = 0xFFFF00;"
"int COLOR_BLUE = 0x0000FF;"
"int COLOR_BLACK = 0x000000;"
"float clock();"
"void saveContext();"
"void restoreContext();"
"void oval(int pos, int size);"
"void rect(int pos, int size);"
"void pushAttribute(int index, int value);"
"void pushAttribute(int index, float value);"
"void popAttribute(int index);"
"int getTextSize(String text);"
"int getInstanceSize(int instance);"
"void displayText(String text, int pos, int size);"
"void displayInstance(int instance, int pos, int size);"
"bool onInstanceEvent(int instance, int event, int pos, int size);"
/*"int[] convertToPoint(int point) {return([point, point])}"
"int[] convertToPoint(int[] point) {return(point)}"
"float[] convertToFloatPoint(float point) {return([point, point])}"
"float[] convertToFloatPoint(float[] point) {return(point)}"
"int convertToInt(int x) {return(x)}"
"float convertToFloat(float x) {return(x)}"*/
""
"void Timer(int timeoutMillis) {"
"  var _timerObj;"
""
"  void reset();"
"};"
""
"void CoList() {"
"  var _coListObj;"
""
"  void end();"
"  bool remove(int index);"
"  bool yield();"
"  bool process();"
"}\n"
;

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

  CoThread *coThread = newThread(NULL);
  VM vm(coThread, false);
  ObjClosure *closure = coThread->pushClosure(function);

//  strcpy(buffer, "");
//  scanner.reset(buffer);

  for (;;) {
    if (function != NULL && coThread->call(closure, coThread->savedStackTop - coThread->stack - 1)) {
      if (vm.run() == INTERPRET_OK)
        length += strlen(line);
      else {
        buffer[length] = '\0';
        scanner.reset(&buffer[length]);
      }

      function->chunk.reset();
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

extern "C" {
void runSource(const char *source) {
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

  Scanner scanner(buffer);
  Parser parser(scanner);
  ObjFunction *function = parser.compile();

  if (!function)
    return;
/*
  CoThread *coThread = newThread(NULL);
  VM vm(coThread, true);
  ObjClosure *closure = coThread->pushClosure(function);

  if (function == NULL)
    exit(65);

  InterpretResult result = vm.interpret(closure);

//  freeObjects();

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);*/
  function->print();
  std::cout << CodeGenerator(parser, NULL).str().str();
}
}

int main(int argc, const char *argv[]) {
  if (argc == 1)
    repl();
  else if (argc == 2) {
    char *source = readFile(argv[1]);

    runSource(source);
    free(source);
  }
  else {
    fprintf(stderr, "Usage: qed [path]\n");
    exit(64);
  }

  return 0;
}
