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
#include "vm.hpp"
#include "qni.hpp"

const char *qedLib =/*
"var WIDTH = 1;"
"var HEIGHT = 2;"
"var OBLIQUE = 3;"
"int COLOR_RED = 0xFF0000;"
"int COLOR_GREEN = 0x00FF00;"
"int COLOR_YELLOW = 0xFFFF00;"
"int COLOR_BLUE = 0x0000FF;"
"int COLOR_BLACK = 0x000000;"
"\n"
"void println(String str) {\n"
"/$console.log(str)"
"$/\n}\n"
"void post(int handlerFn) {\n"
"/$postCount++;\n"
"  setTimeout(function() {\n"
"    handlerFn();\n"
"    _refresh();\n"
"  })"
"$/\n}\n"
"int max(int a, int b) {\n"
"/$return a > b ? a : b"
"$/\n}\n"
"float clock() {\n}\n"
"void saveContext() {\n"
"/$ctx.save()"
"$/\n}\n"
"void restoreContext() {\n"
"/$ctx.restore()"
"$/\n}\n"
"void oval(int pos, int size) {\n"
"/$ctx.fillStyle = toColor(getAttribute(10));\n"
"  ctx.globalAlpha = \"\" + getAttribute(11);\n"
"  ctx.beginPath();\n"
"  ctx.ellipse((pos >> 16) + (size >> 17), (pos & 65535) + ((size & 65535) >> 1), size >> 17, (size & 65535) >> 1, 0, 0, 2*Math.PI);\n"
"  ctx.fill();\n"
"  ctx.clip()"
"$/\n}\n"
"void rect(int pos, int size) {\n"
"/$ctx.fillStyle = toColor(getAttribute(10));\n"
"  ctx.globalAlpha = \"\" + getAttribute(11);\n"
"  ctx.beginPath();\n"
"  ctx.fillRect((pos >> 16), (pos & 65535), size >> 16, size & 65535)"
"$/\n}\n"
"void pushAttribute(int index, int value) {\n"
"/$if (attributeStacks[index] == undefined)\n"
"    attributeStacks[index] = [];\n"
"\n"
"  attributeStacks[index].push(value)"
"$/\n}\n"
"void pushAttribute(int index, float value) {\n"
"/$this.pushAttribute(index, value)"
"$/\n}\n"
"void popAttribute(int index) {\n"
"/$attributeStacks[index].pop()"
"$/\n}\n"
"int getTextSize(String text) {\n"
"/$ctx.font = getAttribute(4) + \"px Arial\";\n"
"\n"
"  const textSize = ctx.measureText(text);\n"
"  const height = textSize.fontBoundingBoxAscent + textSize.fontBoundingBoxDescent;\n"
"  return (textSize.width << 16) | height"
"$/\n}\n"
"void displayText(String text, int pos, int size) {\n"
"/$let pos1 = [(pos >> 16), (pos & 0xFFFF)];\n"
"  let size1 = [(size >> 16), (size & 0xFFFF)];\n"
"  ctx.font = getAttribute(4) + \"px Arial\";\n"
"  ctx.fillStyle = toColor(getAttribute(10));\n"
"  ctx.globalAlpha = getAttribute(11);\n"
"  ctx.textBaseline = \"top\";\n"
"  ctx.fillText(text, pos1[0], pos1[1])"
"$/\n}\n"
"\n"*/
"void Timer(int timeoutMillis) {\n"
"/$setTimeout(function() {\n"
"    ReturnHandler_();\n"
"    _refresh();\n"
"  }, timeoutMillis)$/"
"\n"
//"  void reset() {}\n"
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

extern std::stringstream s;

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
  std::cout << s.str();
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
