# Virtual HP-IL

Phase 2 working notes. Last updated 14 November 2026.

## How it fits together

- `HPIL.MOD` (the stock HP 82160A HP-IL module ROM) runs unmodified on the
  Nut CPU. V41's `Hpil` class emulates the 1LB3 chip at register level.
- `src/hpil/tcpip.cpp` is a POSIX port of V41's `Tcpip.cpp`. Frames are 16
  bits, big-endian, one TCP connection each way: the calculator sends to
  `HOST:OUT` and listens on `IN`. Same convention as pyILPER and Emu71.
  The server listens on loopback only (IPv4 and IPv6).
- While a frame is in transit the headless driver runs the CPU at authentic
  speed in real time (180 us per instruction, as V41 does with
  `HpilInstrNSec`), so the HP-IL ROM's timeouts match the real loop latency.
  The rest of the time it runs unthrottled on emulated time.
- `hp41-headless --hpil HOST:OUT:IN --ilog FILE --displog` starts the loop,
  logs every frame and logs display changes.
  Frame log columns: emulated seconds, host milliseconds, direction
  (`>` calculator to loop, `<` loop to calculator), frame, class, data byte.

## Observed

Automated, `tests/hpil.sh` (5 checks pass):

- No partner listening: `DIR` gives `NO DRIVE`. V41 treats an unreachable
  partner as a closed, empty loop after the first frame. A real HP-41 with
  an open loop reports a transmission error instead. Not yet handled.
- Empty loop (`tools/il_loop.py`): `DIR` gives `NO DRIVE`, 32 frames sent and
  32 received, and the device side logged the same 32 frames in the same
  order, starting with the power-on `IDY 23`.

Manual, against pyILPER 1.9.1 (TCP/IP mode, out 60001, in 60000, one
virtual drive), 14 November 2026:

- Loop setup: `AAD 01` came back as `AAD 02`, so one device took address 1.
  Its reply to Send Accessory ID was `DAB 10`, the mass-storage class.
- `DIR` displayed `T1      AS`, the ASCII file already on that LIF image.
- On a scratch copy of the image: `SIZE 003`, `"DATA"`, `3 CREATE`, 11 22 33
  into R00 to R02, `WRTR`, `CLRG` (R01 reads 0), `READR`, `RCL 01` gives
  `22.0000`. 983 frames each way, none lost. A second `DIR` listed
  `DATA    DA`. Read on the host, the LIF directory has `DATA` with type
  E0D0 and the file's first sector holds the BCD digits 11, 22 and 33.

Excerpt of the calculator-side log for the pyILPER loop setup:

        1.6272     11.429 > 581 RDY 81      AAD 01 sent
        1.6274     11.525 < 582 RDY 82      returned incremented: one device
        1.6368     11.815 > 563 RDY 63      SAI (send accessory ID)
        1.6369     11.880 < 010 DAB 10      mass storage

Reproduce: start pyILPER in TCP/IP mode with a drive, then

    build/hp41-headless --displog --mod MOD/NUT-CX.MOD --mod MOD/HPIL.MOD \
      --hpil localhost:60001:60000 --ilog dir.ilog --keys "ON XEQ ALPHA D I R ALPHA WAIT:5"

## Not done or not verified

- A loop partner that drops, corrupts or delays frames (broken loop,
  malformed command, interrupted transfer).
- Frame capture exists (`--ilog`). Deterministic replay does not.
- The minimal virtual device of our own (identity, addressing, data,
  status, errors).
- Throughput. A first look at the pyILPER session: the median gap between
  outgoing frames was 2.4 ms of emulated CPU time, 0.19 ms of host time.
  That is calculator-side cost with a near-instant partner, not a loop
  throughput figure. The measurement the brief asks for is still to do.
- HP-IL in `HP41.app` (the SDL front end does not call `StartHpil` yet).
- Not tried against Emu71's own calculator on the same loop.
