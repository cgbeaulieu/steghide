# steghide 0.5.1 compatibility fixtures

Stego files written by an unmodified steghide 0.5.1, used to check that a
modernized build still reads them, and that 0.5.1 can still read what the
modernized build writes.

| Path | Contents |
| --- | --- |
| `fixtures/` | 50 stego files, one per case |
| `manifest.tsv` | case id, stego file, cover, passphrase, payload, embedded name, embed options |
| `payloads/` | the embedded files (`rnd40`, `rnd100`: random bytes; `text.txt`: compressible) |
| `REFERENCE` | `--version` of the binary that wrote the fixtures |

The cases cover every cover format and sub-format in `tests/data`, every
cipher, every block-cipher mode, the `-e none`, `-Z`, `-z`, `-K` and `-N`
options, and empty, non-ASCII and long passphrases.

## Checking a build

```bash
tests/compat/check.sh path/to/steghide
```

That extracts every fixture. To also embed every case with the build and
extract it with 0.5.1, pass a 0.5.1 binary as the second argument:

```bash
tests/compat/check.sh path/to/steghide /usr/bin/steghide
```

## Adding cases

Add an `embed` line to `generate.sh` and run it with a 0.5.1 binary. Embedding
is randomized, so this rewrites every fixture with new bytes; commit them
together.

```bash
tests/compat/generate.sh /usr/bin/steghide
```
