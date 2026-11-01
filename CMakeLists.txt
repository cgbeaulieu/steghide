cmake_minimum_required(VERSION 3.20...4.4)

project(steghide VERSION 0.6.0 LANGUAGES C CXX)

include(CheckIncludeFileCXX)
include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(STEGHIDE_DEBUG "Compile code used only for debugging (--debug)" OFF)
option(STEGHIDE_NORANDOM "Disable randomness, making every run exactly reproducible" OFF)
option(STEGHIDE_NLS "Translate messages with gettext" ON)
set(STEGHIDE_SANITIZE "" CACHE STRING "Sanitizers to build with, e.g. address,undefined")
set(STEGHIDE_COMPAT_REFERENCE "" CACHE FILEPATH
	"steghide 0.5.1 binary; when set, the compat test also checks that 0.5.1 reads what this build writes")

# Dependencies. As in configure.ac, zlib and libjpeg are optional. The
# ciphers and hashes steghide used from libmcrypt and libmhash are in-tree.

find_package(ZLIB)
if(ZLIB_FOUND)
	set(USE_ZLIB 1)
else()
	message(WARNING "zlib not found: steghide will not be able to compress embedded data")
endif()

find_package(JPEG)
if(JPEG_FOUND)
	set(USE_LIBJPEG 1)
else()
	message(WARNING "libjpeg not found: steghide will not be able to read or write JPEG files")
endif()

if(STEGHIDE_NLS)
	find_package(Intl)
	if(Intl_FOUND)
		set(ENABLE_NLS 1)
	else()
		message(WARNING "libintl not found: messages will not be translated")
	endif()
endif()

# config.h

set(DEBUG ${STEGHIDE_DEBUG})
set(NORANDOM ${STEGHIDE_NORANDOM})
check_include_file_cxx(termios.h HAVE_TERMIOS_H)
if(EXISTS /dev/urandom)
	set(HAVE_DEV_URANDOM 1)
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Windows|CYGWIN")
	set(WIN32 1)
endif()
configure_file(cmake/config.h.in config.h)

# libmcrypt's ciphers, see src/mcrypt/README.md

add_library(steghide_mcrypt STATIC
	src/mcrypt/arcfour.c src/mcrypt/blowfish.c src/mcrypt/cast-128.c src/mcrypt/cast-256.c
	src/mcrypt/des.c src/mcrypt/enigma.c src/mcrypt/gost.c src/mcrypt/loki97.c src/mcrypt/rc2.c
	src/mcrypt/rijndael-128.c src/mcrypt/rijndael-192.c src/mcrypt/rijndael-256.c
	src/mcrypt/saferplus.c src/mcrypt/serpent.c src/mcrypt/tripledes.c src/mcrypt/twofish.c
	src/mcrypt/wake.c src/mcrypt/xtea.c
)
target_include_directories(steghide_mcrypt PRIVATE src/mcrypt)

# Everything but main.cc goes into an object library that the unit tests link too.

add_library(steghide_core OBJECT
	src/Arg.cc src/Arguments.cc src/AssertionFailed.cc src/AuFile.cc src/AuSampleValues.cc
	src/DFSAPHeuristic.cc src/BFSAPHeuristic.cc src/BinaryIO.cc src/BitString.cc src/BmpFile.cc
	src/BmpPaletteSampleValue.cc src/BmpRGBSampleValue.cc src/BmpSampleValue.cc
	src/WKSConstructionHeuristic.cc src/DMDConstructionHeuristic.cc src/CvrStgFile.cc
	src/Edge.cc src/EdgeIterator.cc src/EmbData.cc src/Embedder.cc
	src/EncryptionAlgorithm.cc src/EncryptionMode.cc src/Extractor.cc src/Graph.cc src/JpegFile.cc
	src/JpegSampleValue.cc src/MCryptPP.cc src/MHashKeyGen.cc src/MHashPP.cc src/Matching.cc
	src/MatchingAlgorithm.cc src/ProgressOutput.cc src/PseudoRandomSource.cc src/RGBTriple.cc
	src/RandomSource.cc src/SampleValue.cc src/SampleValueAdjacencyList.cc src/Selector.cc
	src/Session.cc src/SteghideError.cc src/Terminal.cc src/Utils.cc src/Vertex.cc src/WavChunk.cc
	src/WavChunkHeader.cc src/WavChunkUnused.cc src/WavFile.cc src/WavFormatChunk.cc
	src/WavPCMSampleValue.cc src/error.cc src/msg.cc src/SMDConstructionHeuristic.cc
)
target_include_directories(steghide_core PUBLIC ${PROJECT_BINARY_DIR} src)
target_compile_definitions(steghide_core PUBLIC HAVE_CONFIG_H)
target_link_libraries(steghide_core PUBLIC steghide_mcrypt)
if(USE_ZLIB)
	target_link_libraries(steghide_core PUBLIC ZLIB::ZLIB)
endif()
if(USE_LIBJPEG)
	target_link_libraries(steghide_core PUBLIC JPEG::JPEG)
