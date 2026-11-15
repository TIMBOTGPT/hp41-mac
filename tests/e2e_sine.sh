#!/bin/sh
# Phase 3 end to end, before the XROM: an HP-41 FOCAL program (keyed in)
# draws axes and a sine curve on hp41-gfx over the virtual HP-IL loop.
# The program computes the points itself (SIN, ARCL) and sends protocol
# lines with OUTA. Checks: the lines the peripheral received equal the
# documented golden script tests/gfx/sine.gfx line for line, and the
# rendered screen has the golden hash.
# usage: tests/e2e_sine.sh [MOD-dir] [--keep DIR]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
HERE="$(cd "$(dirname "$0")" && pwd)"
B="$HERE/../build"
TMP="${KEEP:-${TMPDIR:-/tmp}/hp41-e2e.$$}"; mkdir -p "$TMP"
[ -z "${KEEP:-}" ] && trap 'rm -rf "$TMP"' EXIT
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }
A() { python3 "$HERE/../tools/alpha_keys.py" "$1"; }
T() { echo "ALPHA $(A "$1") ALPHA"; }                 # text line
AP() { echo "ALPHA SHIFT XEQ $(A "$1") ALPHA"; }      # appended text line
X="XEQ ALPHA"
OUTA="$X O U T A ALPHA"
# 01 LBL 01  RAD  FIX 4
# "GI:C 0" OUTA   "SC -7 7 -1.2 1." "|-2" OUTA   "K 1:M -7 0:D 7" "|- 0" OUTA
# "K 1:M 0 -1.2:D" "|- 0 1.2" OUTA   "K 2" OUTA
# 63 STO 03   -6.3 STO 01  SIN STO 02  "M " ARCL 01 "|- " ARCL 02 OUTA
# LBL 02  .2 ST+ 01  RCL 01 SIN STO 02  "D " ARCL 01 "|- " ARCL 02 OUTA
#         1 ST- 03  RCL 03  X=0?  RTN  GTO 02
PROG="PRGM SHIFT STO 0 1 $X R A D ALPHA SHIFT 1 4 \
$(T 'GI:C 0') $OUTA $(T 'SC -7 7 -1.2 1.') $(AP '2') $OUTA \
$(T 'K 1:M -7 0:D 7') $(AP ' 0') $OUTA $(T 'K 1:M 0 -1.2:D') $(AP ' 0 1.2') $OUTA $(T 'K 2') $OUTA \
6 3 STO 0 3 6 . 3 CHS STO 0 1 SIN STO 0 2 $(T 'M ') ALPHA SHIFT RCL 0 1 ALPHA $(AP ' ') ALPHA SHIFT RCL 0 2 ALPHA $OUTA \
SHIFT STO 0 2 . 2 STO + 0 1 RCL 0 1 SIN STO 0 2 $(T 'D ') ALPHA SHIFT RCL 0 1 ALPHA $(AP ' ') ALPHA SHIFT RCL 0 2 ALPHA $OUTA \
1 STO - 0 3 RCL 0 3 SHIFT / SHIFT EEX SHIFT RCL 0 2 PRGM"
KEYS="ON $(T HP41GFX) $X F I N D I D ALPHA $X S E L E C T ALPHA $PROG XEQ 0 1"

"$B/hp41-gfx" --out 61001 --in 61000 --log "$TMP/gfx.log" --png "$TMP/sine.png" --hash 1 > "$TMP/gfx.out" 2>&1 &
dev=$!
i=0; while ! grep -q listening "$TMP/gfx.out" 2>/dev/null && [ $i -lt 100 ]; do sleep 0.05; i=$((i+1)); done
"$B/hp41-headless" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --hpil localhost:61001:61000 \
  --ilog "$TMP/calc.ilog" --keys "$KEYS" > "$TMP/calc.out"
wait $dev

sed -n 's/^ *[0-9.]* line \[\(.*\)\] -> .*/\1/p' "$TMP/gfx.log" > "$TMP/received.txt"
grep -v '^[#!]' "$HERE/gfx/sine.gfx" > "$TMP/expected.txt"
n=$(wc -l < "$TMP/received.txt" | tr -d ' ')
if cmp -s "$TMP/received.txt" "$TMP/expected.txt"; then pass "peripheral received the documented $n lines, identical to tests/gfx/sine.gfx"
else bad "received lines differ from the golden script:"; diff "$TMP/expected.txt" "$TMP/received.txt" | head -10; fi
want=$(sed -n 's/^!hash 0 //p' "$HERE/gfx/sine.gfx"); got=$(sed -n 's/.*screen 0 hash \([0-9a-f]*\).*/\1/p' "$TMP/gfx.out")
[ "$got" = "$want" ] && pass "rendered screen 0 matches the golden hash $want" || bad "screen hash $got, golden $want"
grep -q 'ok' "$TMP/gfx.log" && ! grep -q -- '-> [A-Z]' "$TMP/gfx.log" && pass "no protocol errors" || bad "protocol errors: $(grep -- '-> [A-Z]' "$TMP/gfx.log" | head -3)"
echo "      HP-41 emulated time $(sed -n 's/^emulated time: //p' "$TMP/calc.out"), $(sed -n 's/^hpil frames: //p' "$TMP/calc.out")"
exit $fail
