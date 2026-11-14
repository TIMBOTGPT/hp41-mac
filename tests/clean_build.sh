#!/bin/sh
# Phase 1 acceptance test 6: a clean checkout builds and launches on arm64.
# Clones the committed HEAD of this repository into a temporary folder (so
# uncommitted and ignored files such as skin/local are absent), builds it,
# checks both binaries are arm64 only, runs the headless tests from the clone
# and launches the SDL front end once with a hidden window.
# Dependencies: Xcode command line tools (clang++), Homebrew sdl2, and the V41
# R9O source tree (UPSTREAM, default ~/Projects/hp41-upstream/v41r9o/src/V41R9).
# usage: tests/clean_build.sh
set -u
REPO="$(cd "$(dirname "$0")/.." && pwd)"
UPSTREAM="${UPSTREAM:-$HOME/Projects/hp41-upstream/v41r9o/src/V41R9}"
WORK="${TMPDIR:-/tmp}/hp41-clean.$$"
STATE="$HOME/Library/Application Support/hp41-mac/HP-41CX.lod"
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }
restore() { [ -f "$WORK/state.bak" ] && cp "$WORK/state.bak" "$STATE"; rm -rf "$WORK"; }
trap restore EXIT

git clone -q "$REPO" "$WORK/src" || { echo "FAIL  clone"; exit 1; }
echo "clean checkout of $(git -C "$WORK/src" rev-parse --short HEAD) in $WORK/src"
if make -C "$WORK/src" UPSTREAM="$UPSTREAM" > "$WORK/make.log" 2>&1; then pass "make"; else bad "make (see below)"; tail -20 "$WORK/make.log"; exit 1; fi
for b in hp41-headless hp41 hp41-ildev; do
  a=$(lipo -archs "$WORK/src/build/$b")
  if [ "$a" = "arm64" ]; then pass "$b is $a only"; else bad "$b architectures [$a]"; fi
done
sh "$WORK/src/tests/basic.sh"  "$UPSTREAM/MOD" || fail=1
sh "$WORK/src/tests/phase1.sh" "$UPSTREAM/MOD" || fail=1
sh "$WORK/src/tests/hpil.sh"   "$UPSTREAM/MOD" || fail=1
sh "$WORK/src/tests/ildev.sh"  "$UPSTREAM/MOD" || fail=1

# launch the window build once; it saves state on exit, so keep the user's copy
[ -f "$STATE" ] && cp "$STATE" "$WORK/state.bak"
if (cd "$WORK/src" && ./build/hp41 --keys "2 ENTER 3 +" --screenshot "$WORK/launch.bmp") > "$WORK/app.log" 2>&1 \
   && [ -s "$WORK/launch.bmp" ]; then
  pass "SDL front end launched and drew a frame ($(wc -c < "$WORK/launch.bmp" | tr -d ' ') byte screenshot)"
else
  bad "SDL front end launch"; cat "$WORK/app.log"
fi
exit $fail
