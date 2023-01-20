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
//> Representing Code generate-ast
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void writeHeader(FILE *file) {
  fprintf(file, "/*\n");
  fprintf(file, " * The QED Programming Language\n");
  fprintf(file, " * Copyright (C) 2022  Hocus Codus Software inc.\n");
  fprintf(file, " */\n");
}

char *strtok2(const char *str, const char *delim) {
  static char lowercaseStr[512];

  if (str != NULL)
    strcpy(lowercaseStr, str);

  return (strtok(str != NULL ? lowercaseStr : NULL, delim));
}

char **split(char *str, char *sep) {
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
    fprintf(file, "#include \"chunk.hpp\"\n\n");
    fprintf(file, "#include \"object.hpp\"\n");
    fprintf(file, "#include \"scanner.hpp\"\n\n");

    // The enum types for AST classes.
    fprintf(file, "typedef enum {\n");
    for (int index = 0; types[index] != NULL; index++) {
      char *type = trim(strtok2((char *) types[index], ":"));

      fprintf(file, "  %s%s\n", getEnum(type, baseName), types[index + 1] != NULL ? "," : "");
    }
    fprintf(file, "} %sType;\n\n", baseName);

    fprintf(file, "struct %sVisitor;\n\n", baseName);
    fprintf(file, "struct %s {\n", baseName);
    fprintf(file, "  %sType type;\n\n", baseName);
    fprintf(file, "  %s(%sType type);\n\n", baseName, baseName);
    fprintf(file, "  virtual void accept(%sVisitor *visitor) = 0;\n", baseName);
    fprintf(file, "};\n\n", baseName);
    defineVisitor(file, baseName, types);

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

  void defineVisitor(FILE *file, char *baseName, const char *types[]) {
    for (int index = 0; types[index] != NULL; index++) {
      char *type = (char *) types[index];
      char *typeName = trim(strtok2(type, ":"));

      fprintf(file, "struct %s%s;\n", typeName, baseName);
    }

    fprintf(file, "\nstruct %sVisitor {\n", baseName);
    fprintf(file, "  template <typename T> T accept(%s *%s, T buf = T()) {\n", baseName, toLowerCase(baseName));
    fprintf(file, "    static T _buf;\n\n");
    fprintf(file, "    _buf = buf;\n\n");
    fprintf(file, "    if (%s != NULL)\n", toLowerCase(baseName));
    fprintf(file, "      %s->accept(this);\n\n", toLowerCase(baseName));
    fprintf(file, "    return _buf;\n", toLowerCase(baseName));
    fprintf(file, "  }\n\n");
    fprintf(file, "  template <typename T> void set(T buf) {\n");
    fprintf(file, "    accept(NULL, buf);\n");
    fprintf(file, "  }\n\n");

    for (int index = 0; types[index] != NULL; index++) {
      char *type = (char *) types[index];
      char *typeName = trim(strtok2(type, ":"));
      fprintf(file, "  virtual void visit%s%s(%s%s *%s) = 0;\n", typeName, baseName, typeName, baseName, toLowerCase(baseName));
    }

    fprintf(file, "};\n\n");
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

    // Visitor pattern.
    fprintf(file, "  void accept(%sVisitor *visitor);\n", baseName);
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

    // Visitor pattern.
    fprintf(file, "\nvoid %s%s::accept(%sVisitor *visitor) {\n", className, baseName, baseName);
    fprintf(file, "  return visitor->visit%s%s(this);\n", className, baseName);
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
    "Variable    : Token name, int8_t index, bool upvalueFlag",
    "UIAttribute : Token name, Expr* handler, int _uiIndex, int _index",
    "UIDirective : int childDir, int attCount, UIAttributeExpr** attributes, UIDirectiveExpr* previous, UIDirectiveExpr* lastChild, int viewIndex, int _layoutIndexes[NUM_DIRS]",
    "Assign      : VariableExpr* varExp, Token op, Expr* value",
    "Binary      : Expr* left, Token op, Expr* right, OpCode opCode, bool notFlag",
    "Grouping    : Token name, int count, Expr** expressions, int popLevels, Expr* ui",
    "Call        : Expr* callee, Token paren, int count, Expr** arguments, bool newFlag, Expr* handler",
    "Declaration : Type type, Token name, Expr* initExpr",
    "Function    : Type type, Token name, int count, Expr** params, Expr* body, ObjFunction* function",
    "Get         : Expr* object, Token name, int index",
    "List        : int count, Expr** expressions, ExprType listType, ObjFunction* function",
    "Literal     : ValueType type, As as",
    "Logical     : Expr* left, Token op, Expr* right",
    "Opcode      : OpCode op, Expr* right",
    "Return      : Token keyword, Expr* value",
    "Set         : Expr* object, Token name, Token op, Expr* value, int index",
    "Statement   : Expr* expr",
    "Super       : Token keyword, Token method",
    "Ternary     : Token op, Expr* left, Expr* middle, Expr* right",
    "This        : Token keyword",
    "Type        : Type type",
    "Unary       : Token op, Expr* right",
    "Swap        : Expr* _expr",
    NULL
  };
  GenerateAst().defineAst(outputDir, "Expr", array1);
/*
  const char *array2[] = {
    "Block      : Stmt** statements",
    "Class      : Token name, Expr.Variable superclass, Stmt::Function** methods",
    "Expression : Expr* expression",
    "Function   : Token name, Token* params, Stmt** body",
    "If         : Expr* condition, Stmt* thenBranch, Stmt* elseBranch",
    "Print      : Expr* expression",
    "Return     : Token keyword, Expr* value",
    "Var        : Token name, Expr* initializer",
    "While      : Expr* condition, Stmt* body",
    NULL
  };

  GenerateAst().defineAst(outputDir, "Stmt", array2);*/
}
