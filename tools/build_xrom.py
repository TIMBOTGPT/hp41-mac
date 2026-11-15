#!/usr/bin/env python3
"""Build a user-code XROM (MOD file) from a FOCAL source.

Steps:
  1. focalcod compiles the FOCAL source and lists every line with its bytes.
  2. The bytes become 10-bit ROM words in the user-code ROM layout:
     - one byte per word, bit 8 set on the first byte of every line
     - before each program chunk two header words used by COPY: the number
       of 7-byte registers the chunk needs, then 2b0 where b (1..7) is the
       number of bytes in the first register
     - the chunk ends with END, its last byte 0x20 with bit 9 set
     (layout checked against HP's Math Pac ROM: its first chunk is 983
     bytes = 7*140 + 3 and is preceded by 08D 230)
  3. asmnut assembles the page: XROM number, FAT (entry 0 an MCODE header
     function that only returns, then one user-code entry per global
     label), checksum, and writes the MOD file.

usage: build_xrom.py SOURCE.focal OUT.MOD --xrom N --header NAME [--title T]
env: FOCALCOD, ASMNUT, FOCALXROM (focalcod XROM database directory)
"""
import argparse, os, re, subprocess, sys, tempfile

TC = os.path.expanduser("~/Projects/hp41-upstream/toolchain")
FOCALCOD = os.environ.get("FOCALCOD", TC + "/focalcod/focalcod")
ASMNUT = os.environ.get("ASMNUT", TC + "/asmnut/asmnut")
XROMDB = os.environ.get("FOCALXROM", TC + "/focalcod/XROM")

ap = argparse.ArgumentParser()
ap.add_argument("source"); ap.add_argument("out")
ap.add_argument("--xrom", type=int, required=True)
ap.add_argument("--header", required=True)
ap.add_argument("--title", default="")
ap.add_argument("--version", default="")
ap.add_argument("--xrm", action="append", default=["ILMODULE-1H.XRM", "XFUNS3-2D.XRM"])
ap.add_argument("--listing")
a = ap.parse_args()

cmd = [FOCALCOD, "-E", "-P", XROMDB]
for x in a.xrm: cmd += ["-X", x]
cmd += ["-l-", "-x", "-o", os.devnull, a.source]
r = subprocess.run(cmd, capture_output=True, text=True)
if r.returncode or "ERROR" in r.stdout + r.stderr:
    sys.exit("focalcod failed:\n" + r.stdout + r.stderr)

# listing lines: "<text>   ; HH HH ..."
lines = []
for l in r.stdout.splitlines():
    m = re.match(r"^\s*(.*?)\s*;\s*((?:[0-9A-F]{2}\s*)+)$", l)
    if m: lines.append((m.group(1), [int(h, 16) for h in m.group(2).split()]))
if not lines or not lines[-1][0].startswith("END"):
    sys.exit("source must end with END")

# chunks end at END
chunks, cur = [], []
for text, b in lines:
    cur.append((text, b))
    if text.startswith("END"):
        chunks.append(cur); cur = []
if cur: sys.exit("code after the last END")

words, labels, listing = [], [], []
for ch in chunks:
    nbytes = sum(len(b) for _, b in ch)
    regs = (nbytes + 6) // 7
    first = nbytes - 7 * (regs - 1)
    words += [regs, 0x200 | (first << 4)]
    listing.append("       %03X %03X  ; chunk header: %d bytes, %d registers, %d in the first" % (regs, 0x200 | (first << 4), nbytes, regs, first))
    for text, b in ch:
        if text.startswith("END"):
            b = [b[0], b[1], 0x20]
        m = re.match(r'LBL "(.*)"$', text)
        if m:
            labels.append((m.group(1), len(words)))
        w = [x | (0x100 if i == 0 else 0) for i, x in enumerate(b)]
        if text.startswith("END"):
            w[-1] |= 0x200
        listing.append((text, len(words), len(w)))
        words += w

