/*
 * Fuzz target for the cover file parsers: reads the input as a BMP, JPEG,
 * WAV or AU file, reads every sample, and writes the file out again, as
 * embedding does. See README.md.
 */

#include <memory>
#include <string>

#include "CvrStgFile.h"
#include "SteghideError.h"
#include "fuzz.h"

static int InputFd = -1 ;
static std::string InputName ;

extern "C" int LLVMFuzzerInitialize (int*, char***)
{
	fuzz_setup() ;
	InputName = fuzz_memfile (&InputFd) ;
	return 0 ;
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
	fuzz_setinput (InputFd, data, size) ;
	try {
		std::unique_ptr<CvrStgFile> file = CvrStgFile::readFile (InputName) ;
		file->getCapacity() ;
		const unsigned long n = file->getNumSamples() ;
		for (unsigned long pos = 0 ; pos < n ; pos++) {
			file->getEmbeddedValue (pos) ;
		}
		file->transform ("/dev/null") ;
		file->write() ;
	}
	catch (const AssertionFailed& e) {
		fuzz_assertion (e) ;
	}
	catch (const SteghideError&) {
	}
	return 0 ;
}
