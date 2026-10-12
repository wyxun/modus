#ifndef MSHELL_TEST_UTIL_DEBUG_H
#define MSHELL_TEST_UTIL_DEBUG_H

#include <stdint.h>

#define MLOG_MASK_E (1U << 0)
#define MLOG_MASK_W (1U << 1)
#define MLOG_MASK_I (1U << 2)
#define MLOG_MASK_D (1U << 3)
#define MLOG_MASK_T (1U << 4)
#define MLOG(level, ...) util_debug_Printf(__VA_ARGS__)
#define MLOGF(level, ...) util_debug_Printf(__VA_ARGS__)

extern uint8_t g_chGLogMask;
void util_debug_Printf(const char *format, ...);

#endif
