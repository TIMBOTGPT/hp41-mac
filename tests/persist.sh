#!/bin/sh
# Saved screens across processes: save in one run of hp41-gfx, quit, load in
# a fresh run, compare pixels and palette exactly. Then damaged files:
# truncated, wrong magic, wrong version, wrong size must give BAD FILE and
# leave the target screen untouched.
HERE="$(cd "$(dirname "$0")" && pwd)"
B="$HERE/../build/hp41-gfx"
T="${TMPDIR:-/tmp}/hp41-persist.$$"; mkdir -p "$T/store"; trap 'rm -rf "$T"' EXIT
fail=0
pass() { echo "PASS  $1"; }
bad()  { echo "FAIL  $1"; fail=1; }

# run 1: draw on screen 4 with a changed palette, save it
cat > "$T/save.gfx" <<'EOF'
GI:T 4:C 20:K 2:M 10 10:BF 100 60:K 5:M 240 160:OF 40:KP 20 7 8 9
M 300 250:L SAVED
VS SCREEN4
!err 0
!print 4
EOF
h=$("$B" --script "$T/save.gfx" --store "$T/store" | sed -n 's/.*screen 4 hash \([0-9a-f]*\).*/\1/p')
f="$T/store/SCREEN4.h41g"
size=$(wc -c < "$f" | tr -d ' ')
[ "$size" = "154380" ] && pass "saved file is 12 + 768 + 480*320 = 154380 bytes" || bad "file size $size"
hdr=$(head -c 12 "$f" | xxd -p)
[ "$hdr" = "483431470100e00140010000" ] && pass "header: H41G, version 1, raw, 480 x 320" || bad "header $hdr"

# run 2: a fresh process loads it into screen 0
cat > "$T/load.gfx" <<EOF
GI:T 0:VL SCREEN4
!err 0
!hash 0 $h
!pixel 0 50 30 2
!pixel 0 240 160 5
?P 20
!reply 7 8 9
EOF
"$B" --script "$T/load.gfx" --store "$T/store" > "$T/load.out" && pass "fresh process: loaded screen identical (hash $h), palette entry kept" || { bad "load in a fresh process"; cat "$T/load.out"; }

# damaged files
head -c 1000 "$f" > "$T/store/TRUNC.h41g"
{ printf 'XXXX'; tail -c +5 "$f"; } > "$T/store/MAGIC.h41g"
{ head -c 4 "$f"; printf '\002'; tail -c +6 "$f"; } > "$T/store/VERSION.h41g"
{ head -c 6 "$f"; printf '\000\001'; tail -c +9 "$f"; } > "$T/store/SIZE.h41g"
for n in TRUNC MAGIC VERSION SIZE; do
  printf 'GI:T 0:C 33\nVL %s\n!err 11\n!pixel 0 0 0 33\n' "$n" > "$T/bad.gfx"
  "$B" --script "$T/bad.gfx" --store "$T/store" > "$T/bad.out" && pass "$n file: BAD FILE, screen untouched" || { bad "$n file"; cat "$T/bad.out"; }
done
exit $fail
