/*
 * steghide 0.5.1 - a steganography program
 * Copyright (C) 1999-2003 Stefan Hetzl <shetzl@chello.at>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <string>

#include "BitString.h"
#include "common.h"
#include "error.h"
#include "MHashPP.h"

namespace {

// CRC-32, polynomial 0x04C11DB7, most significant bit first
struct CrcTable {
	UWORD32 Entries[256] ;
	constexpr CrcTable () : Entries()
	{
		for (UWORD32 i = 0 ; i < 256 ; i++) {
			UWORD32 c = i << 24 ;
			for (int j = 0 ; j < 8 ; j++) {
				c = (c & 0x80000000) ? (c << 1) ^ 0x04C11DB7 : (c << 1) ;
			}
			Entries[i] = c ;
		}
	}
} ;
constexpr CrcTable Crc32Table ;

// RFC 1321
const UWORD32 Md5T[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
} ;

const unsigned short Md5S[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
} ;

inline UWORD32 rotl (UWORD32 x, unsigned short n)
{
	return (x << n) | (x >> (32 - n)) ;
}

}

MHashPP::MHashPP ()
{
	hashing = false ;
	HashBytesValid = false ;
}

MHashPP::MHashPP (Algorithm a)
{
	init (a) ;
}

void MHashPP::init (Algorithm a)
{
	Algo = a ;
	switch (Algo) {
		case CRC32:
		Crc = 0xFFFFFFFF ;
		break ;

		case MD5:
		Md5State[0] = 0x67452301 ;
		Md5State[1] = 0xefcdab89 ;
		Md5State[2] = 0x98badcfe ;
		Md5State[3] = 0x10325476 ;
		Md5Length = 0 ;
		break ;
	}
	hashing = true ;
	HashBytesValid = false ;
}

void MHashPP::update (const BYTE* data, unsigned long n)
{
	myassert (hashing) ;
	switch (Algo) {
		case CRC32:
		for (unsigned long i = 0 ; i < n ; i++) {
			Crc = (Crc << 8) ^ Crc32Table.Entries[(Crc >> 24) ^ data[i]] ;
		}
		break ;

		case MD5:
		for (unsigned long i = 0 ; i < n ; i++) {
			Md5Block[Md5Length % 64] = data[i] ;
			Md5Length++ ;
			if (Md5Length % 64 == 0) {
				md5Transform (Md5Block) ;
			}
		}
		break ;
	}
}

void MHashPP::md5Transform (const BYTE* block)
{
	UWORD32 x[16] ;
	for (unsigned short i = 0 ; i < 16 ; i++) {
		x[i] = block[4 * i] | (block[4 * i + 1] << 8) | (block[4 * i + 2] << 16) | ((UWORD32) block[4 * i + 3] << 24) ;
	}

	UWORD32 a = Md5State[0], b = Md5State[1], c = Md5State[2], d = Md5State[3] ;
	for (unsigned short i = 0 ; i < 64 ; i++) {
		UWORD32 f = 0 ;
		unsigned short g = 0 ;
		if (i < 16) {
			f = (b & c) | (~b & d) ;
			g = i ;
		}
		else if (i < 32) {
			f = (d & b) | (~d & c) ;
			g = (5 * i + 1) % 16 ;
		}
		else if (i < 48) {
			f = b ^ c ^ d ;
			g = (3 * i + 5) % 16 ;
		}
		else {
			f = c ^ (b | ~d) ;
			g = (7 * i) % 16 ;
		}
		UWORD32 tmp = d ;
		d = c ;
		c = b ;
		b = b + rotl (a + f + Md5T[i] + x[g], Md5S[i]) ;
		a = tmp ;
	}
	Md5State[0] += a ;
	Md5State[1] += b ;
	Md5State[2] += c ;
	Md5State[3] += d ;
}

const std::vector<BYTE>& MHashPP::end ()
{
	myassert (hashing) ;
	switch (Algo) {
		case CRC32: {
		UWORD32 crc = ~Crc ;
		HashBytes = std::vector<BYTE> (4) ;
		for (unsigned short i = 0 ; i < 4 ; i++) {
			HashBytes[i] = (BYTE) (crc >> (8 * i)) ;
		}
		break ; }

		case MD5: {
		unsigned long long nbits = 8 * Md5Length ;
		BYTE pad = 0x80 ;
		update (&pad, 1) ;
		pad = 0x00 ;
		while (Md5Length % 64 != 56) {
			update (&pad, 1) ;
		}
		BYTE length[8] ;
		for (unsigned short i = 0 ; i < 8 ; i++) {
			length[i] = (BYTE) (nbits >> (8 * i)) ;
		}
		update (length, 8) ;

		HashBytes = std::vector<BYTE> (16) ;
		for (unsigned short i = 0 ; i < 16 ; i++) {
			HashBytes[i] = (BYTE) (Md5State[i / 4] >> (8 * (i % 4))) ;
		}
		break ; }
	}
	hashing = false ;
	HashBytesValid = true ;
	return HashBytes ;
}

unsigned int MHashPP::getHashSize (void)
{
	myassert (hashing) ;
	return (Algo == MD5) ? 16 : 4 ;
}

MHashPP& MHashPP::operator<< (std::string v)
{
	update ((const BYTE*) v.data(), v.size()) ;
	return *this ;
}

MHashPP& MHashPP::operator<< (BitString v)
{
	myassert (v.getLength() % 8 == 0) ;

	unsigned long n = v.getLength() / 8 ;
	for (unsigned int i = 0 ; i < n ; i++) {
		(*this) << (BYTE) v.getValue (8 * i, 8) ;
	}

	return *this ;
}

MHashPP& MHashPP::operator<< (BYTE v)
{
	update (&v, 1) ;
	return *this ;
}

MHashPP& MHashPP::operator<< (MHashPP::Command c)
{
	switch (c) {
		case endhash:
		HashBytes = end() ;
		break ;

		default:
		myassert (0) ;
		break ;
	}
	return *this ;
}

BitString MHashPP::getHashBits ()
{
	myassert (HashBytesValid) ;
	return BitString (HashBytes) ;
}

const std::vector<BYTE>& MHashPP::getHashBytes()
{
	myassert (HashBytesValid) ;
	return HashBytes ;
}
