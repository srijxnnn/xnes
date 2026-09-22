#!/bin/sh
# Runs the CPU against nestest.nes and compares its trace with the golden log.
set -eu

xnes=$1
rom=$2
golden=$3

expected=$(mktemp)
actual=$(mktemp)
trap 'rm -f "$expected" "$actual"' EXIT

# The golden log also carries disassembly and PPU state, which we do not emit
# yet, so reduce it to the fields the emulator actually prints.
awk '{
  regs = substr($0, index($0, "A:"), 25)
  cyc = substr($0, index($0, "CYC:"))
  sub(/\r$/, "", cyc)
  print substr($0, 1, 4), regs, cyc
}' "$golden" >"$expected"

lines=$(wc -l <"$expected" | tr -d ' ')
if [ "$lines" -lt 8000 ]; then
  echo "golden log reduced to $lines lines; the parser is broken" >&2
  exit 1
fi

"$xnes" --nestest "$rom" | head -n "$lines" >"$actual"

diff -u "$expected" "$actual"
