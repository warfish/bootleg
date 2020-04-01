#pragma once

#include <stdio.h>

#define LOG_ERROR(msg, ...) { fprintf(stderr, msg, ## __VA_ARGS__); }
#define LOG_DEBUG(msg, ...) { fprintf(stderr, msg, ## __VA_ARGS__); }
