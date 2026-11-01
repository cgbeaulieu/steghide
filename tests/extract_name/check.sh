#!/usr/bin/env bash
# Check that steghide extract writes the extracted file to the current
# directory when the file name embedded in the stego file has a directory.
#
# Usage: tests/extract_name/check.sh STEGHIDE
#
# dotdot.bmp holds a file embedded under the name "../escaped.txt" with the
# passphrase "traversal". steghide strips directories when it embeds, so it
# was made with a build whose EmbData::getBitString did not. steghide 0.5.1
# writes escaped.txt to the parent directory.
set -uo pipefail

if [[ $# -ne 1 ]]; then
	sed -n '4,6s/^# \{0,1\}//p' "$0" >&2
	exit 2
fi
sut=$(realpath "$(command -v "$1")") || { echo "error: $1 not found" >&2; exit 2; }
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

mkdir -p "$tmp/parent/cwd"
if ! (cd "$tmp/parent/cwd" && "$sut" extract -q -sf "$here/dotdot.bmp" -p traversal </dev/null); then
	echo "FAIL: extract failed"
	exit 1
fi
if [[ -e $tmp/parent/escaped.txt ]]; then
	echo "FAIL: wrote escaped.txt outside the current directory"
	exit 1
fi
if [[ ! -f $tmp/parent/cwd/escaped.txt ]]; then
	echo "FAIL: did not write escaped.txt to the current directory"
	exit 1
fi
echo "PASS"
