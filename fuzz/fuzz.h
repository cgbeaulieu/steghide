/*
 * Setup shared by the fuzz targets. See README.md.
 */

#ifndef SH_FUZZ_H
#define SH_FUZZ_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#include "AssertionFailed.h"
#include "common.h"

/**
 * set the arguments steghide would otherwise take from the command line:
 * no messages, and no "overwrite ?" questions
 **/
inline void fuzz_setup (void)
{
	Args.Verbosity.setValue (QUIET) ;
	Args.Force.setValue (true) ;
}

/**
 * steghide reads cover files by name, so the targets hand it each input
 * as a file in memory. Returns the name to open.
 **/
inline std::string fuzz_memfile (int* fd)
{
	*fd = memfd_create ("steghide-fuzz", 0) ;
	if (*fd < 0) {
		perror ("memfd_create") ;
		abort() ;
	}
	return "/proc/self/fd/" + std::to_string (*fd) ;
}

inline void fuzz_setinput (int fd, const uint8_t* data, size_t size)
{
	if (ftruncate (fd, 0) != 0 || (size > 0 && pwrite (fd, data, size, 0) != (ssize_t) size)) {
		perror ("fuzz_setinput") ;
		abort() ;
	}
}

/**
 * steghide reports bad input with a SteghideError. An AssertionFailed means
 * the input reached a state the code assumes is impossible: a bug.
 **/
[[noreturn]] inline void fuzz_assertion (const AssertionFailed& e)
{
	e.printMessage() ;
	abort() ;
}

#endif // ndef SH_FUZZ_H