# ---- compile local GTO/XEQ for ROM ----
# ROM jumps hold plain word distances, unlike RAM's register/byte form.
# Rules fitted to HP's Math and Stat Pac ROMs (293 of 293 user-code jumps):
#   long GTO Dx / XEQ Ex, 3 words: v = (low nibble of byte 1) << 8 | byte 2;
#     byte 3 bit 7 set = toward higher addresses; target = a + 1 +/- v
#   short GTO B1..BF, 2 words: v = byte 2 & 7F, bit 7 set = higher;
#     target = a + 2 +/- v
# a is the address of the instruction's first word, relative to the code.
# The target label is found as the HP-41 searches: forward to END, then
# from the start of the chunk.
def compile_jumps(words, chunk_ranges):
    def lbl_at(i):
        w = words[i]
        if not w & 0x100: return None
        b = w & 0xFF
        if 0x01 <= b <= 0x0F: return b - 1
        if b == 0xCF: return words[i + 1] & 0x7F
        return None
    n = 0
    for lo, hi in chunk_ranges:
        for a in range(lo, hi):
            w = words[a]
            if not w & 0x100: continue
            op = w & 0xFF
            if 0xD0 <= op <= 0xEF: lab = words[a + 2] & 0x7F; size = 3
            elif 0xB1 <= op <= 0xBF: lab = op - 0xB1; size = 2
            else: continue
            order = list(range(a + size, hi)) + list(range(lo, a))
            t = next((i for i in order if lbl_at(i) == lab), None)
            if t is None: sys.exit("label %d not found for the jump at word %d" % (lab, a))
            if size == 3:
                up = t > a; v = t - a - 1 if up else a + 1 - t
                if v > 0xFFF: sys.exit("jump too long")
                words[a] = 0x100 | (op & 0xF0) | (v >> 8)
                words[a + 1] = v & 0xFF
                words[a + 2] = (words[a + 2] & 0x7F) | (0x80 if up else 0)
            else:
                up = t > a; v = t - a - 2 if up else a + 2 - t
                if v > 0x7F: sys.exit("short GTO %d too far: use a label above 14" % lab)
                words[a + 1] = (0x80 if up else 0) | v
            n += 1
    return n

ranges, pos = [], 0
for ch in chunks:
    nb = sum(len(b) for _, b in ch)
    ranges.append((pos + 2, pos + 2 + nb)); pos += 2 + nb
njumps = compile_jumps(words, ranges)

nf = 1 + len(labels)
src = [".ZENCODE", ".XROM %d" % a.xrom, ".FCNS %d" % nf, ".ORG 002", ".DEFR4K (HEADER)"]
src += [".U4KDEF (U%02d)" % i for i in range(len(labels))]
src += ["#000", "#000", '.NAME "%s"' % a.header, "(HEADER) RTN", ".UCODE"]
label_at = {pos: i for i, (_, pos) in enumerate(labels)}
for i, w in enumerate(words):
    if i in label_at:
        src.append("(U%02d)" % label_at[i])      # own line: asmnut drops a word after a label in .UCODE
    src.append("#%03X" % w)
src += [".MCODE", ".END"]

with tempfile.TemporaryDirectory() as td:
    base = os.path.join(td, "XROM")
    open(base + ".SRC", "w").write("\n".join(src) + "\n")
    out = os.path.abspath(a.out)
    acmd = [ASMNUT, "-E", "-kM:" + out]
    if a.title: acmd.append("-MT=" + a.title)
    if a.version: acmd.append("-MV=" + a.version)
    acmd += [base + ".SRC", base + ".ROM"]
    r = subprocess.run(acmd, capture_output=True, text=True, cwd=td)
    if r.returncode or not os.path.exists(out):
        sys.exit("asmnut failed:\n" + "\n".join(src[:20]) + "\n...\n" + r.stdout + r.stderr)

if a.listing:
    out = []
    for e in listing:
        if isinstance(e, str): out.append(e)
        else:
            text, at, n = e
            out.append("  %03X  %-24s %s" % (at, text, " ".join("%03X" % x for x in words[at:at + n])))
    open(a.listing, "w").write("\n".join(out) + "\n")
print("%s: XROM %d, %d functions (%s), %d user-code words in %d chunk(s), %d jumps compiled" %
      (a.out, a.xrom, nf, " ".join(n for n, _ in labels), len(words), len(chunks), njumps))
