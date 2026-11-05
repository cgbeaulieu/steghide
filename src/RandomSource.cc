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

#include <cerrno>
#include <cstring>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/random.h>
#endif

#include "BitString.h"
#include "common.h"
#include "error.h"
#include "msg.h"
#include "RandomSource.h"

// the global RandomSource object
RandomSource RndSrc ;

RandomSource::RandomSource ()
	: RandomBytePos(8), RandomByte(0), Buffer(), BufferPos(Buffer.size())
{
}

BYTE RandomSource::getByte ()
{
	if (BufferPos == Buffer.size()) {
		// with NORANDOM, Buffer keeps the zeros it was initialized with
#ifndef NORANDOM
		if (getentropy (Buffer.data(), Buffer.size()) != 0) {
			throw SteghideError (_("could not get random data from the operating system: %s."), strerror (errno)) ;
		}
#endif
		BufferPos = 0 ;
	}
	return Buffer[BufferPos++] ;
}

std::vector<BYTE> RandomSource::getBytes (unsigned int n)
{
	std::vector<BYTE> retval ;
	for (unsigned int i = 0 ; i < n ; i++) {
		retval.push_back (getByte()) ;
	}
	return retval ;
}

BitString RandomSource::getBits (unsigned int n)
{
	BitString retval ;
	BYTE rndbyte = 0 ;
	unsigned int bitsused = 8 ;
	while (n > 0) {
		if (bitsused == 8) {
			rndbyte = getByte() ;
			bitsused = 0 ;
		}
		retval.append ((BIT) (rndbyte & 1)) ;
		rndbyte = rndbyte >> 1 ;
		bitsused++ ;
		n-- ;
	}
	return retval ;
}

bool RandomSource::getBool()
{
	if (RandomBytePos == 8) {
		RandomByte = getByte() ;
		RandomBytePos = 0 ;
	}
	bool retval = (RandomByte & (1 << RandomBytePos)) ;
	RandomBytePos++ ;
	return retval ;
}

unsigned long RandomSource::getValue (unsigned long n)
{
	const unsigned long ceilvalue = 0x1000000UL ;
	myassert (n < ceilvalue) ;
	unsigned long value = ((unsigned long) (getByte() << 16)) |
						  ((unsigned long) (getByte() << 8)) |
						  ((unsigned long) getByte()) ;

	return (unsigned long) ((((double) value) / ((double) ceilvalue)) * ((double) n)) ;
}
