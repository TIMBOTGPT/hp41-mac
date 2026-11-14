#!/bin/sh
# HP-IL throughput at authentic HP-41CX speed (emulated time, 173 us per
# instruction), with the stock HP-IL module and hp41-ildev as the listener.
# A keyed-in FOCAL program sends a 15-character ALPHA string with OUTA N
# times (17 bytes each with CR LF). Repeated for several artificial loop
# delays per frame, to show how much the loop latency matters.
# usage: tools/hpil_throughput.sh [N] [MOD-dir]    env: DELAYS="0 250 500 1000 2000" MODE=AUTOIO|MANIO SPEED=1 (CPU speed multiple)
HERE="$(cd "$(dirname "$0")" && pwd)"
B="$HERE/../build"
N="${1:-20}"; MOD="${2:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
KN=$(echo "$N" | sed 's/./& /g')
TMP="${TMPDIR:-/tmp}/hp41-tput.$$"; mkdir -p "$TMP"; trap 'rm -rf "$TMP"' EXIT
X="XEQ ALPHA"
# 01 LBL 01  02 STO 01  03 "ABCDEFGHIJKLMNO"  04 LBL 02  05 OUTA  06 1  07 ST- 01
# 08 RDN  09 RCL 01  10 X=0?  11 RTN  12 RDN  13 GTO 02
PROG="PRGM SHIFT STO 0 1 STO 0 1 ALPHA A B C D E F G H I J K L M N O ALPHA SHIFT STO 0 2 \
$X O U T A ALPHA 1 STO - 0 1 RDN RCL 0 1 SHIFT / SHIFT EEX RDN SHIFT RCL 0 2 PRGM"
SETUP="ON 1 $X S E L E C T ALPHA"
[ "${MODE:-AUTOIO}" = MANIO ] && SETUP="ON $X M A N I O ALPHA 1 $X S E L E C T ALPHA"
echo "mode ${MODE:-AUTOIO}, speed ${SPEED:-1}x, $N x OUTA of 17 bytes"
for d in ${DELAYS:-0 250 500 1000 2000}; do
  "$B/hp41-ildev" --out 61001 --in 61000 --delay-us "$d" > "$TMP/dev.out" 2>&1 &
  dev=$!
  i=0; while ! grep -q listening "$TMP/dev.out" 2>/dev/null && [ $i -lt 100 ]; do sleep 0.05; i=$((i+1)); done
  "$B/hp41-headless" --speed "${SPEED:-1}" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --hpil localhost:61001:61000 \
     --ilog "$TMP/t.ilog" --keys "$SETUP $PROG $KN XEQ 0 1" > "$TMP/calc.out"
  sleep 0.2; kill $dev 2>/dev/null; wait $dev 2>/dev/null
  python3 "$HERE/il_throughput.py" "$TMP/t.ilog" "$d"
  grep -q "$N messages received" "$TMP/dev.out" || echo "  warning: device saw $(sed -n 's/.*frames, \([0-9]*\) messages.*/\1/p' "$TMP/dev.out") messages"
done
