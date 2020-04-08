#pragma once

#include <inttypes.h>

#define __va_aligned_type uintptr_t

#define va_list __va_aligned_type*

#define va_start(ap, last) ((ap) = (__va_aligned_type*)&(last) + 1)

#define va_end(ap) ((ap) = NULL)

#define va_arg(ap, type) (*(type*)(ap)++)
