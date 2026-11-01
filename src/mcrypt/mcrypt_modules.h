/*
 * Replaces libmcrypt's lib/mcrypt_modules.h for the cipher sources in this
 * directory. See README.md.
 *
 * libmcrypt defined the rotations as macros that shift by 32 - (n & 31),
 * which is undefined behaviour when n & 31 is 0, and that do not parenthesize
 * n. cast-256 rotates by key-dependent amounts that are 0 mod 32 for some
 * keys. GCC turns the macro into a rotate instruction, so the files steghide
 * 0.5.1 wrote carry the rotate-by-0 result, but Clang may fold the undefined
 * shift into a different value. These are well-defined rotations with the
 * same results as the rotate instruction.
 */

#ifndef MCRYPT_MODULES_H
#define MCRYPT_MODULES_H

static inline word32 rotl32 (word32 x, word32 n)
{
	n &= 31 ;
	return n == 0 ? x : (x << n) | (x >> (32 - n)) ;
}

static inline word32 rotr32 (word32 x, word32 n)
{
	n &= 31 ;
	return n == 0 ? x : (x >> n) | (x << (32 - n)) ;
}

static inline word16 rotl16 (word16 x, word16 n)
{
	n &= 15 ;
	return n == 0 ? x : (word16) ((x << n) | (x >> (16 - n))) ;
}

static inline word16 rotr16 (word16 x, word16 n)
{
	n &= 15 ;
	return n == 0 ? x : (word16) ((x >> n) | (x << (16 - n))) ;
}

static inline word32 byteswap32 (word32 x)
{
	return ((x & 0x000000ffU) << 24) | ((x & 0x0000ff00U) << 8) | ((x & 0x00ff0000U) >> 8) | ((x & 0xff000000U) >> 24) ;
}

static inline word16 byteswap16 (word16 x)
{
	return (word16) ((x << 8) | (x >> 8)) ;
}

#define Bzero(x, y) memset(x, 0, y)

#endif /* MCRYPT_MODULES_H */
