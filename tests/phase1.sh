#!/bin/sh
# Phase 1 acceptance tests 3, 5 and the Time module, headless build.
# Needs upstream MOD files (not in this repository).
# usage: tests/phase1.sh [path-to-MOD-dir]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
BIN="$(dirname "$0")/../build/hp41-headless"
TMP="${TMPDIR:-/tmp}/hp41-phase1.$$"
mkdir -p "$TMP"; trap 'rm -rf "$TMP"' EXIT
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }
final() { sed -n 's/^final display: \[\(.*\)\]$/\1/p'; }

# ---- 3. keyed-in user program with a loop and a conditional branch ----
# n! by repeated multiplication: R01 counts down, X=0? leaves the loop.
#   01 LBL 01  02 STO 01  03 1  04 LBL 02  05 RCL 01  06 *  07 1  08 ST- 01
#   09 RDN  10 RCL 01  11 X=0?  12 GTO 03  13 RDN  14 GTO 02
#   15 LBL 03  16 RDN  17 RTN
PROG="PRGM SHIFT STO 0 1 STO 0 1 1 SHIFT STO 0 2 RCL 0 1 * 1 STO - 0 1 RDN \
RCL 0 1 SHIFT / SHIFT RCL 0 3 RDN SHIFT RCL 0 2 SHIFT STO 0 3 RDN SHIFT EEX PRGM"
for case in "5 120.0000" "8 40,320.0000"; do
  n=${case%% *}; want=" ${case#* }"
  "$BIN" --mod "$MOD/NUT-CX.MOD" --keys "ON $PROG $n XEQ 0 1" > "$TMP/prog.out"
  got=$(final < "$TMP/prog.out")
  if [ "$got" = "$want" ]; then pass "program $n! [$got]"; else bad "program $n! expected [$want] got [$got]"; fi
done
# the program listing itself: line 17 must be RTN and the program 17 lines long
if grep -q '^key EEX .*\[17 RTN ' "$TMP/prog.out"; then pass "program listing ends at 17 RTN"; else bad "program listing"; fi

# ---- 5. plug-in ROM function, with proof the Nut CPU ran module code ----
# Advantage Pac AND (XROM 22,08) is MCODE at offset 309 of the module's lower
# page. With NUT-CX plus ADVANTAGE the loader places that page at 8, so the
# trace must show the program counter at 8309 and further inside page 8.
"$BIN" --trace "$TMP/adv.trc" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/ADVANTAGE.MOD" \
  --keys "ON 1 2 ENTER 1 0 XEQ ALPHA A N D ALPHA" > "$TMP/adv.out"
got=$(final < "$TMP/adv.out")
if [ "$got" = " 8.0000" ]; then pass "Advantage AND 12 10 [$got]"; else bad "Advantage AND expected [ 8.0000] got [$got]"; fi
grep -E '^[0-9A-F]{4}  ' "$TMP/adv.trc" > "$TMP/pc.txt"
entry=$(grep -c '^8309  ' "$TMP/pc.txt")
inpage=$(grep -c '^8' "$TMP/pc.txt")
if [ "$entry" -ge 1 ] && [ "$inpage" -ge 20 ]; then
  pass "trace: PC reached 8309 ($entry times), $inpage instructions in page 8"
  grep -B1 -A2 -m1 '^8309  ' "$TMP/pc.txt" | sed 's/^/        /'
else
  bad "trace: entry hits $entry, page 8 instructions $inpage"
fi

# ---- Time module: the clock advances with emulated time ----
# SETIME 0 sets midnight. 60 s of emulated time plus about 4 s of keying
# later, TIME must show 12:01:0x AM. A clock that does not tick shows 12:00.
"$BIN" --mod "$MOD/NUT-CX.MOD" \
  --keys "ON 0 XEQ ALPHA S E T I M E ALPHA WAIT:60 XEQ ALPHA T I M E ALPHA" > "$TMP/time.out"
got=$(final < "$TMP/time.out")
case "$got" in
  "12:01:0"?" AM") pass "clock after 60 s [$got]";;
  *) bad "clock after 60 s expected [12:01:0x AM] got [$got]";;
esac
"$BIN" --mod "$MOD/NUT-CX.MOD" \
  --keys "ON 0 XEQ ALPHA S E T I M E ALPHA WAIT:300 XEQ ALPHA T I M E ALPHA" > "$TMP/time2.out"
got=$(final < "$TMP/time2.out")
case "$got" in
  "12:05:0"?" AM") pass "clock after 300 s [$got]";;
  *) bad "clock after 300 s expected [12:05:0x AM] got [$got]";;
esac
exit $fail
