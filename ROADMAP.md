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
- `CMakeLists.txt` says 0.6.0 (bumped in `configure.ac` in `d4afe38`), but the code is 0.5.1 plus fixes.
- Fixed in phase 2 (see `git log` for each):
  - a new-delete-type-mismatch in `AuFile::~AuFile` and a heap-use-after-free in `EdgeIterator::operator++`, found with ASan
  - undefined behaviour in seven of libmcrypt's ciphers, found with UBSan (see `src/mcrypt/README.md`)
  - a stack overflow, also in 0.5.1: `BmpFile` kept one `std::vector` per sample value in a variable-length array on the stack, so a 100 KB secret in a 1000x1000 24-bit BMP of random colours segfaulted
  - memory leaks on every path, found with LeakSanitizer, and with them a dangling libjpeg error manager in `JpegFile`
  - `stripDir`'s truncated `npos` (reported by [alexnorell/steghide](https://github.com/alexnorell/steghide); two bugs that cancelled out)
  - an `sprintf` overflow in `Utils::formatHRSize` and a throwing destructor in `RandomSource`
  - **extract wrote outside the current directory**: it saves the extracted file under the name stored in the stego file, and did not strip directories from it, so a stego file holding `../name` or `/path/name` wrote there. Embedding has always stripped them, so no file steghide wrote has one; extract now strips them too, with a warning (`tests/extract_name`)
  - found by fuzzing (see `fuzz/README.md` for an input for each):
    - a BMP whose palette is shorter than its pixels' colour indices was read past the end of the palette
    - the sizes in BMP, AU and WAV headers were allocated before the data was read, so a file of a few hundred bytes could make steghide allocate gigabytes
    - the same for JPEG, where libjpeg allocates the whole image before reading the data and fills in what is missing with zeros. That size can be real, so steghide now refuses JPEGs of more than 2^28 DCT coefficients (512 MB; about 178 megapixels with 4:2:0 subsampling)
    - any error in a JPEG made libjpeg call `exit()`; it now comes back as a steghide error, and libjpeg's memory is freed
    - embedded data was read past its end, and a length near 2^32 wrapped around once encryption padding was added; both failed assertions
    - more assertions reachable from a file: BMP planes, compressed data that is not whole bytes, an AU data size that is not whole samples, a WAV with more than 32 bits per sample
  - embedding in an 8-bit WAV turned some samples of 255 into 0, a full-scale click: the largest 8-bit value was taken to be 256, which a byte stores as 0. Found while reading the WAV code for the fuzzer; 32-bit WAV samples also relied on signed overflow

## Phase 2: modernize without changing the format

Every step must keep `tests/compat/check.sh` at 100% in both directions.

- [x] Fix the two sanitizer findings: `CvrStgObject` gets a virtual destructor, and `DFSAPHeuristic::run()` repositions its edge iterators after the construction heuristic has rebuilt the graph's sample-occurence lists
- [x] CMake build, C++20, with `STEGHIDE_SANITIZE` and the unit, system and compat tests in CTest
- [x] Builds with Clang
- [x] Replace libmcrypt and libmhash. The 18 ciphers libmcrypt 2.5.8 builds are its own code in `src/mcrypt/` (with its undefined behaviour fixed); the modes, the key setup, MD5, CRC32 and the key generation are reimplemented. Checked against vectors from the real libraries for every algorithm, mode and key length (`tests/vectors/`), and against the 0.5.1 fixtures in both directions.
- [x] Remove autotools, the files it generated, and what gettextize copied in (`intl/`, `m4/`, most of `po/`). CMake took over the two things it still did: `update-po` regenerates `po/steghide.pot` and merges it into the translations, and `doxygen` builds the source documentation. The RPM spec and `make dist` are gone; distributions have their own spec, and `git archive` makes a tarball.
- [x] CI (`.github/workflows/ci.yml`), on every push and pull request, on Ubuntu 24.04: GCC 13 and Clang 18 (Clang also at -O3), both with ASan/UBSan, and GCC with `STEGHIDE_DEBUG`. Each runs all CTest tests, with Ubuntu's steghide 0.5.1 as the compat reference, and fails on a compiler warning outside `src/mcrypt/`. A Clang build with `STEGHIDE_FUZZ` fuzzes both targets for two minutes each, starting from the corpus earlier runs kept. GCC 13 warned about two things in the unit tests that GCC 16 does not: an ignored `system()` result (Ubuntu builds with `_FORTIFY_SOURCE`), now `std::filesystem`, and a false `-Warray-bounds` in `EmbDataTest`.
- [x] Fuzz the cover-file parsers (BMP, JPEG, WAV, AU) and the embedded data format with libFuzzer (`fuzz/`, `STEGHIDE_FUZZ`). CTest replays an input for each bug found in every build.
- [x] Replace `hash_map`/`hash_set` with `std::unordered_*`; drop `binary_function`/`unary_function`; replace `sprintf` and variable-length arrays. Sample values are now labelled in first-seen order instead of hash-table order.
- [x] No compiler warnings with GCC or Clang (`-Wall`), outside `src/mcrypt/`
- [x] No leaks, in steghide or in the unit tests; CTest checks every test for leaks under `STEGHIDE_SANITIZE`
- [x] Ownership: `src/` has no `new` or `delete` left. Owners hold `std::unique_ptr`s or containers (cover files their `BinaryIO` and contents, `Embedder` and `Extractor` their file, `Embedder` its graph and matching, `Graph` its vertices, sample values and adjacency lists, `Matching` its edges by value), factories return `std::unique_ptr`, and `Globs` only points to the current file and graph (they clear it when destroyed). The unit tests still use raw pointers for their fixtures, freed in `cleanup()`.
- [x] Replace `srand(time(NULL))` in `RandomSource.cc` with the OS CSPRNG: `getentropy()`, 256 bytes at a time. The IVs come from it, so if it fails steghide stops with an error; 0.5.1 fell back to `rand()` seeded with the time when it could not open `/dev/urandom`, and always on Windows. A native Windows build would need `BCryptGenRandom` instead.

State so far, with GCC 16.2, Clang 23.1 (-O2 and -O3), both compilers with ASan/UBSan and leak checks, and `STEGHIDE_DEBUG`: all six CTest tests pass (unit, system, compat in both directions against 0.5.1, `extract_name`, and the replays of both fuzz targets), with no warnings. CI checks the same with Ubuntu 24.04's GCC 13 and Clang 18. With all fixes in, `fuzz_cover` ran 3.2 million inputs and `fuzz_embdata` 25 million, under ASan and UBSan, without a finding. The unit tests use their own harness in `tests/`, not cppunit. Translations build and install, but loading them has not been checked on a system with a matching locale.

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
