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
- the cppunit unit tests have not been run (cppunit is not installed)

### Bugs found so far

- **new-delete-type-mismatch** in `AuFile::~AuFile` (`src/AuFile.cc:46`): `AudioData` has no virtual destructor, so deleting an `AudioDataImpl` through it is undefined behaviour. ASan reports it when embedding into 8-bit AU files (mu-law and PCM8). Matches the `-Wdelete-non-virtual-dtor` warning.
- **heap-use-after-free** in `EdgeIterator::operator++` (`src/EdgeIterator.cc:72`), reached from `DFSAPHeuristic::getNextEdge` while computing the matching: the iterator walks a `SampleOccurence` list node that has already been erased. Whether it triggers depends on the cover and options; embedding `rnd100` into `pcm16_std.au` with `-e none -Z -K` reproduces it.
- **CVE-2021-27211**: `Selector.cc` folds MD5(passphrase) into a 32-bit seed for the embedding permutation, and `MCryptPP.cc` derives the key with MD5. This makes passphrase cracking fast (see stegseek) and lets an attacker detect steghide data by brute-forcing the seed. It is part of the file format, so only phase 3 can fix it.
- `configure.ac` says 0.6.0 (bumped in `d4afe38`), but the code is 0.5.1 plus fixes.

## Phase 2: modernize without changing the format

Every step must keep `tests/compat/check.sh` at 100% in both directions.

- [ ] CMake build and C++20; remove autotools and the generated files committed with it
- [ ] CI: GCC and Clang, ASan/UBSan, system tests, compat check
- [ ] Fix the two sanitizer findings above
- [ ] Fuzz the cover-file parsers (BMP, JPEG, WAV, AU); they read untrusted input
- [ ] Replace libmcrypt and mhash, both unmaintained for over 15 years. This has to reproduce mcrypt's key generation (`KEYGEN_MCRYPT` with MD5) exactly, and decide which of the 18 mcrypt ciphers stay readable (see below).
- [ ] Replace `hash_map`/`hash_set` (18 uses), `binary_function`/`unary_function` (5, removed in C++17), `sprintf`/`strcpy` (20), and raw `new`/`delete` (~290 `new`)
- [ ] Replace `srand(time(NULL))` in `RandomSource.cc` with the OS CSPRNG
- [ ] Get the unit tests running in CI (keep cppunit or move to another framework)

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

- **Old ciphers.** `-e` exposes every mcrypt cipher. AES (rijndael-128, the default) is easy to replace; the long tail (gost, loki97, saferplus, wake, enigma, ...) is not. Proposal: keep reading only what a maintained library supports, and write only the v2 format's cipher.
- **Name.** Whether the fork keeps the steghide name.
- **Upstream.** Whether to offer this work to StegHigh, which is looking for maintainers.
