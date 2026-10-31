#!/usr/bin/env bash
# Check a steghide build against the 0.5.1 compatibility fixtures.
#
# Usage: tests/compat/check.sh STEGHIDE [REFERENCE_STEGHIDE]
#
# Extracts every fixture with STEGHIDE and compares the payload and the
# embedded file name. With REFERENCE_STEGHIDE (a steghide 0.5.1 binary), also
# re-embeds every case with STEGHIDE and extracts it with the reference, to
# check that 0.5.1 can still read what STEGHIDE writes.
set -uo pipefail

if [[ $# -lt 1 ]]; then
	sed -n '4,9s/^# \{0,1\}//p' "$0" >&2
	exit 2
fi
resolve() { local p; p=$(command -v "$1") && realpath "$p"; }
sut=$(resolve "$1") || { echo "error: $1 not found" >&2; exit 2; }
ref=
if [[ $# -ge 2 ]]; then
	ref=$(resolve "$2") || { echo "error: $2 not found" >&2; exit 2; }
fi

here=$(cd "$(dirname "$0")" && pwd)
data=$here/../data
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

passed=0
failed=0
fail() {
	echo "FAIL $1"
	failed=$((failed + 1))
}

# extract BINARY STEGO PASSPHRASE NAME DIR: extract into DIR; prints the path
# of the extracted file. Uses the embedded name unless the stego file has none.
extract() {
	local bin=$1 stego=$2 pass=$3 name=$4 dir=$5
	mkdir -p "$dir"
	if [[ -n $name ]]; then
		(cd "$dir" && "$bin" extract -q -f -sf "$stego" -p "$pass" </dev/null) && echo "$dir/$name"
	else
		"$bin" extract -q -f -sf "$stego" -p "$pass" -xf "$dir/payload" </dev/null && echo "$dir/payload"
	fi
}

# Tabs become \037 first: read collapses runs of whitespace IFS characters,
# which would drop the empty passphrase and name fields.
while IFS=$'\037' read -r id stego cover pass payload name options; do
	[[ $id == id ]] && continue
	read -ra opts <<< "$options"

	# 0.5.1 wrote it, STEGHIDE reads it.
	if ! out=$(extract "$sut" "$here/$stego" "$pass" "$name" "$tmp/$id/read" 2>"$tmp/err"); then
		fail "$id: extract failed: $(head -1 "$tmp/err")"
	elif [[ ! -f $out ]]; then
		fail "$id: extracted file not named '$name'"
	elif ! cmp -s "$out" "$here/payloads/$payload"; then
		fail "$id: extracted payload differs"
	else
		passed=$((passed + 1))
	fi

	[[ -n $ref ]] || continue

	# STEGHIDE writes it, 0.5.1 reads it.
	sf=$tmp/$id/write.${cover##*.}
	if ! "$sut" embed -q -f -cf "$data/$cover" -ef "$here/payloads/$payload" \
		-sf "$sf" -p "$pass" "${opts[@]}" </dev/null 2>"$tmp/err"; then
		fail "$id (write): embed failed: $(head -1 "$tmp/err")"
	elif ! out=$(extract "$ref" "$sf" "$pass" "$name" "$tmp/$id/write" 2>"$tmp/err"); then
		fail "$id (write): reference extract failed: $(head -1 "$tmp/err")"
	elif [[ ! -f $out ]] || ! cmp -s "$out" "$here/payloads/$payload"; then
		fail "$id (write): reference extracted a different payload"
	else
		passed=$((passed + 1))
	fi
done < <(tr '\t' '\037' < "$here/manifest.tsv")

echo "$passed passed, $failed failed"
[[ $failed -eq 0 ]]
