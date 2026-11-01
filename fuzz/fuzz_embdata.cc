/*
 * Fuzz target for the embedded data format: feeds the input's bits to
 * EmbData the way Extractor feeds it the bits it reads from a stego file.
 * This covers the header, decryption with the passphrase "fuzz",
 * decompression, the checksum and the embedded file name. See README.md.
 */

#include <memory>
#include <vector>

#include "BitString.h"
#include "EmbData.h"
#include "SteghideError.h"
#include "fuzz.h"

extern "C" int LLVMFuzzerInitialize (int*, char***)
{
	fuzz_setup() ;
	return 0 ;
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
	const BitString bits (std::vector<BYTE> (data, data + size)) ;
	try {
		EmbData embdata (EmbData::EXTRACT, "fuzz") ;
		unsigned long pos = 0 ;
		while (!embdata.finished()) {
			const unsigned long n = embdata.getNumBitsRequested() ;
			if (n > bits.getLength() - pos) {
				return 0 ; // Extractor: the stego file is too short
			}
			BitString chunk ;
			for (unsigned long i = 0 ; i < n ; i++) {
				chunk.append (bits[pos++]) ;
			}
			embdata.addBits (chunk) ;
		}
		embdata.checksumOK() ;
		embdata.getFileName() ;
		embdata.getData() ;
	}
	catch (const AssertionFailed& e) {
		fuzz_assertion (e) ;
	}
	catch (const SteghideError&) {
	}
	return 0 ;
}
