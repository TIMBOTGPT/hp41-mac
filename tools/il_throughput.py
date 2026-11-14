#!/usr/bin/env python3
"""Throughput from an hp41-headless --ilog capture of repeated OUTA calls.

Uses emulated time (authentic speed). A message ends with its LF data frame.
  cycle      LF to LF of successive messages: sustained rate, all overhead included
  in-message first data byte to LF within one message: rate while bytes flow
usage: il_throughput.py CAPTURE [label]
"""
import sys

rows = [l.split() for l in open(sys.argv[1]) if not l.startswith('#')]
label = sys.argv[2] if len(sys.argv) > 2 else ""
out = [(float(r[0]), int(r[3], 16)) for r in rows if r[2] == '>']
lf = [t for t, f in out if f & 0x7FF == 0x00A]
data = [t for t, f in out if (f & 0x600) in (0x000, 0x200)]
if len(lf) < 2:
    sys.exit("delay %s: only %d messages in the capture" % (label, len(lf)))
starts = [min(t for t in data if t > a) for a in lf[:-1]]
cycle = sum(b - a for a, b in zip(lf, lf[1:])) / (len(lf) - 1)
inmsg = sum(b - a for a, b in zip(starts, lf[1:])) / (len(lf) - 1)
per_msg = round(len(data) / len(lf))      # data frames per message (includes any SAI reply bytes)
print("delay %5s us/frame: %3d messages, %4d frames; cycle %6.1f ms = %5.1f bytes/s; in-message %5.1f ms/byte = %5.1f bytes/s; %4.1f frames per byte"
      % (label, len(lf), len(out), cycle * 1000, 17 / cycle, inmsg / 16 * 1000, 16 / inmsg, len(out) / (len(lf) * 17)))
