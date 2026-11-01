/*
 * A main() for the fuzz targets in builds without libFuzzer: runs each file
 * given on the command line, or each file in each directory given, once,
 * and ignores options such as libFuzzer's -runs=0. CTest uses it to replay the saved inputs in fuzz/regressions with GCC and
 * the sanitizer builds. See README.md.
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

extern "C" int LLVMFuzzerInitialize (int* argc, char*** argv) ;
extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size) ;

static void run (const std::filesystem::path& p)
{
	std::ifstream in (p, std::ios::binary) ;
	std::vector<uint8_t> data ((std::istreambuf_iterator<char> (in)), std::istreambuf_iterator<char>()) ;
	LLVMFuzzerTestOneInput (data.data(), data.size()) ;
}

int main (int argc, char* argv[])
{
	LLVMFuzzerInitialize (&argc, &argv) ;
	unsigned long n = 0 ;
	for (int i = 1 ; i < argc ; i++) {
		if (argv[i][0] == '-') {
			continue ;
		}
		std::filesystem::path p (argv[i]) ;
		if (std::filesystem::is_directory (p)) {
			std::vector<std::filesystem::path> files ;
			for (const auto& e : std::filesystem::directory_iterator (p)) {
				if (e.is_regular_file()) {
					files.push_back (e.path()) ;
				}
			}
			std::sort (files.begin(), files.end()) ;
			for (const auto& f : files) {
				run (f) ;
				n++ ;
			}
		}
		else {
			run (p) ;
			n++ ;
		}
	}
	printf ("ran %lu inputs\n", n) ;
	return 0 ;
}
