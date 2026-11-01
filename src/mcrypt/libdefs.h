/*
 * Replaces libmcrypt's lib/libdefs.h for the cipher sources in this
 * directory: the fixed-width types, the byte order, and the C headers they
 * expect. See README.md.
 */

#ifndef LIBDEFS_H
#define LIBDEFS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define WORDS_BIGENDIAN 1
#endif

typedef uint32_t word32;
typedef int32_t sword32;
typedef uint16_t word16;
typedef uint8_t byte;

/* libmcrypt exported module symbols from Windows DLLs with this */
#define WIN32DLL_DEFINE

#endif /* LIBDEFS_H */
