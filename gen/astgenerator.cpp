/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
//> Representing Code generate-ast
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PASSES_DEF \
    PASS_DEF( void cleanExprs() )  \
    PASS_DEF( void astPrint() )  \
    PASS_DEF( Expr *toCps(K k) )  \
    PASS_DEF( Type resolve(Parser &parser) )  \
    PASS_DEF( void toCode(Parser &parser, ObjFunction *function) )

void writeHeader(FILE *file) {
  fprintf(file, "/*\n");
  fprintf(file, " * The QED Programming Language\n");
  fprintf(file, " * Copyright (C) 2022-2023  Hocus Codus Software inc.\n");
  fprintf(file, " */\n");
}

char *strtok2(const char *str, const char *delim) {
  static char lowercaseStr[512];

  if (str != NULL)
    strcpy(lowercaseStr, str);

  return (strtok(str != NULL ? lowercaseStr : NULL, delim));
}

char **split(char *str, const char *sep) {
    static char string[2048];
    static char *list[100];

    int count = 0;

    strcpy(string, str);
    // Extract the first token
    char *token = strtok2(string, sep);

    // loop through the string to extract all other tokens
    while( token != NULL ) {
      list[count++] = token; //printing each token
      token = strtok2(NULL, sep);
    }

    list[count] = NULL;
    return list;
}

char *toLowerCase(char *str) {
  static char lowercaseStr[512];

  for(int i = 0; ; i++){
    lowercaseStr[i] = tolower(str[i]);

    if (str[i] == '\0')
      break;
  }

  return lowercaseStr;
}

char *toUpperCase(char *str) {
  static char uppercaseStr[512];

  for(int i = 0; ; i++){
    uppercaseStr[i] = toupper(str[i]);

    if (str[i] == '\0')
      break;
  }

  return uppercaseStr;
}


char *trim(char *str) {
  static char trimmedStr[512];
  int length = strlen(str);

  if (length != 0) {
    strcpy(trimmedStr, str);
    str = &trimmedStr[length - 1];
    while (memchr("\r\n\t ", *str, 4) != NULL)
      *str-- = '\0';
    str = trimmedStr;
    while (memchr("\r\n\t ", *str, 4) != NULL)
      str++;
  }

  return str;
}

char *getEnum(char *type, char *baseName) {
  static char enumStr[128];

  strcpy(enumStr, toUpperCase(baseName));
  strcat(enumStr, "_");
  strcat(enumStr, toUpperCase(type));

  return enumStr;
}

class GenerateAst {
public:
  void defineAst(const char *outputDir, char *baseName, const char *types[]) {
    char path[256];

    // HPP
    sprintf(path, "%s/%s.hpp", outputDir, toLowerCase(baseName));
    FILE *file = fopen(path, "w");

    writeHeader(file);
    fprintf(file, "#ifndef %s_h\n", toLowerCase(baseName));
    fprintf(file, "#define %s_h\n\n", toLowerCase(baseName));
    fprintf(file, "#include \"chunk.hpp\"\n");
    fprintf(file, "#include \"compiler.hpp\"\n\n");
    fprintf(file, "struct ObjFunction;\n\n");
    fprintf(file, "struct ObjCallable;\n\n");

    // The enum types for AST classes.
    fprintf(file, "typedef enum {\n");
    for (int index = 0; types[index] != NULL; index++) {
      char *type = trim(strtok2((char *) types[index], ":"));

      fprintf(file, "  %s%s\n", getEnum(type, baseName), types[index + 1] != NULL ? "," : "");
    }
    fprintf(file, "} %sType;\n\n", baseName);

    fprintf(file, "struct %s {\n", baseName);
    fprintf(file, "  %sType type;\n\n", baseName);
    fprintf(file, "  %s(%sType type);\n\n", baseName, baseName);

    fprintf(file, "  void destroy();\n\n");

#define PASS_DEF( pass ) fprintf(file, "  virtual " #pass " = 0;\n");
PASSES_DEF
#undef PASS_DEF

    fprintf(file, "};\n\n");

    // The AST classes.
    for (int index = 0; types[index] != NULL; index++) {
      char *type = (char *) types[index];
      char className[128];
      strcpy(className, trim(strtok2(type, ":")));
      char fieldList[256];
      strcpy(fieldList, trim(strtok2(NULL, ":"))); // [robust]
      declareType(file, baseName, className, fieldList);
    }

    fprintf(file, "#endif\n");
    fclose(file);

    // CPP
    sprintf(path, "%s/%s.cpp", outputDir, toLowerCase(baseName));
    file = fopen(path, "w");

    writeHeader(file);
    fprintf(file, "#include \"%s.hpp\"\n", toLowerCase(baseName));

    // Base class constructor
    fprintf(file, "\n%s::%s(%sType type) {\n", baseName, baseName, baseName);
    fprintf(file, "  this->type = type;\n");
    fprintf(file, "}\n\n");

    fprintf(file, "void %s::destroy() {\n", baseName);
    fprintf(file, "  cleanExprs();\n");
    fprintf(file, "  delete this;\n");
    fprintf(file, "}\n");

    // The AST classes.
    for (int index = 0; types[index] != NULL; index++) {
      char *type = (char *) types[index];
      char className[128];
      strcpy(className, trim(strtok2(type, ":")));
      char fieldList[256];
      strcpy(fieldList, trim(strtok2(NULL, ":"))); // [robust]
      defineType(file, baseName, className, fieldList);
    }

    fclose(file);
  }

