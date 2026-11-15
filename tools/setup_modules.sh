#!/bin/sh
# Make ~/Library/Application Support/hp41-mac/MOD a folder of links: every
# MOD file of the V41 download, plus this repository's HP41GFX.MOD
# (xrom/build, built with `make xrom`). Nothing is copied into the upstream
# tree. Optionally add modules to the state file's list.
# usage: tools/setup_modules.sh [--add HPIL --add HP41GFX ...]
set -e
REPO="$(cd "$(dirname "$0")/.." && pwd)"
UP="${UPSTREAM:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9}/MOD"
DATA="$HOME/Library/Application Support/hp41-mac"
LOD="$DATA/HP-41CX.lod"
mkdir -p "$DATA"
if [ -L "$DATA/MOD" ]; then rm "$DATA/MOD"; fi
mkdir -p "$DATA/MOD"
for f in "$UP"/*.MOD "$UP"/*.mod; do [ -e "$f" ] && ln -sf "$f" "$DATA/MOD/$(basename "$f")"; done
[ -e "$REPO/xrom/build/HP41GFX.MOD" ] && ln -sf "$REPO/xrom/build/HP41GFX.MOD" "$DATA/MOD/HP41GFX.MOD"
echo "MOD folder: $(ls "$DATA/MOD" | wc -l | tr -d ' ') links in $DATA/MOD"
while [ $# -gt 1 ] && [ "$1" = "--add" ]; do
  m="$2"; shift 2
  [ -e "$DATA/MOD/$m.MOD" ] || { echo "no $m.MOD in $DATA/MOD"; exit 1; }
  if [ -f "$LOD" ] && ! grep -q "^\$MODULE $m[[:space:]]*\(;.*\)\?\$" "$LOD" && ! grep -q "^\$MODULE $m\$" "$LOD"; then
    cp "$LOD" "$LOD.before-$m"
    # insert after the last $MODULE line
    awk -v m="$m" '{ lines[NR]=$0 } /^\$MODULE/ { last=NR } END { for (i=1;i<=NR;i++) { print lines[i]; if (i==last) printf "$MODULE %s\r\n", m } }' "$LOD.before-$m" > "$LOD"
    echo "added \$MODULE $m (previous state kept as $(basename "$LOD").before-$m)"
  else
    echo "\$MODULE $m already listed"
  fi
done
