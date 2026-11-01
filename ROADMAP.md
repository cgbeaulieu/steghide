# Modernization roadmap

Goal: a maintained steghide that builds on current toolchains, drops its
abandoned dependencies, and fixes the weaknesses in its file format, while
still reading files written by steghide 0.5.1.

## Sources

| Remote | What it is |
| --- | --- |
| `steghigh` | [StegHigh/steghide](https://github.com/StegHigh/steghide), the GitHub mirror that accepts pull requests. This fork starts from its `master` (`baaea53`), which already builds with current autotools and GCC. |
| `museoa` | [museoa/steghide](https://codeberg.org/museoa/steghide) on Codeberg: the full CVS history (2001–2003) plus Fedora's patches, tagged 0.5.1.1. Reference only. |

## Phase 1: baseline

- [x] Debian's patch series ([salsa](https://salsa.debian.org/pkg-security-team/steghide/-/tree/debian/master/debian/patches)), one commit per patch with the original author, including the CVE-2026-31024 overflow fix. `consolidated-gcc-patches` is applied partially; the commit says which hunks were skipped and why.
- [x] The two Fedora fixes StegHigh was missing.
- [x] Compatibility fixtures written by steghide 0.5.1, with a checker for both directions ([tests/compat](tests/compat/README.md)).

State at the end of phase 1, built with GCC 16.2:

- builds, with 164 warnings (85 `-Wdeprecated-declarations`, 27 `-Wcpp`, 17 `-Wcatch-value`, 11 `-Wwrite-strings`, and a few others)
- `tests/systemtests.pl` passes
- `tests/compat/check.sh` passes 100/100 against 0.5.1 in both directions

### Bugs found so far

- **CVE-2021-27211**: `Selector.cc` folds MD5(passphrase) into a 32-bit seed for the embedding permutation, and `MCryptPP.cc` derives the key with MD5. This makes passphrase cracking fast (see stegseek) and lets an attacker detect steghide data by brute-forcing the seed. It is part of the file format, so only phase 3 can fix it.
- `configure.ac` and `CMakeLists.txt` say 0.6.0 (bumped in `d4afe38`), but the code is 0.5.1 plus fixes.
- `tests/MCryptPPTest.cc` has two empty test stubs that return `bool` without a return statement. They are never called.
- `EmbData::stripDir` and `Session::stripDir` store `find_last_of`'s result in an `unsigned int`, which truncates `npos`. Reported by [alexnorell/steghide](https://github.com/alexnorell/steghide) (two bugs that cancel out); not yet checked here.
- Fixed in phase 2: a new-delete-type-mismatch in `AuFile::~AuFile` and a heap-use-after-free in `EdgeIterator::operator++`, both found with ASan, and undefined behaviour in seven of libmcrypt's ciphers, found with UBSan (see `src/mcrypt/README.md`).

## Phase 2: modernize without changing the format

Every step must keep `tests/compat/check.sh` at 100% in both directions.

- [x] Fix the two sanitizer findings: `CvrStgObject` gets a virtual destructor, and `DFSAPHeuristic::run()` repositions its edge iterators after the construction heuristic has rebuilt the graph's sample-occurence lists
- [x] CMake build, C++20, with `STEGHIDE_SANITIZE` and the unit, system and compat tests in CTest
- [x] Builds with Clang
- [x] Replace libmcrypt and libmhash. The 18 ciphers libmcrypt 2.5.8 builds are its own code in `src/mcrypt/` (with its undefined behaviour fixed); the modes, the key setup, MD5, CRC32 and the key generation are reimplemented. Checked against vectors from the real libraries for every algorithm, mode and key length (`tests/vectors/`), and against the 0.5.1 fixtures in both directions.
- [ ] Remove autotools and the generated files committed with it (kept for now; `make check` still passes)
- [ ] CI: GCC and Clang, ASan/UBSan, all three CTest suites
- [ ] Fuzz the cover-file parsers (BMP, JPEG, WAV, AU); they read untrusted input
- [ ] Replace `hash_map`/`hash_set` (18 uses), `binary_function`/`unary_function` (5, removed in C++17), `sprintf`/`strcpy` (20), and raw `new`/`delete` (~290 `new`)
- [ ] Replace `srand(time(NULL))` in `RandomSource.cc` with the OS CSPRNG
- [ ] Turn leak detection back on under ASan (off in CTest for now; steghide frees little of what it allocates)

State so far, with GCC 16.2, Clang 23.1 (-O2 and -O3), and both compilers with ASan/UBSan: all three CTest suites pass (the sanitizer builds 3 runs in a row), and so does the autotools `make check`. The unit tests use their own harness in `tests/`, not cppunit. Translations build and install, but loading them has not been checked on a system with a matching locale.

## Phase 3: v2 file format

Opt-in at first, then the default for embedding. v1 (0.5.1) files stay readable.

- [ ] Argon2id for key derivation
- [ ] Authenticated encryption (for example XChaCha20-Poly1305) instead of mcrypt CBC + CRC32
- [ ] A full-width seed for the embedding permutation
- [ ] Open problem: where to keep the salt without making stego files detectable

## Phase 4: later

- [ ] PNG support (attempted in StegHigh #9 and #10, never merged)
- [ ] A library API, so the CLI is one client of it

## Open decisions

- **Old ciphers.** Decided: all 18 that libmcrypt 2.5.8 builds stay, in-tree. steghide's algorithm table also names safer-sk64, safer-sk128, threeway and panama, which libmcrypt 2.5.8 does not build, so 0.5.1 could not use them with a standard libmcrypt and neither can this fork. alexnorell/steghide supports them; worth a look if such files turn up.
- **Writing v1.** Once the v2 format exists, whether v1 (and these ciphers) should only be read.
- **Name.** Whether the fork keeps the steghide name.
- **Upstream.** Whether to offer this work to StegHigh, which is looking for maintainers.

## Related forks

- [alexnorell/steghide](https://github.com/alexnorell/steghide) (August 2026, from the same StegHigh commit): replaces libmcrypt and libmhash with libgcrypt plus C++ rewrites of 14 ciphers, all 22 algorithms; C++23 with no warnings; std::unordered containers; memory leak fixes; autotools and translations removed. Its list of libmcrypt quirks was used as a checklist for `src/mcrypt/`.
- [RickdeJager/stegseek](https://github.com/RickdeJager/stegseek): steghide's code turned into a passphrase cracker, still on libmcrypt and libmhash.