endif()
if(ENABLE_NLS)
	target_link_libraries(steghide_core PUBLIC Intl::Intl)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	# the hardening flags Debian's add-hardening.patch adds to the autotools build
	target_compile_options(steghide_core PUBLIC
		-Wall -fstack-protector-strong -Wformat -Werror=format-security)
	# the ciphers compare unsigned test vectors with strcmp in their self-tests
	target_compile_options(steghide_mcrypt PRIVATE
		-Wall -Wno-pointer-sign -fstack-protector-strong -Wformat -Werror=format-security)
endif()

if(STEGHIDE_SANITIZE)
	foreach(target steghide_mcrypt steghide_core)
		target_compile_options(${target} PUBLIC -fsanitize=${STEGHIDE_SANITIZE} -fno-omit-frame-pointer)
		target_link_options(${target} PUBLIC -fsanitize=${STEGHIDE_SANITIZE})
	endforeach()
endif()

add_executable(steghide src/main.cc)
target_link_libraries(steghide PRIVATE steghide_core)
target_compile_definitions(steghide PRIVATE LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")
# tests/systemtests.pl runs ../src/steghide
set_target_properties(steghide PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src)

# Translations

find_package(Gettext)
if(ENABLE_NLS AND GETTEXT_MSGFMT_EXECUTABLE)
	file(STRINGS po/LINGUAS languages REGEX "^[^#]")
	string(REPLACE " " ";" languages "${languages}")
	file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/po)
	foreach(lang IN LISTS languages)
		set(mo ${PROJECT_BINARY_DIR}/po/${lang}.mo)
		add_custom_command(OUTPUT ${mo}
			COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${mo} ${PROJECT_SOURCE_DIR}/po/${lang}.po
			DEPENDS po/${lang}.po
			VERBATIM)
		list(APPEND mo_files ${mo})
		install(FILES ${mo} DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES RENAME steghide.mo)
	endforeach()
	add_custom_target(translations ALL DEPENDS ${mo_files})
endif()

# Install

install(TARGETS steghide)
install(FILES doc/steghide.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ABOUT-NLS BUGS COPYING CREDITS HISTORY LEAME README TODO DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Tests

include(CTest)
if(BUILD_TESTING)
	add_executable(unittests
		tests/AuFileTest.cc tests/AUtilsTest.cc tests/AudioSampleValueTest.cc
		tests/BFSAPHeuristicTest.cc tests/DFSAPHeuristicTest.cc
		tests/BitStringTest.cc tests/BmpOS2FileTest.cc tests/BmpPaletteSampleValueTest.cc
		tests/BmpRGBSampleValueTest.cc tests/BmpWinFileTest.cc tests/WKSConstructionHeuristicTest.cc
		tests/BmpFileTest.cc tests/CvrStgFileTest.cc
		tests/DummyFile.cc tests/DummySampleValue.cc tests/EdgeIteratorTest.cc tests/EmbDataTest.cc
		tests/GraphTest.cc tests/JpegFileTest.cc tests/JpegSampleValueTest.cc tests/MCryptPPTest.cc
		tests/MHashKeyGenTest.cc tests/MHashPPTest.cc tests/MatchingTest.cc tests/CryptoVectorTest.cc
		tests/SampleValueAdjacencyListTest.cc tests/SampleValueTest.cc tests/SelectorTest.cc
		tests/TestSuite.cc tests/UnitTest.cc tests/WavFileTest.cc tests/WavPCMSampleValueTest.cc
		tests/main.cc
	)
	target_link_libraries(unittests PRIVATE steghide_core)

	# The unit and system tests use paths relative to the tests directory
	# (./data, ./st_*.pl, ../src/steghide) and write scratch files there, so
	# they run in a mirror of it in the build tree.
	set(test_dir ${PROJECT_BINARY_DIR}/tests)
	file(MAKE_DIRECTORY ${test_dir})
	foreach(f data vectors systemtests.pl st_embparameters.pl st_fileformats.pl STSupport.pm)
		file(CREATE_LINK ${PROJECT_SOURCE_DIR}/tests/${f} ${test_dir}/${f} SYMBOLIC COPY_ON_ERROR)
	endforeach()

	add_test(NAME unittests COMMAND unittests WORKING_DIRECTORY ${test_dir})

	find_package(Perl)
	if(PERL_FOUND)
		add_test(NAME systemtests COMMAND ${PERL_EXECUTABLE} systemtests.pl WORKING_DIRECTORY ${test_dir})
	endif()

	find_program(BASH_EXECUTABLE bash)
	if(BASH_EXECUTABLE)
		add_test(NAME compat
			COMMAND ${BASH_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/compat/check.sh
				$<TARGET_FILE:steghide> ${STEGHIDE_COMPAT_REFERENCE})
	endif()

	if(STEGHIDE_SANITIZE)
		# steghide itself must not leak, so the tests that run it check for
		# leaks; the unit tests do not free their fixtures
		set(ubsan_options "UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1")
		set_tests_properties(unittests PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0;${ubsan_options}")
		foreach(t systemtests compat)
			if(TEST ${t})
				set_tests_properties(${t} PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1;${ubsan_options}")
			endif()
		endforeach()
	endif()
endif()
