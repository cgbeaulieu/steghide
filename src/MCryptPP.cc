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

#include <cstdlib>
#include <string>
#include <vector>

#include "BitString.h"
#include "EncryptionAlgorithm.h"
#include "EncryptionMode.h"
#include "MCryptPP.h"
#include "MHashKeyGen.h"
#include "RandomSource.h"
#include "common.h"
#include "error.h"
#include "mcrypt/mcrypt_algorithm.h"

namespace {

/// the algorithms in the order libmcrypt 2.5.8 lists them
const mcrypt_algorithm* const Algorithms[] = {
	&mcrypt_cast_128, &mcrypt_gost, &mcrypt_rijndael_128, &mcrypt_twofish,
	&mcrypt_arcfour, &mcrypt_cast_256, &mcrypt_loki97, &mcrypt_rijndael_192,
	&mcrypt_saferplus, &mcrypt_wake, &mcrypt_des, &mcrypt_rijndael_256,
	&mcrypt_serpent, &mcrypt_xtea, &mcrypt_blowfish, &mcrypt_enigma,
	&mcrypt_rc2, &mcrypt_tripledes
} ;

/// the modes in the order libmcrypt 2.5.8 lists them
const char* const Modes[] = { "cbc", "cfb", "ctr", "ecb", "ncfb", "nofb", "ofb", "stream" } ;

/// memory from calloc, as libmcrypt allocated key schedules and keys (aligned for any type)
class CallocBuffer {
	public:
	CallocBuffer (size_t n) : Ptr(calloc (1, n == 0 ? 1 : n))
		{ if (Ptr == NULL) throw SteghideError (_("could not allocate memory.")) ; } ;
	~CallocBuffer (void) { free (Ptr) ; } ;
	CallocBuffer (const CallocBuffer&) = delete ;
	CallocBuffer& operator= (const CallocBuffer&) = delete ;
	BYTE* get (void) const { return (BYTE*) Ptr ; } ;

	private:
	void* Ptr ;
} ;

/**
 * libmcrypt's eight modes, translated from the sources in modules/modes/ of
 * libmcrypt 2.5.8. Reg and Enc are the two registers each mode keeps (s_register and
 * enc_s_register, or the counter and the encrypted counter).
 *
 * libmcrypt kept state between mcrypt_generic calls so that a stream could
 * continue after a partial block. MCryptPP en- or decrypts everything in one
 * run, where only the last block can be partial, so that state is left out.
 **/
class ModeRunner {
	public:
	ModeRunner (const mcrypt_algorithm* a, void* ctx, EncryptionMode::IRep m, const BYTE* iv)
		: Algo(a), Ctx(ctx), M(m), BS(a->get_block_size()), Reg(BS), Enc(BS)
	{
		if (iv != NULL && M != EncryptionMode::ECB && M != EncryptionMode::STREAM) {
			copy (Reg.data(), iv, BS) ;
		}
	} ;

