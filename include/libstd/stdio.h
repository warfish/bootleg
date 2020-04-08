#pragma once

#include <stdarg.h>

typedef void(FILE)(char);

extern FILE* stdout;
extern FILE* stderr;

int vfprintf(FILE* filp, const char* format, va_list ap);
int fprintf(FILE* filp, const char* format, ...);

int vprintf(const char* format, va_list ap);
int printf(const char* format, ...);
