# libmcrypt's ciphers

steghide 0.5.1 encrypted embedded data with libmcrypt, which has been
unmaintained since 2007. The algorithm and mode are stored in every stego
file, so reading old files needs exactly libmcrypt's ciphers, including its
bugs. This directory holds the 18 ciphers libmcrypt 2.5.8 builds by default,
taken from its source; `MCryptPP.cc` replaces the rest of libmcrypt (its
modes and key setup) and `MHashPP.cc`/`MHashKeyGen.cc` replace libmhash.

The ciphers are only here to read and write 0.5.1-format files.

## Source

`modules/algorithms/` of libmcrypt 2.5.8, from the `upstream` branch of
Debian's packaging (https://salsa.debian.org/debian/libmcrypt). The original
implementations are public domain or freely licensed by their authors (see
each file); libmcrypt's changes to them are under the LGPL 2.1 or later
(`COPYING.LIB`), which allows distributing them under the GPL, as steghide
does.

`libdefs.h` and `mcrypt_modules.h` replace libmcrypt's headers of the same
names. `mcrypt_algorithm.h` is new.

## Changes

Each change except the include paths is marked with a `steghide:` comment.
None changes the ciphertext GCC on x86 produced with libmcrypt.

- Each `.c` file ends with a descriptor (`mcrypt_algorithm.h`) instead of
  being loaded through libmcrypt's module table.
- `mcrypt_modules.h`: the rotations shift by `32 - n`, which is undefined for
  `n == 0`; cast-256 and cast-128 (its own `ROL`) rotate by key-dependent
  amounts that are 0 for some keys. They are now well-defined functions with
  the rotate-instruction result, which is what GCC produced. Clang does not
  necessarily do the same with the original macros.
- `des.c`, `tripledes.c`: an `int` shift that overflowed is done as `word32`;
  K&R declarations became prototypes; `tripledes.c` passed `&key->iperm`
  (the right address, the wrong type) to `perminit`.
- `gost.c`, `loki97.c`: bytes shifted left by 24 as `int`, which overflowed,
  are shifted as `word32`.
- `enigma.c`: the seed arithmetic overflowed `int`; it is done in `word32` and
  converted back. `shuffle` is now `static` (it was exported, and is never
  called because `secureflg` is always 0).
- `enigma.h`: its tables are `signed char` instead of `char`, the
  signedness `char` has on x86, so the cipher computes the same everywhere.
- `twofish.c`: two temporaries that were read uninitialized (and then
  overwritten) are initialized.
- `wake.c`: the self-test freed its key twice when it failed.
- `arcfour.c`: include paths.

Bugs libmcrypt has and steghide keeps, because files were written with them:
gost's S-box `gost_k1` has 2 where the published value is 12, rc2 leaves out
RFC 2268's effective-key-bits phase ("Phase 2 ... stripped"), and steghide's
`blowfish` is libmcrypt's byte order, not the standard one (libmcrypt's
`blowfish-compat`, which steghide never supported).

## Testing

`tests/CryptoVectorTest.cc` checks every algorithm, mode and key length
against `tests/vectors/crypto_vectors.txt`, which
`tests/vectors/gen_crypto_vectors.c` captured from the real libmcrypt 2.5.8
and libmhash. It also runs each cipher's own self-test. `tests/compat` checks
whole stego files against steghide 0.5.1 in both directions.