	void run (BYTE* data, unsigned int len, bool encrypt)
	{
		switch (M) {
			case EncryptionMode::ECB:
			requireBlock (len) ;
			for (unsigned int j = 0 ; j < len / BS ; j++) {
				if (encrypt) {
					Algo->block_encrypt (Ctx, data + j * BS) ;
				}
				else {
					Algo->block_decrypt (Ctx, data + j * BS) ;
				}
			}
			break ;

			case EncryptionMode::CBC:
			requireBlock (len) ;
			for (unsigned int j = 0 ; j < len / BS ; j++) {
				BYTE* block = data + j * BS ;
				if (encrypt) {
					xorInto (block, Reg.data(), BS) ;
					Algo->block_encrypt (Ctx, block) ;
					copy (Reg.data(), block, BS) ;
				}
				else {
					copy (Enc.data(), block, BS) ;
					Algo->block_decrypt (Ctx, block) ;
					xorInto (block, Reg.data(), BS) ;
					copy (Reg.data(), Enc.data(), BS) ;
				}
			}
			break ;

			case EncryptionMode::CFB:	// 8 bit cfb
			case EncryptionMode::OFB:	// 8 bit ofb
			for (unsigned int j = 0 ; j < len ; j++) {
				copy (Enc.data(), Reg.data(), BS) ;
				Algo->block_encrypt (Ctx, Enc.data()) ;
				BYTE in = data[j] ;
				data[j] ^= Enc[0] ;
				for (unsigned int i = 0 ; i < BS - 1 ; i++) {
					Reg[i] = Reg[i + 1] ;
				}
				if (M == EncryptionMode::OFB) {
					Reg[BS - 1] = Enc[0] ;
				}
				else {
					Reg[BS - 1] = encrypt ? data[j] : in ;	// the ciphertext byte
				}
			}
			break ;

			case EncryptionMode::NCFB:	// n bit cfb
			case EncryptionMode::NOFB:	// n bit ofb
			case EncryptionMode::CTR:
			for (unsigned int j = 0 ; j < len / BS ; j++) {
				feedback (data + j * BS, BS, encrypt) ;
			}
			if (len % BS > 0) {
				feedback (data + (len / BS) * BS, len % BS, encrypt) ;
			}
			break ;

			case EncryptionMode::STREAM:
			if (encrypt) {
				Algo->stream_encrypt (Ctx, data, len) ;
			}
			else {
				Algo->stream_decrypt (Ctx, data, len) ;
			}
			break ;
		}
	} ;

	private:
	const mcrypt_algorithm* Algo ;
	void* Ctx ;
	EncryptionMode::IRep M ;
	unsigned int BS ;
	std::vector<BYTE> Reg ;
	std::vector<BYTE> Enc ;

	static void xorInto (BYTE* dst, const BYTE* src, unsigned int n)
		{ for (unsigned int i = 0 ; i < n ; i++) dst[i] ^= src[i] ; } ;
	static void copy (BYTE* dst, const BYTE* src, unsigned int n)
		{ for (unsigned int i = 0 ; i < n ; i++) dst[i] = src[i] ; } ;

	void requireBlock (unsigned int len) const
	{
		// libmcrypt returned an error instead of encrypting nothing
		if (len != 0 && len < BS) {
			throw SteghideError (_("could not encrypt data.")) ;
		}
	} ;

	/// increment the counter (big endian, the whole block)
	void increaseCounter (void)
	{
		for (int i = BS - 1 ; i >= 0 ; i--) {
			if (Reg[i] == 0xff) {
				Reg[i] = 0 ;
			}
			else {
				Reg[i]++ ;
				break ;
			}
		}
	} ;

	/**
	 * process n <= BS bytes in ncfb, nofb or ctr, as the s_register_pos == 0
	 * branches of xor_stuff_en/xor_stuff_de (ncfb) and xor_stuff (nofb, ctr)
	 **/
	void feedback (BYTE* p, unsigned int n, bool encrypt)
	{
		copy (Enc.data(), Reg.data(), BS) ;
		Algo->block_encrypt (Ctx, Enc.data()) ;
		if (M == EncryptionMode::NCFB && !encrypt) {
			copy (Reg.data(), p, n) ;	// the ciphertext
		}
		xorInto (p, Enc.data(), n) ;
		if (M == EncryptionMode::NCFB && encrypt) {
			copy (Reg.data(), p, n) ;	// the ciphertext
		}
		if (M == EncryptionMode::NOFB) {
			copy (Reg.data(), Enc.data(), BS) ;
		}
		if (M == EncryptionMode::CTR && n == BS) {
			increaseCounter() ;
		}
	} ;
} ;

}

MCryptPP::MCryptPP ()
	: ModuleOpen(false), Algorithm(NULL)
{
}

MCryptPP::MCryptPP (EncryptionAlgorithm a, EncryptionMode m)
	: ModuleOpen(false), Algorithm(NULL)
{
	open (a, m) ;
}

MCryptPP::~MCryptPP ()
{
	if (ModuleOpen) {
		close() ;
	}
}

