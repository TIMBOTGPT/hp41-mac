#!/bin/sh
# Phase 2: the HP-41's stock HP-IL module against our own virtual device
# (build/hp41-ildev, portable core in src/ildev). Ports 61001/61000.
# usage: tests/ildev.sh [path-to-MOD-dir]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
HERE="$(cd "$(dirname "$0")" && pwd)"
B="$HERE/../build"
TMP="${TMPDIR:-/tmp}/hp41-ildev.$$"
mkdir -p "$TMP"; trap 'rm -rf "$TMP"' EXIT
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }
final() { sed -n 's/^final display: \[\(.*\)\]$/\1/p' "$1"; }
shown() { grep -q "^ .*\[$2\]$" "$1"; }      # display showed this at some point (--displog)
X="XEQ ALPHA"
# FINDID "ILTESTD", SELECT, OUTA "HELLO", CLA, INA, show ALPHA.
# In this sequence the OUTA data frames are 56-62 and the INA data frames 72-78.
FULL="ON ALPHA I L T E S T D ALPHA $X F I N D I D ALPHA $X S E L E C T ALPHA ALPHA H E L L O ALPHA $X O U T A ALPHA ALPHA SHIFT BACK ALPHA $X I N A ALPHA ALPHA"

session() { # name, device options
  "$B/hp41-ildev" --out 61001 --in 61000 --log "$TMP/$1.dev" $2 > "$TMP/$1.devout" 2>&1 &
  dev=$!
  # wait for the device to listen (a freshly built binary can be slow to start the first time)
  i=0; while ! grep -q listening "$TMP/$1.devout" 2>/dev/null && [ $i -lt 100 ]; do sleep 0.05; i=$((i+1)); done
  "$B/hp41-headless" --displog --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --hpil localhost:61001:61000 \
     --ilog "$TMP/$1.ilog" --keys "$FULL" > "$TMP/$1.calc"
  sleep 0.2; kill $dev 2>/dev/null; wait $dev 2>/dev/null
}

# ---- normal operation ----
session clean ""
shown "$TMP/clean.calc" " 1.0000" && pass "FINDID ILTESTD gives address 1" || bad "FINDID"
if grep -q 'messages received' "$TMP/clean.devout" && grep -q '1 messages received, 7 data bytes in, 7 out, last message "HELLO' "$TMP/clean.devout"; then
  pass "device received HELLO CR LF from OUTA and sent 7 bytes back"; else bad "device: $(tail -1 "$TMP/clean.devout")"; fi
got=$(final "$TMP/clean.calc")
[ "$got" = "HELLO" ] && pass "INA read HELLO back into ALPHA" || bad "INA: ALPHA [$got]"
grep -q 'ERR' "$TMP/clean.calc" && bad "an error was displayed in the clean session" || pass "no error displayed"

# ---- replay: deterministic, catches a changed device ----
"$B/hp41-ildev" --replay "$TMP/clean.ilog" > "$TMP/r1" && pass "replay of this capture: $(sed 's/replay: //' "$TMP/r1" | cut -d';' -f1)" || bad "replay: $(cat "$TMP/r1")"
"$B/hp41-ildev" --replay "$HERE/data/findid_outa_ina.ilog" > "$TMP/r2" && pass "replay of the committed golden capture: $(sed 's/replay: //' "$TMP/r2" | cut -d';' -f1)" || bad "golden replay: $(cat "$TMP/r2")"
"$B/hp41-ildev" --replay "$TMP/clean.ilog" --did OTHERID > "$TMP/r3" && bad "replay with a different device ID should differ" || pass "replay with a different device ID differs ($(tail -1 "$TMP/r3" | sed 's/.*frames, //; s/;.*//'))"

# ---- faults ----
session corrupt_out "--corrupt 58"     # a data byte of OUTA comes back altered
shown "$TMP/corrupt_out.calc" "TRANSMIT ERR" && pass "corrupted OUTA byte: TRANSMIT ERR" || bad "corrupted OUTA byte not detected"
session corrupt_in "--corrupt 74"      # a data byte of INA reaches the calculator altered
shown "$TMP/corrupt_in.calc" "TRANSMIT ERR" && [ "$(final "$TMP/corrupt_in.calc")" = "HELM" ] \
  && pass "corrupted INA byte: TRANSMIT ERR, ALPHA holds HELM" || bad "corrupted INA byte: final [$(final "$TMP/corrupt_in.calc")]"
"$B/hp41-ildev" --replay "$TMP/corrupt_in.ilog" > "$TMP/r4"; grep -q '^frame 74:' "$TMP/r4" && grep -q ' 1 differ' "$TMP/r4" \
  && pass "replay pinpoints the corrupted frame (74)" || bad "replay of corrupted capture: $(cat "$TMP/r4")"
session broken "--drop-from 58"        # loop breaks during OUTA
shown "$TMP/broken.calc" "TRANSMIT ERR" && pass "broken loop during OUTA: TRANSMIT ERR" || bad "broken loop not reported"
session cut "--cut 58"                 # partner disconnects during OUTA
shown "$TMP/cut.calc" "TRANSMIT ERR" && pass "partner disconnects during OUTA: TRANSMIT ERR" || bad "disconnect not reported"
session wrongclass "--wrongclass 54"   # LAD 1 comes back as a data frame
[ "$(final "$TMP/wrongclass.calc")" = "HELLO" ] && ! grep -q ERR "$TMP/wrongclass.calc" \
  && pass "LAD returned as DAB: not detected by the HP-IL ROM (recorded behaviour)" || bad "wrong-class frame: behaviour changed"
exit $fail
