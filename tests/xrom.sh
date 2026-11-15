#!/bin/sh
# Phase 3 acceptance: a user program calls the HP41GFX user-code XROM to
# draw axes and a sine curve over the virtual HP-IL loop.
# Builds xrom/build/HP41GFX.MOD (needs the focalcod and asmnut toolchain,
# see docs/xrom.md), then checks:
#   - the MOD file: 13 functions, names, checksum
#   - PSET then PGET round trip through the XROM functions
#   - the traffic of the sine program is the documented protocol, line by line
#   - the rendered screen has the golden hash of tests/gfx/sine.gfx
# usage: tests/xrom.sh [MOD-dir]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
HERE="$(cd "$(dirname "$0")" && pwd)"
R="$HERE/.."
B="$R/build"
TMP="${KEEP:-${TMPDIR:-/tmp}/hp41-xrom.$$}"; mkdir -p "$TMP"
[ -z "${KEEP:-}" ] && trap 'rm -rf "$TMP"' EXIT
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }
X="XEQ ALPHA"
XR="$TMP/HP41GFX.MOD"

python3 "$R/tools/build_xrom.py" "$R/xrom/src/hp41gfx.focal" "$XR" --xrom 17 --header "-HP41GFX 0A" \
  --title "HP41GFX graphics v0" > "$TMP/build.out" 2>&1 && pass "XROM built: $(sed 's/.*: //' "$TMP/build.out")" || { bad "XROM build"; cat "$TMP/build.out"; exit 1; }
"$B/hp41-headless" --modinfo "$XR" > "$TMP/modinfo"
names=$(grep -E '^17,(0[1-9]|1[0-2]) ' "$TMP/modinfo" | awk '{print $3}' | tr '\n' ' ')
[ "$names" = "GRAPH GCLEAR GCOLOR GTARG GSHOW GMOVE GDRAW GPLOT GSCALE GPIXEL PSET PGET " ] && pass "FAT: $names" || bad "FAT names [$names]"
grep -q 'Correct - Same as Computed Value' "$TMP/modinfo" && pass "ROM checksum correct" || bad "ROM checksum"

session() { # name, keys
  "$B/hp41-gfx" --out 61001 --in 61000 --log "$TMP/$1.gfxlog" --png "$TMP/$1.png" --hash 1 > "$TMP/$1.gfxout" 2>&1 &
  dev=$!
  i=0; while ! grep -q listening "$TMP/$1.gfxout" 2>/dev/null && [ $i -lt 100 ]; do sleep 0.05; i=$((i+1)); done
  "$B/hp41-headless" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --mod "$XR" --hpil localhost:61001:61000 \
    --ilog "$TMP/$1.ilog" --keys "$2" > "$TMP/$1.calc"
  wait $dev
}
final() { sed -n 's/^final display: \[\(.*\)\]$/\1/p' "$1"; }

# PSET 1 1 5, PGET 1 1
session pix "ON $X G R A P H ALPHA 1 ENTER 1 ENTER 5 $X P S E T ALPHA 1 ENTER 1 $X P G E T ALPHA"
got=$(final "$TMP/pix.calc")
[ "$got" = " 5.0000" ] && pass "PSET 1 1 5 then PGET 1 1 returns 5 in X" || bad "PGET returned [$got]"

# user program: axes and sine curve, all drawing through the XROM
# 01 LBL 01  GRAPH  -7 ENTER 7 ENTER -1.2 ENTER 1.2 GSCALE  1 GCOLOR
# -7 ENTER 0 GMOVE  7 ENTER 0 GDRAW  0 ENTER -1.2 GMOVE  0 ENTER 1.2 GDRAW  2 GCOLOR
# RAD  63 STO 03  -6.3 STO 01  ENTER SIN GMOVE
# LBL 02  .2 ST+ 01  RCL 01 ENTER SIN GDRAW  1 ST- 03  RCL 03  X=0?  RTN  GTO 02
F() { echo "$X $1 ALPHA"; }
PROG="PRGM SHIFT STO 0 1 $(F 'G R A P H') 7 CHS ENTER 7 ENTER 1 . 2 CHS ENTER 1 . 2 $(F 'G S C A L E') 1 $(F 'G C O L O R') \
7 CHS ENTER 0 $(F 'G M O V E') 7 ENTER 0 $(F 'G D R A W') 0 ENTER 1 . 2 CHS $(F 'G M O V E') 0 ENTER 1 . 2 $(F 'G D R A W') \
2 $(F 'G C O L O R') $(F 'R A D') 6 3 STO 0 3 6 . 3 CHS STO 0 1 ENTER SIN $(F 'G M O V E') \
SHIFT STO 0 2 . 2 STO + 0 1 RCL 0 1 ENTER SIN $(F 'G D R A W') 1 STO - 0 3 RCL 0 3 SHIFT / SHIFT EEX SHIFT RCL 0 2 PRGM"
session sine "ON $PROG XEQ 0 1"

sed -n 's/^ *[0-9.]* line \[\(.*\)\] -> .*/\1/p' "$TMP/sine.gfxlog" > "$TMP/received.txt"
{ printf 'GI:C 0\nSX -7.0000 7.0000\nSY -1.2000 1.2000\nK 1\nM -7.0000 0.0000\nD 7.0000 0.0000\nM 0.0000 -1.2000\nD 0.0000 1.2000\nK 2\n'
  sed -n '/^M -6.3000/,$p' "$HERE/gfx/sine.gfx" | grep -v '^!'; } > "$TMP/expected.txt"
n=$(wc -l < "$TMP/received.txt" | tr -d ' ')
if cmp -s "$TMP/received.txt" "$TMP/expected.txt"; then pass "HP-IL traffic is the documented protocol: $n lines as expected"
else bad "traffic differs from the expected lines:"; diff "$TMP/expected.txt" "$TMP/received.txt" | head -10; fi
want=$(sed -n 's/^!hash 0 //p' "$HERE/gfx/sine.gfx"); got=$(sed -n 's/.*screen 0 hash \([0-9a-f]*\).*/\1/p' "$TMP/sine.gfxout")
[ "$got" = "$want" ] && pass "rendered screen matches the golden hash $want" || bad "screen hash $got, golden $want"
grep -q -- '-> [A-Z]' "$TMP/sine.gfxlog" && bad "protocol errors: $(grep -- '-> [A-Z]' "$TMP/sine.gfxlog" | head -3)" || pass "no protocol errors"
grep -q 'NONEXISTENT\|NO HP41GFX\|TRANSMIT ERR\|DATA ERROR' "$TMP/sine.calc" && bad "calculator showed an error" || pass "no calculator error shown"
grep -q 'XROMG.GDRAW' "$TMP/sine.calc" && pass "program steps are XROM calls (PRGM mode shows XROM \"GDRAW\")" || bad "program steps are not XROM calls"
echo "      HP-41 emulated time $(sed -n 's/^emulated time: //p' "$TMP/sine.calc")"
exit $fail