const mcrypt_algorithm* MCryptPP::findAlgorithm (EncryptionAlgorithm a)
{
	std::string name = a.getStringRep() ;
	for (const mcrypt_algorithm* algo : Algorithms) {
		if (name == algo->name) {
			return algo ;
		}
	}
	return NULL ;
}

bool MCryptPP::isBlockAlgorithmMode (EncryptionMode m)
{
	return m.getIntegerRep() != EncryptionMode::STREAM ;
}

bool MCryptPP::modeHasIV (EncryptionMode m)
{
	return m.getIntegerRep() != EncryptionMode::ECB ;
}

void MCryptPP::open (EncryptionAlgorithm a, EncryptionMode m)
{
	Algorithm = findAlgorithm (a) ;
	if (Algorithm == NULL || !AlgoSupportsMode (a, m)) {
		throw SteghideError (_("could not open libmcrypt module \"%s\",\"%s\"."), a.getStringRep().c_str(), m.getStringRep().c_str()) ;
	}
	Mode = m ;
	ModuleOpen = true ;
}

void MCryptPP::close ()
{
	Algorithm = NULL ;
	ModuleOpen = false ;
}

unsigned int MCryptPP::getBlockSize () const
{
	myassert (ModuleOpen) ;
	return Algorithm->get_block_size() ;
}

unsigned int MCryptPP::getKeySize () const
{
	myassert (ModuleOpen) ;
	return Algorithm->get_key_size() ;
}

bool MCryptPP::hasIV () const
{
	myassert (ModuleOpen) ;
	return modeHasIV (Mode) ;
}

unsigned int MCryptPP::getIVSize () const
{
	myassert (ModuleOpen) ;
	if (isBlockAlgorithmMode (Mode)) {
		return Algorithm->get_block_size() ;
	}
	else {
		return Algorithm->stream_get_iv_size() ;
	}
}

BitString MCryptPP::encrypt (BitString p, std::string pp)
{
	p.padRandom (8 * getBlockSize()) ; // blocksize is 1 for stream algorithms
	std::vector<BYTE> ciphertext = _encrypt (p.getBytes(), pp) ;
	return BitString (ciphertext) ;
}

BitString MCryptPP::decrypt (BitString c, std::string pp)
{
	myassert (c.getLength() % (8 * getBlockSize()) == 0) ;
	std::vector<BYTE> plaintext = _decrypt (c.getBytes(), pp) ;
	return BitString (plaintext) ;
}

std::vector<BYTE> MCryptPP::createKey (std::string pp)
{
	MHashKeyGen keygen (getKeySize()) ;
	return keygen.createKey (pp) ;
}

void MCryptPP::crypt (const std::vector<BYTE>& key, const BYTE* iv, std::vector<BYTE>& data, bool encrypt)
{
	myassert (ModuleOpen) ;

	// key size selection as in libmcrypt's internal_init_mcrypt
	int maxkeysize = Algorithm->get_key_size() ;
	int lenofkey = key.size() ;
	if (lenofkey > maxkeysize || lenofkey == 0) {
		throw SteghideError (_("could not initialize libmcrypt encryption. see above error messages if any.")) ;
	}
	int nsizes = 0 ;
	const int* sizes = Algorithm->get_supported_key_sizes (&nsizes) ;
	int keysize = lenofkey ;
	bool supported = false ;
	if (sizes != NULL) {
		for (int i = 0 ; i < nsizes ; i++) {
			if (lenofkey == sizes[i]) {
				supported = true ;
			}
		}
		if (!supported) {
			keysize = maxkeysize ;
			for (int i = 0 ; i < nsizes ; i++) {
				if (lenofkey <= sizes[i]) {
					keysize = sizes[i] ;
					break ;
				}
			}
		}
	}

	// the key is zero-extended to the maximum key size
	CallocBuffer keyword (maxkeysize) ;
	for (int i = 0 ; i < lenofkey ; i++) {
		keyword.get()[i] = key[i] ;
	}
	CallocBuffer ctx (Algorithm->get_size()) ;

	int err = 0 ;
	if (Algorithm->is_block_algorithm()) {
		err = Algorithm->block_set_key (ctx.get(), keyword.get(), keysize) ;
	}
	else {
		err = Algorithm->stream_set_key (ctx.get(), keyword.get(), keysize, iv, (iv != NULL) ? getIVSize() : 0) ;
	}
	if (err != 0) {
		throw SteghideError (_("could not initialize libmcrypt encryption. see above error messages if any.")) ;
	}

	// the block functions read and write whole word32s, so they need aligned memory
	CallocBuffer buf (data.size()) ;
	for (unsigned int i = 0 ; i < data.size() ; i++) {
		buf.get()[i] = data[i] ;
	}
	ModeRunner runner (Algorithm, ctx.get(), Mode.getIntegerRep(), iv) ;
	runner.run (buf.get(), data.size(), encrypt) ;
	for (unsigned int i = 0 ; i < data.size() ; i++) {
		data[i] = buf.get()[i] ;
	}
}

