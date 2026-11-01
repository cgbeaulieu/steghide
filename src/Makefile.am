if USE_INTLDIR
AM_CPPFLAGS = -g -fstack-protector-strong -Wformat -Werror=format-security -I../intl -DLOCALEDIR=\"$(localedir)\"
else
AM_CPPFLAGS = -g -fstack-protector-strong -Wformat -Werror=format-security -DLOCALEDIR=\"$(localedir)\"
endif
AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = steghide
noinst_LIBRARIES = libmcryptciphers.a
noinst_HEADERS = \
AUtils.h Arg.h Arguments.h AssertionFailed.h AuData.h AuFile.h \
AuSampleValues.h AudioData.h AudioSampleValue.h DFSAPHeuristic.h BFSAPHeuristic.h \
BinaryIO.h BitString.h BmpFile.h BmpPaletteSampleValue.h BmpRGBSampleValue.h \
BmpSampleValue.h ColorPalette.h WKSConstructionHeuristic.h CvrStgFile.h \
CvrStgObject.h Edge.h EdgeIterator.h DMDConstructionHeuristic.h \
EmbData.h Embedder.h EncryptionAlgorithm.h EncryptionMode.h Extractor.h \
Globals.h Graph.h JpegFile.h JpegSampleValue.h MCryptPP.h MHashKeyGen.h \
MHashPP.h Matching.h MatchingAlgorithm.h ProgressOutput.h PseudoRandomSource.h \
RGBTriple.h RandomSource.h SampleOccurence.h SampleValue.h \
SampleValueAdjacencyList.h Selector.h Session.h SteghideError.h Terminal.h \
Utils.h Vertex.h WavChunk.h WavChunkHeader.h WavChunkUnused.h WavFile.h \
WavFormatChunk.h WavPCMSampleValue.h common.h error.h msg.h \
SMDConstructionHeuristic.h gettext.h
steghide_SOURCES = \
Arg.cc Arguments.cc AssertionFailed.cc AuFile.cc AuSampleValues.cc \
DFSAPHeuristic.cc BFSAPHeuristic.cc BinaryIO.cc BitString.cc BmpFile.cc \
BmpPaletteSampleValue.cc BmpRGBSampleValue.cc BmpSampleValue.cc \
WKSConstructionHeuristic.cc DMDConstructionHeuristic.cc CvrStgFile.cc \
Edge.cc EdgeIterator.cc EmbData.cc Embedder.cc \
EncryptionAlgorithm.cc EncryptionMode.cc Extractor.cc Graph.cc JpegFile.cc \
JpegSampleValue.cc MCryptPP.cc MHashKeyGen.cc MHashPP.cc Matching.cc \
MatchingAlgorithm.cc ProgressOutput.cc PseudoRandomSource.cc RGBTriple.cc \
RandomSource.cc SampleValue.cc SampleValueAdjacencyList.cc Selector.cc \
Session.cc SteghideError.cc Terminal.cc Utils.cc Vertex.cc WavChunk.cc \
WavChunkHeader.cc WavChunkUnused.cc WavFile.cc WavFormatChunk.cc \
WavPCMSampleValue.cc error.cc main.cc msg.cc SMDConstructionHeuristic.cc
AM_LIBTOOLFLAGS = --tag=CXX
steghide_LDADD = libmcryptciphers.a

# libmcrypt's ciphers, see mcrypt/README.md
libmcryptciphers_a_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/mcrypt
libmcryptciphers_a_SOURCES = \
mcrypt/arcfour.c mcrypt/blowfish.c mcrypt/cast-128.c mcrypt/cast-256.c mcrypt/des.c \
mcrypt/enigma.c mcrypt/gost.c mcrypt/loki97.c mcrypt/rc2.c mcrypt/rijndael-128.c \
mcrypt/rijndael-192.c mcrypt/rijndael-256.c mcrypt/saferplus.c mcrypt/serpent.c \
mcrypt/tripledes.c mcrypt/twofish.c mcrypt/wake.c mcrypt/xtea.c \
mcrypt/arcfour.h mcrypt/blowfish.h mcrypt/cast-128.h mcrypt/cast-128_sboxes.h \
mcrypt/cast-256.h mcrypt/des.h mcrypt/enigma.h mcrypt/rc2.h mcrypt/rijndael.h \
mcrypt/saferplus.h mcrypt/serpent.h mcrypt/tripledes.h mcrypt/twofish.h mcrypt/wake.h \
mcrypt/xtea.h mcrypt/libdefs.h mcrypt/mcrypt_modules.h mcrypt/mcrypt_algorithm.h
EXTRA_DIST = mcrypt/README.md mcrypt/COPYING.LIB

LIBS = @LIBINTL@ @LIBS@
localedir = $(datadir)/locale
LIBTOOL = $(SHELL) libtool
MAINTAINERCLEANFILES = Makefile.in
