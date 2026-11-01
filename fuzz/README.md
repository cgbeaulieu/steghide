# Fuzzing

steghide reads files from other people: the cover file when embedding, and
the stego file, with the data hidden in it, when extracting. These targets
feed it random variations of such files with [libFuzzer], looking for
crashes, memory errors, assertion failures and runaway memory use.

| Target | What it reads |
| --- | --- |
| `fuzz_cover` | the input as a cover file (BMP, JPEG, WAV or AU): every sample, then writes it out again, as embedding does |
| `fuzz_embdata` | the input's bits as embedded data, the way extracting reads them from a stego file: the header, decryption with the passphrase `fuzz`, decompression, the checksum and the file name |

An input that makes steghide report an error is fine. An `AssertionFailed`
counts as a crash: it means the input reached a state the code assumes is
impossible.

## Building

libFuzzer comes with Clang:

```bash
cmake -B build-fuzz -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
	-DSTEGHIDE_FUZZ=ON -DSTEGHIDE_SANITIZE=address,undefined
cmake --build build-fuzz
```

Without `STEGHIDE_FUZZ`, the targets are built with `standalone.cc`, which
runs each input once. CTest uses that in every build to replay the inputs in
`regressions/`, the seeds, and the files in `tests/data` and
`tests/compat/fixtures`.

## Running

```bash
mkdir -p corpus/cover corpus/embdata crashes
build-fuzz/fuzz_cover -jobs=8 -workers=8 -max_total_time=600 -malloc_limit_mb=1024 \
	-artifact_prefix=crashes/ corpus/cover tests/data tests/compat/fixtures
build-fuzz/fuzz_embdata -jobs=8 -workers=8 -max_total_time=600 -malloc_limit_mb=1024 \
	-artifact_prefix=crashes/ corpus/embdata fuzz/seeds/embdata
```

The first directory collects the inputs that reach new code; the others are
read only. With Clang 23.1, libFuzzer's `-fork` mode crashed in its own
code after a few seconds, so use `-jobs` and `-workers`.

`seeds/embdata` holds embedded data written by steghide itself, from
`embdata_seeds`. To write them again: `build-fuzz/embdata_seeds fuzz/seeds/embdata`.

## When it finds something

Run the input on its own to see the error, for example
`build-fuzz/fuzz_cover crashes/crash-…`. Once it is fixed, copy the input to
`regressions/cover/` or `regressions/embdata/` under a name that says what
it found, and add it to the table below.

## Regressions

What each input in `regressions/` found. steghide 0.5.1 has all of these.

| Input | Bug |
| --- | --- |
| `cover/bmp-win-planes`, `cover/bmp-os2-planes` | a BMP header with more than one plane failed an assertion |
| `cover/bmp-huge-size` | the width and height in a BMP header were allocated before any pixel was read: gigabytes for a small file |
| `cover/bmp-palette-short` | a BMP whose palette is shorter than its pixels' colour indices: the pixels were looked up past the end of the palette (made by hand from `tests/data/win3x8_std.bmp`) |
| `cover/jpeg-libjpeg-error` | any error in a JPEG made libjpeg call `exit()`, skipping steghide's error handling and cleanup |
| `cover/au-header-offset` | an AU data offset inside the header made the info field length wrap around to 4 GB |
| `cover/au-huge-size` | the data size in an AU header was allocated before any sample was read |
| `cover/wav-huge-chunk` | the length of an unknown WAV chunk was allocated before it was read |
| `cover/wav-bits-per-sample` | a WAV with more than 32 bits per sample failed an assertion |
| `embdata/too-short` | the embedded data was read past its end (compression flag, checksum, file name) |
| `embdata/compressed-odd-length` | compressed data whose length is not a whole number of bytes failed an assertion |
| `embdata/length-wraps`, `-2`, `-3` | an embedded data length near 2^32 wrapped around once encryption padding was added |

[libFuzzer]: https://llvm.org/docs/LibFuzzer.html