std::vector<BYTE> MCryptPP::_encrypt (std::vector<BYTE> p, std::string pp)
{
	myassert (p.size() % getBlockSize() == 0) ;

	std::vector<BYTE> key = createKey (pp) ;
	std::vector<BYTE> iv ;
	if (hasIV()) {
		iv = RndSrc.getBytes (getIVSize()) ;
	}

	crypt (key, hasIV() ? iv.data() : NULL, p, true) ;

	std::vector<BYTE> retval = iv ;
	retval.insert (retval.end(), p.begin(), p.end()) ;
	return retval ;
}

std::vector<BYTE> MCryptPP::_decrypt (std::vector<BYTE> c, std::string pp)
{
	std::vector<BYTE> key = createKey (pp) ;
	unsigned int ivsize = hasIV() ? getIVSize() : 0 ;
	myassert (c.size() >= ivsize) ;
	std::vector<BYTE> iv (c.begin(), c.begin() + ivsize) ;
	std::vector<BYTE> ciphertext (c.begin() + ivsize, c.end()) ;
	myassert (ciphertext.size() % getBlockSize() == 0) ;

	crypt (key, hasIV() ? iv.data() : NULL, ciphertext, false) ;

	return ciphertext ;
}

EncryptionAlgorithm MCryptPP::getAlgorithm () const
{
	myassert (ModuleOpen) ;
	return EncryptionAlgorithm (std::string (Algorithm->name)) ;
}

EncryptionMode MCryptPP::getMode () const
{
	myassert (ModuleOpen) ;
	return Mode ;
}

unsigned long MCryptPP::getEncryptedSize (EncryptionAlgorithm a, EncryptionMode m, unsigned long plnsize)
{
	unsigned long retval = 0 ;
	if (a.getIntegerRep() == EncryptionAlgorithm::NONE) {
		retval = plnsize ;
	}
	else {
		MCryptPP crypto (a, m) ;
		if (crypto.hasIV()) {
			retval += (8 * crypto.getIVSize()) ;
		}
		const unsigned long blocksize = 8 * crypto.getBlockSize() ; // is 1 for stream algorithms
		unsigned long blocks = (plnsize + blocksize - 1) / blocksize ;
		retval += (blocks * blocksize) ;
	}
	return retval ;
}

std::vector<std::string> MCryptPP::getListAlgorithms ()
{
	std::vector<std::string> retval ;
	for (const mcrypt_algorithm* algo : Algorithms) {
		retval.push_back (std::string (algo->name)) ;
	}
	return retval ;
}

std::vector<std::string> MCryptPP::getListModes ()
{
	return std::vector<std::string> (std::begin (Modes), std::end (Modes)) ;
}

bool MCryptPP::AlgoSupportsMode (EncryptionAlgorithm a, EncryptionMode m)
{
	const mcrypt_algorithm* algo = findAlgorithm (a) ;
	if (algo == NULL) {
		return false ;
	}
	return (algo->is_block_algorithm() != 0) == isBlockAlgorithmMode (m) ;
}
