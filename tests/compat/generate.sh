#!/usr/bin/env bash
# Generate the steghide 0.5.1 compatibility fixtures.
#
# Usage: tests/compat/generate.sh [REFERENCE_STEGHIDE]
#
# REFERENCE_STEGHIDE must be an unmodified steghide 0.5.1 binary (default:
# "steghide" from PATH). Every fixture is embedded with it, so the fixtures
# record what real 0.5.1 writes. Embedding is randomized, so rerunning this
# produces different bytes: only rerun it when adding cases.
set -euo pipefail

ref=$(command -v "${1:-steghide}") || { echo "error: steghide not found" >&2; exit 1; }
here=$(cd "$(dirname "$0")" && pwd)
data=$here/../data
out=$here/fixtures
manifest=$here/manifest.tsv

version=$("$ref" --version | head -1)
if [[ $version != "steghide version 0.5.1" ]]; then
	echo "error: $ref reports '$version', expected steghide 0.5.1" >&2
	exit 1
fi

rm -rf "$out"
mkdir -p "$out"
printf 'id\tstego\tcover\tpassphrase\tpayload\tname\toptions\n' > "$manifest"

# embed ID COVER PAYLOAD PASSPHRASE [EMBED OPTIONS...]
embed() {
	local id=$1 cover=$2 payload=$3 pass=$4
	shift 4
	local stego=fixtures/$id.${cover##*.} name=$payload opt
	for opt in "$@"; do
		[[ $opt == -N ]] && name=
	done
	"$ref" embed -q -f -cf "$data/$cover" -ef "$here/payloads/$payload" \
		-sf "$here/$stego" -p "$pass" "$@" || { echo "error: case $id failed" >&2; exit 1; }
	printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
		"$id" "$stego" "$cover" "$pass" "$payload" "$name" "$*" >> "$manifest"
}

pass="steghide compat"

# Every cover format and sub-format, with the defaults (rijndael-128 cbc,
# zlib level 6, crc32 checksum, file name embedded). rnd40 because the
# smallest covers hold only 138 bytes once the defaults' overhead is added.
for cover in win3x1_std.bmp win3x4_std.bmp win3x8_std.bmp win3x24_std.bmp \
	os21x1_std.bmp os21x4_std.bmp os21x8_std.bmp os21x24_std.bmp \
	std.jpg prog.jpg pcm8_std.wav pcm16_std.wav \
	mulaw_std.au pcm8_std.au pcm16_std.au; do
	embed "format-${cover%.*}" "$cover" rnd40 "$pass"
done

# Every cipher, in cbc for block ciphers and stream for stream ciphers (the
# first mode encinfo lists).
while read -r algo mode; do
	embed "cipher-$algo" win3x24_std.bmp rnd100 "$pass" -e "$algo" "$mode"
done < <("$ref" encinfo | sed -n 's/^\([a-z0-9-]*\): \([a-z]*\).*/\1 \2/p')

# Every block-cipher mode.
for mode in cbc cfb ctr ecb ncfb nofb ofb; do
	embed "mode-rijndael-128-$mode" win3x24_std.bmp rnd100 "$pass" -e rijndael-128 "$mode"
done

# Embedding options.
embed opt-no-encryption win3x24_std.bmp text.txt "$pass" -e none
embed opt-no-compression win3x24_std.bmp text.txt "$pass" -Z
embed opt-compression-1 win3x24_std.bmp text.txt "$pass" -z 1
embed opt-compression-9 win3x24_std.bmp text.txt "$pass" -z 9
embed opt-no-checksum win3x24_std.bmp text.txt "$pass" -K
embed opt-no-name win3x24_std.bmp text.txt "$pass" -N
embed opt-raw win3x24_std.bmp rnd100 "$pass" -e none -Z -K -N

# Passphrases.
embed pass-empty win3x24_std.bmp rnd100 ""
embed pass-unicode win3x24_std.bmp rnd100 "pässwörd ключ 🔑"
embed pass-long win3x24_std.bmp rnd100 "$(printf 'long passphrase %.0s' {1..20})"

echo "$version" > "$here/REFERENCE"
echo "wrote $(($(wc -l < "$manifest") - 1)) fixtures with $ref ($version)"
