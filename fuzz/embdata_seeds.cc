/*
 * Writes seed inputs for fuzz_embdata: the bits steghide embeds for a few
 * combinations of encryption, compression, checksum and file name, with
 * the passphrase fuzz_embdata decrypts with. See README.md.
 *
 * Usage: embdata_seeds DIRECTORY
 */

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "BitString.h"
#include "EmbData.h"
#include "EncryptionAlgorithm.h"
#include "EncryptionMode.h"
#include "common.h"

int main (int argc, char* argv[])
{
	if (argc != 2) {
		fprintf (stderr, "usage: %s DIRECTORY\n", argv[0]) ;
		return 2 ;
	}
	const std::string dir = argv[1] ;

	const std::string text = "steghide hides data in image and audio files.\n" ;
	std::vector<BYTE> payload (text.begin(), text.end()) ;
	for (unsigned int i = 0 ; i < 4 ; i++) {
		payload.insert (payload.end(), text.begin(), text.end()) ;
	}

	struct Case { const char* name ; const char* algo ; const char* mode ; int compression ; bool checksum ; const char* fn ; } ;
	const Case cases[] = {
		{ "plain",			"none",			"cbc", 0, true,  "secret.txt" },
		{ "plain-nocrc",	"none",			"cbc", 0, false, "secret.txt" },
		{ "plain-noname",	"none",			"cbc", 0, true,  "" },
		{ "compressed",		"none",			"cbc", 9, true,  "secret.txt" },
		{ "aes-cbc",		"rijndael-128",	"cbc", 9, true,  "secret.txt" },
		{ "arcfour",		"arcfour",		"stream", 0, true, "secret.txt" },
		{ "des-ofb",		"des",			"ofb", 0, false, "a" },
	} ;

	for (const Case& c : cases) {
		EmbData embdata (EmbData::EMBED, "fuzz", c.fn) ;
		embdata.setEncAlgo (EncryptionAlgorithm (c.algo)) ;
		embdata.setEncMode (EncryptionMode (c.mode)) ;
		embdata.setCompression (c.compression) ;
		embdata.setChecksum (c.checksum) ;
		embdata.setData (payload) ;
		BitString bits = embdata.getBitString() ;
		if (bits.getLength() % 8 != 0) {
			bits.append ((BYTE) 0, 8 - bits.getLength() % 8) ;
		}
		const std::vector<BYTE>& bytes = bits.getBytes() ;
		std::ofstream out (dir + "/" + c.name, std::ios::binary) ;
		out.write ((const char*) bytes.data(), bytes.size()) ;
	}
	return 0 ;
}
