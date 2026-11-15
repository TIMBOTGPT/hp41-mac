#!/bin/sh
# Graphics peripheral golden tests, standalone (no calculator, no HP-IL):
# every tests/gfx/*.gfx script through build/hp41-gfx --script.
HERE="$(cd "$(dirname "$0")" && pwd)"
STORE="${TMPDIR:-/tmp}/hp41-gfxstore.$$"; mkdir -p "$STORE"; trap 'rm -rf "$STORE"' EXIT
fail=0
for f in "$HERE"/gfx/*.gfx; do
  args=$(sed -n '1s/^# args: //p' "$f")              # optional first line: extra options
  out=$("$HERE/../build/hp41-gfx" --script "$f" --assets "$HERE/assets" --store "$STORE" $args) && echo "PASS  $(basename "$f"): ${out#*: }" || { echo "FAIL  $(basename "$f")"; echo "$out"; fail=1; }
done
exit $fail
