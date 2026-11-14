#!/bin/sh
# Phase 2 virtual HP-IL tests with the stock HP-IL module ROM (HPIL.MOD).
# Uses tools/il_loop.py (an empty loop) on ports 61001/61000, so a pyILPER
# on the default ports 60001/60000 is not disturbed.
# usage: tests/hpil.sh [path-to-MOD-dir]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN="$HERE/../build/hp41-headless"
TMP="${TMPDIR:-/tmp}/hp41-hpil.$$"
mkdir -p "$TMP"; trap 'rm -rf "$TMP"; [ -n "${LOOP:-}" ] && kill $LOOP 2>/dev/null' EXIT
OUT=61001; IN=61000
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }
final() { sed -n 's/^final display: \[\(.*\)\]$/\1/p' "$1"; }
DIR="ON XEQ ALPHA D I R ALPHA"

# 1. no partner listening: V41 treats the loop as closed and empty
"$BIN" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --hpil localhost:$OUT:$IN --keys "$DIR" > "$TMP/none.out"
got=$(final "$TMP/none.out")
[ "$got" = "NO DRIVE" ] && pass "no partner: DIR [$got]" || bad "no partner: DIR expected [NO DRIVE] got [$got]"

# 2. empty loop: every frame goes round and comes back, both sides log it
python3 "$HERE/../tools/il_loop.py" --out $OUT --in $IN --log "$TMP/dev.log" > "$TMP/loop.out" 2>&1 &
LOOP=$!
sleep 0.5
"$BIN" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --hpil localhost:$OUT:$IN --ilog "$TMP/calc.log" --keys "$DIR" > "$TMP/loop1.out"
got=$(final "$TMP/loop1.out")
[ "$got" = "NO DRIVE" ] && pass "empty loop: DIR [$got]" || bad "empty loop: DIR expected [NO DRIVE] got [$got]"
sleep 0.3
sent=$(grep -c ' > ' "$TMP/calc.log"); recv=$(grep -c ' < ' "$TMP/calc.log"); dev=$(wc -l < "$TMP/dev.log" | tr -d ' ')
if [ "$sent" -gt 10 ] && [ "$sent" = "$recv" ] && [ "$sent" = "$dev" ]; then
  pass "empty loop: $sent frames sent, $recv received, $dev seen by the device"
else
  bad "empty loop: sent $sent received $recv device $dev"
fi
awk '$3==">" {print $4}' "$TMP/calc.log" > "$TMP/a"; awk '{print $2}' "$TMP/dev.log" > "$TMP/b"
cmp -s "$TMP/a" "$TMP/b" && pass "empty loop: device saw the calculator's frames in order" || bad "frame sequences differ"
first=$(head -1 "$TMP/b")
[ "$first" = "623" ] && pass "first frame is IDY 23 (HP-IL module power-on)" || bad "first frame [$first]"
exit $fail