  void declareType(FILE *file, char *baseName, char *className, char *fieldList) {
    fprintf(file, "struct %s%s : public %s {\n", className, baseName, baseName);

    // Fields.
    char **fields = split(fieldList, ",");
    for (int index = 0; fields[index] != NULL; index++) {
      char field[128];
      strcpy(field, trim(fields[index]));
      fprintf(file, "  %s;\n", field);
    }

    // Constructor.
    fprintf(file, "\n  %s%s(", className, baseName);
    for (int index = 0; fields[index] != NULL; index++) {
      char field[128];
      strcpy(field, trim(fields[index]));

      char *term = strchr(field, ' ') + 1;

      if (term[0] != '_')
        fprintf(file, index == 0 ? "%s" : ", %s", field);
    }
    fprintf(file, ");\n");

#define PASS_DEF( pass ) fprintf(file, "  " #pass ";\n");
PASSES_DEF
#undef PASS_DEF

    fprintf(file, "};\n\n");
  }

  void defineType(FILE *file, char *baseName, char *className, char *fieldList) {
    char **fields = split(fieldList, ",");

    // Constructor.
    fprintf(file, "\n%s%s::%s%s(", className, baseName, className, baseName, fieldList, baseName, getEnum(className, baseName));
    for (int index = 0; fields[index] != NULL; index++) {
      char field[128];
      strcpy(field, trim(fields[index]));

      char *term = strchr(field, ' ') + 1;

      if (term[0] != '_')
        fprintf(file, index == 0 ? "%s" : ", %s", field);
    }
    fprintf(file, ") : %s(%s) {\n", baseName, getEnum(className, baseName));

    for (int index = 0; fields[index] != NULL; index++) {
      char field[128];
      strcpy(field, trim(fields[index]));
      strtok2(field, " ");
      char *name = strtok2(NULL, " ");

      if (name[0] != '_')
        fprintf(file, "  this->%s = %s;\n", name, name);
    }

    fprintf(file, "}\n");
  }
};

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Usage: generate_ast <output directory>\n");
    exit(64);
  }

  const char *outputDir = argv[1];

  const char *array1[] = {
    "Type        : Token name, bool functionFlag, bool noneFlag, int numDim, int index, Declaration* declaration",
    "UIAttribute : Token name, Expr* handler, int _uiIndex, int _index",
    "UIDirective : int childDir, int attCount, UIAttributeExpr** attributes, UIDirectiveExpr* previous, UIDirectiveExpr* lastChild, int viewIndex, bool childrenViewFlag, int _layoutIndexes[NUM_DIRS], long _eventFlags",
    "Assign      : Expr* varExp, Token op, Expr* value",
    "Binary      : Expr* left, Token op, Expr* right",
    "Cast        : Expr* typeExpr, Expr* expr, Type _srcType, Type _dstType",
    "Grouping    : Token name, Expr* _body, Compiler _compiler",
    "Array       : int count, Expr** expressions, ObjFunction* function",
    "Call        : bool newFlag, Expr* callee, Token paren, int count, Expr** arguments, Expr* handler",
    "ArrayElement: Expr* callee, Token bracket, int count, Expr** indexes",
    "Declaration : Expr* typeExpr, Token name, Expr* initExpr, Declaration* _declaration",
    "Function    : Expr* typeExpr, Token name, int arity, Expr** params, GroupingExpr* body, Expr* ui, ObjFunction _function, Declaration* _declaration",
    "Get         : Expr* object, Token name, int index",
    "If          : Expr* condition, Expr* thenBranch, Expr* elseBranch",
    "List        : int count, Expr** expressions",
    "Literal     : ValueType type, As as",
    "Logical     : Expr* left, Token op, Expr* right",
    "Reference   : Token name, Type returnType, Declaration* _declaration",
    "Return      : Token keyword, Expr* value",
    "Set         : Expr* object, Token name, Token op, Expr* value, int index",
    "Ternary     : Token op, Expr* left, Expr* middle, Expr* right",
    "This        : Token keyword",
    "Unary       : Token op, Expr* right",
    "While       : Expr* condition, Expr* body",
    "Swap        : Expr* _expr",
    "Native      : Token nativeCode",
    NULL
  };
  GenerateAst().defineAst(outputDir, "Expr", array1);
}
