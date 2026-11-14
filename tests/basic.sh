#!/bin/sh
# Phase 1 smoke tests for the headless build. Needs upstream MOD files.
# usage: tests/basic.sh [path-to-MOD-dir]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
BIN="$(dirname "$0")/../build/hp41-headless"
fail=0
check() { # name, keys, expected final display
  got=$("$BIN" --mod "$MOD/NUT-CX.MOD" --keys "$2" | sed -n 's/^final display: \[\(.*\)\]$/\1/p')
  if [ "$got" = "$3" ]; then echo "PASS  $1  [$got]"; else echo "FAIL  $1  expected [$3] got [$got]"; fail=1; fi
}
check "cold start"        "ON"                                   "MEMORY LOST"
check "2 ENTER 3 +"       "ON 2 ENTER 3 +"                       " 5.0000"
check "sin 30 (DEG)"      "ON 3 0 SIN"                           " 0.5000"
check "STO 01 / RCL 01"   "ON 5 STO 0 1 BACK RCL 0 1"            " 5.0000"
check "stack RDN X<>Y"    "ON 1 ENTER 2 ENTER 3 RDN X<>Y"        " 1.0000"
check "PRGM mode"         "ON PRGM"                              "00 REG 219"
check "ALPHA entry"       "ON ALPHA A B"                         "AB_"
exit $fail
