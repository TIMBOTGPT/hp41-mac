#!/usr/bin/env python3
"""Empty virtual HP-IL loop for tests: returns every frame unchanged.

Same convention as V41, pyILPER and Emu71: 16-bit frames, big-endian, over TCP.
The calculator connects to OUT (we listen); we connect back to its IN port.

usage: il_loop.py [--out 60001] [--in 60000] [--log FILE] [--frames N]
  --log FILE   write one line per frame as the device side saw it
  --frames N   exit after N frames (default: run until the calculator disconnects)
"""
import argparse, socket, struct, sys, time

ap = argparse.ArgumentParser()
ap.add_argument("--out", type=int, default=60001)
ap.add_argument("--in", dest="inp", type=int, default=60000)
ap.add_argument("--log")
ap.add_argument("--frames", type=int, default=0)
a = ap.parse_args()

CLASSES = ["DAB", "DSR", "END", "ESR", "CMD", "RDY", "IDY", "ISR"]
log = open(a.log, "w") if a.log else None
srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", a.out))
srv.listen(1)
print("listening on", a.out, flush=True)
c, _ = srv.accept()
back, n, t0 = None, 0, time.monotonic()
while True:
    d = b""
    while len(d) < 2:
        part = c.recv(2 - len(d))
        if not part:
            break
        d += part
    if len(d) < 2:
        break
    f = struct.unpack(">H", d)[0]
    n += 1
    if log:
        log.write("%10.3f %03X %s %02X\n" % ((time.monotonic() - t0) * 1000, f & 0x7FF, CLASSES[(f >> 8) & 7], f & 0xFF))
        log.flush()
    while back is None:
        try:
            back = socket.create_connection(("127.0.0.1", a.inp), timeout=2)
        except OSError:
            time.sleep(0.05)
    back.sendall(d)
    if a.frames and n >= a.frames:
        break
print("frames:", n, flush=True)
