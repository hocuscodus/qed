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