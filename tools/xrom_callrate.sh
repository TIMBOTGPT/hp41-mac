#!/bin/sh
# Time per XROM drawing call at stock speed: a program of 20 GDRAW steps in a
# row (stack unchanged, so no other steps between calls), period measured from
# the HP-IL frame log. Compares the microcode ROM with the FOCAL-only ROM, and
# OUTA alone (20 OUTA steps with ALPHA already set).
# usage: tools/xrom_callrate.sh [MOD-dir]
MOD="${1:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9/MOD}"
R="$(cd "$(dirname "$0")/.." && pwd)"
B="$R/build"
T="${TMPDIR:-/tmp}/hp41-callrate.$$"; mkdir -p "$T"; trap 'rm -rf "$T"' EXIT
X="XEQ ALPHA"
build() { python3 "$R/tools/build_xrom.py" "$R/xrom/src/hp41gfx.focal" "$T/$1.MOD" --xrom 8 --header "-HP41GFX 0A" $2 > /dev/null; }
build mc "--mcode $R/xrom/src/hp41gfx_mc.src"
build focal ""
run() { # name rom steps
  "$B/hp41-gfx" --out 61101 --in 61100 > "$T/$1.gfxout" 2>&1 &
  dev=$!
  i=0; while ! grep -q listening "$T/$1.gfxout" 2>/dev/null && [ $i -lt 100 ]; do sleep 0.05; i=$((i+1)); done
  "$B/hp41-headless" --mod "$MOD/NUT-CX.MOD" --mod "$MOD/HPIL.MOD" --mod "$T/$2.MOD" --hpil localhost:61101:61100 \
    --ilog "$T/$1.ilog" --keys "ON PRGM SHIFT STO 0 1 $X G R A P H ALPHA 1 ENTER 2 ALPHA D 0 SHIFT 1 0 SHIFT 2 ALPHA $3 PRGM XEQ 0 1" > "$T/$1.calc"
  wait $dev
  python3 - "$T/$1.ilog" "$1" <<'EOF'
import sys
ends = []
for l in open(sys.argv[1]):
    p = l.split()
    if l.startswith('#') or len(p) < 6: continue
    if p[2] == '>' and p[3] == '43F': ends.append(float(p[0]))   # UNL ends each OUTA
ends = ends[-20:]
print("%-6s %6.3f s per call (%d calls)" % (sys.argv[2], (ends[-1] - ends[0]) / (len(ends) - 1), len(ends)))
EOF
}
G=""; O=""
for i in $(seq 20); do G="$G $X G D R A W ALPHA"; O="$O $X O U T A ALPHA"; done
run mcode mc "$G"
run focal focal "$G"
run outa mc "$O"
