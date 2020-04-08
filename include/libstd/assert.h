#pragma once

#ifndef NDEBUG
    void _assert(const char* file, unsigned long line, const char* reason) __attribute__((noreturn));
#   define assert(cond) { if (!(cond)) { _assert(__FILE__, __LINE__, #cond); } }
#else
#   define assert(cond)
#endif
