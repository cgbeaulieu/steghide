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

#include <fstream>
#include <iostream>
#include <sstream>

#include "CryptoVectorTest.h"
#include "EncryptionAlgorithm.h"
#include "EncryptionMode.h"
#include "MCryptPP.h"
#include "MHashKeyGen.h"
#include "MHashPP.h"
#include "mcrypt/mcrypt_algorithm.h"

#define VECTORS "vectors/crypto_vectors.txt"

CryptoVectorTest::CryptoVectorTest (TestSuite* s)
	: UnitTest ("CryptoVector", s)
{
	ADDTESTCATEGORY (CryptoVectorTest, testMD5) ;
	ADDTESTCATEGORY (CryptoVectorTest, testCRC32) ;
	ADDTESTCATEGORY (CryptoVectorTest, testKeyGen) ;
	ADDTESTCATEGORY (CryptoVectorTest, testParameters) ;
	ADDTESTCATEGORY (CryptoVectorTest, testCiphers) ;
	ADDTESTCATEGORY (CryptoVectorTest, testSelfTests) ;
}

void CryptoVectorTest::setup ()
{
	UnitTest::setup() ;
	Lines.clear() ;
	std::ifstream in (VECTORS) ;
	std::string line ;
	while (std::getline (in, line)) {
		if (line.empty() || line[0] == '#') {
			continue ;
		}
		std::istringstream fields (line) ;
		std::vector<std::string> v ;
		std::string f ;
		while (fields >> f) {
			v.push_back (f == "-" ? std::string() : f) ;
		}
		Lines.push_back (v) ;
	}
	if (Lines.empty()) {
		std::cerr << std::endl << "could not read " << VECTORS << std::endl ;
	}
}

std::vector<std::vector<std::string> > CryptoVectorTest::getLines (const std::string& type) const
{
	std::vector<std::vector<std::string> > retval ;
	for (std::vector<std::vector<std::string> >::const_iterator l = Lines.begin() ; l != Lines.end() ; l++) {
		if ((*l)[0] == type) {
			retval.push_back (*l) ;
		}
	}
	return retval ;
}

std::vector<BYTE> CryptoVectorTest::fromHex (const std::string& s)
{
	std::vector<BYTE> retval ;
	for (unsigned int i = 0 ; i + 1 < s.size() ; i += 2) {
		retval.push_back ((BYTE) std::stoul (s.substr (i, 2), NULL, 16)) ;
	}
	return retval ;
}

std::string CryptoVectorTest::toHex (const std::vector<BYTE>& v)
{
	static const char digits[] = "0123456789abcdef" ;
	std::string retval ;
	for (std::vector<BYTE>::const_iterator i = v.begin() ; i != v.end() ; i++) {
		retval += digits[*i >> 4] ;
		retval += digits[*i & 0x0f] ;
	}
	return retval ;
}

static bool check (const std::string& what, const std::string& result, const std::string& shouldbe)
{
	if (result != shouldbe) {
		std::cerr << std::endl << what << ": got " << result << ", should be " << shouldbe << std::endl ;
		return false ;
	}
	return true ;
}

void CryptoVectorTest::testMD5 ()
{
	std::vector<std::vector<std::string> > lines = getLines ("md5") ;
	bool ok = !lines.empty() ;
	for (std::vector<std::vector<std::string> >::const_iterator l = lines.begin() ; l != lines.end() ; l++) {
		std::vector<BYTE> in = fromHex ((*l)[1]) ;
		MHashPP hash (MHashPP::MD5) ;
		for (std::vector<BYTE>::const_iterator i = in.begin() ; i != in.end() ; i++) {
			hash << *i ;
		}
		hash << MHashPP::endhash ;
		ok = check ("md5 of " + (*l)[1], toHex (hash.getHashBytes()), (*l)[2]) && ok ;
	}
	addTestResult (ok) ;
}

void CryptoVectorTest::testCRC32 ()
{
	std::vector<std::vector<std::string> > lines = getLines ("crc32") ;
	bool ok = !lines.empty() ;
	for (std::vector<std::vector<std::string> >::const_iterator l = lines.begin() ; l != lines.end() ; l++) {
		std::vector<BYTE> in = fromHex ((*l)[1]) ;
		MHashPP hash (MHashPP::CRC32) ;
		for (std::vector<BYTE>::const_iterator i = in.begin() ; i != in.end() ; i++) {
			hash << *i ;
		}
		hash << MHashPP::endhash ;
		ok = check ("crc32 of " + (*l)[1], toHex (hash.getHashBytes()), (*l)[2]) && ok ;
	}
	addTestResult (ok) ;
}

