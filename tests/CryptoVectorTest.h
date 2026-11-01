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

#ifndef SH_CRYPTOVECTORTEST_H
#define SH_CRYPTOVECTORTEST_H

#include <string>
#include <vector>

#include "UnitTest.h"
#include "TestSuite.h"

/**
 * \class CryptoVectorTest
 * \brief checks MHashPP, MHashKeyGen and MCryptPP against vectors captured from the real libmhash and libmcrypt
 *
 * The vectors are in vectors/crypto_vectors.txt, written by
 * vectors/gen_crypto_vectors.c, which also describes the format.
 **/
class CryptoVectorTest : public UnitTest {
	public:
	CryptoVectorTest (TestSuite* s) ;

	void setup (void) ;

	void testMD5 (void) ;
	void testCRC32 (void) ;
	void testKeyGen (void) ;
	void testParameters (void) ;
	void testCiphers (void) ;
	void testSelfTests (void) ;

	private:
	/// the lines of the vectors file, split at spaces
	std::vector<std::vector<std::string> > Lines ;

	std::vector<std::vector<std::string> > getLines (const std::string& type) const ;
	static std::vector<BYTE> fromHex (const std::string& s) ;
	static std::string toHex (const std::vector<BYTE>& v) ;
} ;

#endif // ndef SH_CRYPTOVECTORTEST_H
