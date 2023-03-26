/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
/*#include <stdio.h>
#include <stdarg.h>
#include "logger.hpp"

Logger::Logger() {
  panicMode = false;
  hadError = false;
}

#define FORMAT_MESSAGE(fmt) \
  char message[256]; \
  va_list args; \
 \
  va_start (args, fmt); \
  vsnprintf(message, 256, fmt, args); \
  va_end(args);

void Logger::compilerError(const char *fmt, ...) {
  FORMAT_MESSAGE(fmt);
  previous.declareError(message);
}

void Logger::errorAt(Token *token, const char *fmt, ...) {
  if (panicMode)
    return;
  panicMode = true;
  FORMAT_MESSAGE(fmt);
  token->declareError(message);
  hadError = true;
}
#undef FORMAT_MESSAGE
*/