void CryptoVectorTest::testKeyGen ()
{
	std::vector<std::vector<std::string> > lines = getLines ("keygen") ;
	bool ok = !lines.empty() ;
	for (std::vector<std::vector<std::string> >::const_iterator l = lines.begin() ; l != lines.end() ; l++) {
		std::vector<BYTE> pw = fromHex ((*l)[2]) ;
		MHashKeyGen keygen (std::stoul ((*l)[1])) ;
		std::vector<BYTE> key = keygen.createKey (std::string (pw.begin(), pw.end())) ;
		ok = check ("keygen " + (*l)[1] + " " + (*l)[2], toHex (key), (*l)[3]) && ok ;
	}
	// steghide 0.5.1 cut the password off at a '\0'
	MHashKeyGen keygen (32) ;
	ok = check ("keygen with '\\0'", toHex (keygen.createKey (std::string ("ab\0cd", 5))), toHex (keygen.createKey ("ab"))) && ok ;
	addTestResult (ok) ;
}

void CryptoVectorTest::testParameters ()
{
	std::vector<std::vector<std::string> > lines = getLines ("algorithm") ;
	bool ok = (lines.size() == MCryptPP::getListAlgorithms().size()) ;
	for (std::vector<std::vector<std::string> >::const_iterator l = lines.begin() ; l != lines.end() ; l++) {
		const std::string& name = (*l)[1] ;
		bool block = ((*l)[4] == "1") ;
		MCryptPP crypto (EncryptionAlgorithm (name), EncryptionMode (block ? "cbc" : "stream")) ;
		ok = check (name + " block size", std::to_string (crypto.getBlockSize()), (*l)[2]) && ok ;
		ok = check (name + " key size", std::to_string (crypto.getKeySize()), (*l)[3]) && ok ;
		ok = check (name + " supports stream", std::to_string (MCryptPP::AlgoSupportsMode (EncryptionAlgorithm (name), EncryptionMode ("stream"))), block ? "0" : "1") && ok ;
	}
	lines = getLines ("mode") ;
	ok = (lines.size() == MCryptPP::getListModes().size()) && ok ;
	for (std::vector<std::vector<std::string> >::const_iterator l = lines.begin() ; l != lines.end() ; l++) {
		const std::string& name = (*l)[1] ;
		bool blockalgomode = ((*l)[3] == "1") ;
		MCryptPP crypto (EncryptionAlgorithm (blockalgomode ? "rijndael-128" : "arcfour"), EncryptionMode (name)) ;
		ok = check (name + " has an iv", std::to_string (crypto.hasIV()), (*l)[4]) && ok ;
	}
	addTestResult (ok) ;
}

void CryptoVectorTest::testCiphers ()
{
	std::vector<std::vector<std::string> > lines = getLines ("cipher") ;
	std::vector<std::string> algos = MCryptPP::getListAlgorithms() ;
	for (std::vector<std::string>::const_iterator a = algos.begin() ; a != algos.end() ; a++) {
		bool ok = false ;
		for (std::vector<std::vector<std::string> >::const_iterator l = lines.begin() ; l != lines.end() ; l++) {
			if ((*l)[1] != *a) {
				continue ;
			}
			ok = true ;
			std::vector<BYTE> key = fromHex ((*l)[3]), iv = fromHex ((*l)[4]) ;
			std::vector<BYTE> plain = fromHex ((*l)[5]), cipher = fromHex ((*l)[6]) ;
			std::string what = (*l)[1] + " " + (*l)[2] + " with a " + std::to_string (key.size()) + " byte key, " + std::to_string (plain.size()) + " bytes" ;
			EncryptionAlgorithm algo (*a) ;
			EncryptionMode mode ((*l)[2]) ;
			MCryptPP crypto (algo, mode) ;

			std::vector<BYTE> data = plain ;
			crypto.crypt (key, iv.empty() ? NULL : iv.data(), data, true) ;
			bool encok = check (what + " encrypted", toHex (data), (*l)[6]) ;

			// decrypt the recorded ciphertext, not our own
			data = cipher ;
			crypto.crypt (key, iv.empty() ? NULL : iv.data(), data, false) ;
			bool decok = check (what + " decrypted", toHex (data), (*l)[5]) ;

			if (!(encok && decok)) {
				ok = false ;
				break ;
			}
		}
		addTestResult (ok) ;
	}
}

void CryptoVectorTest::testSelfTests ()
{
	// the test each libmcrypt module carries
	const mcrypt_algorithm* algos[] = {
		&mcrypt_cast_128, &mcrypt_gost, &mcrypt_rijndael_128, &mcrypt_twofish,
		&mcrypt_arcfour, &mcrypt_cast_256, &mcrypt_loki97, &mcrypt_rijndael_192,
		&mcrypt_saferplus, &mcrypt_wake, &mcrypt_des, &mcrypt_rijndael_256,
		&mcrypt_serpent, &mcrypt_xtea, &mcrypt_blowfish, &mcrypt_enigma,
		&mcrypt_rc2, &mcrypt_tripledes
	} ;
	bool ok = true ;
	for (const mcrypt_algorithm* a : algos) {
		if (a->self_test() != 0) {
			std::cerr << std::endl << a->name << " failed its self-test" << std::endl ;
			ok = false ;
		}
	}
	addTestResult (ok) ;
}
