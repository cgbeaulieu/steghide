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

#ifndef SH_MCRYPTPP_H
#define SH_MCRYPTPP_H

#include <string>
#include <vector>

#include "common.h"
#include "EncryptionAlgorithm.h"
#include "EncryptionMode.h"

class BitString ;
struct mcrypt_algorithm ;

/**
 * \class MCryptPP
 * \brief encryption the way steghide 0.5.1 did it with libmcrypt
 *
 * The ciphers are libmcrypt's own sources (see src/mcrypt/README.md). The
 * modes and the key setup are reimplemented here from libmcrypt 2.5.8, and
 * keys come from MHashKeyGen. steghide always uses an algorithm's maximum
 * key size, so for example rijndael-128 is AES with a 256 bit key.
 **/
class MCryptPP {
	public:
	MCryptPP (void) ;
	MCryptPP (EncryptionAlgorithm a, EncryptionMode m) ;

	~MCryptPP (void) ;

	/**
	 * open the encryption module and set the algorithm and mode
	 * \param a the algorithm to use
	 * \param m the mode to use
	 **/
	void open (EncryptionAlgorithm a, EncryptionMode m) ;

	/**
	 * close the encryption module
	 **/
	void close (void) ;

	/**
	 * encrypt p using pp as passphrase
	 * \param p the plaintext to be encrypted
	 * \param pp the passphrase
	 * \return the encrypted data (with the IV in front if the mode has one)
	 **/
	BitString encrypt (BitString p, std::string pp) ;

	/**
	 * decrypt c using pp as passphrase
	 * \param c the ciphertext to be decrypted (with the IV in front if the mode has one)
	 * \param pp the passphrase
	 * \return the decrypted data
	 **/
	BitString decrypt (BitString c, std::string pp) ;

	EncryptionAlgorithm getAlgorithm (void) const ;
	EncryptionMode getMode (void) const ;

	/**
	 * en- or decrypt data in place with the given key and IV, as libmcrypt's
	 * mcrypt_generic_init followed by one mcrypt_generic or mdecrypt_generic call
	 * \param key the key; shorter keys are zero-extended as libmcrypt did
	 * \param iv getIVSize() bytes, or NULL to use zeros
	 **/
	void crypt (const std::vector<BYTE>& key, const BYTE* iv, std::vector<BYTE>& data, bool encrypt) ;

	unsigned int getBlockSize (void) const ;
	unsigned int getKeySize (void) const ;
	bool hasIV (void) const ;
	unsigned int getIVSize (void) const ;

	/**
	 * get the size of an encrypted message
	 * \param a the algorithm to be used for encryption
	 * \param m the mode to be used for encryption
	 * \param plnsize the size of the plaintext (in bits)
	 * \return the size the ciphertext would have (in bits and including the IV)
	 **/
	static unsigned long getEncryptedSize (EncryptionAlgorithm a, EncryptionMode m, unsigned long plnsize) ;

	static std::vector<std::string> getListModes (void) ;
	static std::vector<std::string> getListAlgorithms (void) ;

	static bool AlgoSupportsMode (EncryptionAlgorithm a, EncryptionMode m) ;

	protected:
	std::vector<BYTE> createKey (std::string pp) ;

	std::vector<BYTE> _encrypt (std::vector<BYTE> p, std::string pp) ;
	std::vector<BYTE> _decrypt (std::vector<BYTE> c, std::string pp) ;

	private:
	/// true iff Algorithm and Mode are valid
	bool ModuleOpen ;
	const mcrypt_algorithm* Algorithm ;
	EncryptionMode Mode ;

	static const mcrypt_algorithm* findAlgorithm (EncryptionAlgorithm a) ;
	static bool isBlockAlgorithmMode (EncryptionMode m) ;
	static bool modeHasIV (EncryptionMode m) ;
} ;

#endif // ndef SH_MCRYPTPP_H
