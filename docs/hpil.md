# Virtual HP-IL

Phase 2 working notes. Last updated 14 November 2026.

## How it fits together

- `HPIL.MOD` (the stock HP 82160A HP-IL module ROM) runs unmodified on the
  Nut CPU. V41's `Hpil` class emulates the HP-IL chip at register level.
- `src/hpil/tcpip.cpp` is a POSIX port of V41's `Tcpip.cpp`. Frames are 16
  bits, big-endian, one TCP connection each way: the calculator sends to
  `HOST:OUT` and listens on `IN`. Same convention as pyILPER and Emu71.
  The server listens on loopback only (IPv4 and IPv6).
- While a frame is in transit the headless driver runs the CPU at authentic
  speed in real time (180 us per instruction, as V41 does with
  `HpilInstrNSec`), so the HP-IL ROM's timeouts match the real loop latency.
  The rest of the time it runs unthrottled on emulated time. Loop latency
  therefore shows up in emulated time, roughly one to one.
- `hp41-headless --hpil HOST:OUT:IN --ilog FILE --displog` starts the loop,
  logs every frame and logs display changes. Frame log columns: emulated
  seconds, host milliseconds, direction (`>` calculator to loop, `<` loop to
  calculator), frame, class, data byte.

## Our virtual device

`src/ildev/ildev.c` is the device core: plain C99, no allocation, no
operating system calls, built with `-Wall -Wextra -pedantic -Werror`. Its
addressing and talker/listener state machine is ported from pyILPER's
`pildevbase.py` (from ILPER and Christoph Giesselink's C++ base class,
GPL v2 or later). It is meant to move to the MCU unchanged.

The test device (`build/hp41-ildev`, default ID `ILTESTD`, accessory ID 66):

- As listener it collects bytes into a message, ended by LF or an END frame.
- As talker (SDA) it sends the last message back as DAB frames, then EOT.
  With nothing to send, SDA passes on unchanged, as in pyILPER.
- Answers SAI, SDI and SST. Handles AAU, AAD, AEP, AES, LAD, TAD, SAD, UNL,
  UNT, IFC, SDC and DCL.
- Fault injection in the Mac wrapper, by frame number: `--drop`,
  `--drop-from` (broken loop), `--corrupt`, `--wrongclass`, `--cut`
  (disconnect), `--delay-us`.
- `--replay CAPTURE` feeds the calculator's frames from an `--ilog` capture
  to a fresh device and checks every reply against the capture.

The accessory ID 66 is provisional. Its class meaning has not been checked.

Learned on the way:

- `FINDID` compares the first seven characters, as the HP-IL module manual
  says (page 53). A six-character ID (`ILTEST`) was never found. A seven-
  character ID is found with or without CR LF after it.
- Sending the last byte of a talker message as an END frame failed: the
  HP-41 passed it on as a DAB frame, the device counted a talker error and
  the HP-41 showed TRANSMIT ERR. All bytes now go as DAB. Whether a real
  HP-41 does the same is not known.

## Observed

`tests/hpil.sh` (5 checks) and `tests/ildev.sh` (13 checks) pass.

Normal operation with `hp41-ildev`: `FINDID` gives 1, `SELECT`, `OUTA`
delivers `HELLO` CR LF to the device, `CLA`, `INA` reads `HELLO` back into
ALPHA. 81 frames each way, no error shown.

Error cases, HP-41 behaviour in emulation:

| Fault | HP-41 shows |
| --- | --- |
| No partner listening at all | `NO DRIVE` for `DIR`. V41 treats the loop as closed and empty. A real HP-41 with an open loop reports a transmission error |
| A data byte of `OUTA` comes back altered | TRANSMIT ERR |
| A data byte of `INA` reaches the HP-41 altered | ALPHA holds `HELM`, the device answers ETE, TRANSMIT ERR |
| Loop breaks during `OUTA` | TRANSMIT ERR, after about 45 s of emulated time |
| One frame dropped during `OUTA` | TRANSMIT ERR, same delay |
| Partner disconnects during `OUTA` | TRANSMIT ERR, same delay |
| `LAD 1` comes back as a data frame | not detected, the session completes |
| `OUTA` to an address no device holds | not detected |
| 5 ms delay on every frame | works, 0.36 s slower |

Replay: the clean session replays with 0 of 81 frames different, the golden
capture `tests/data/findid_outa_ina.ilog` likewise. Replaying with another
device ID differs from the first ID byte on. Replaying the corrupted-`INA`
session flags exactly frame 74, the corrupted one.

Against pyILPER 1.9.1 by hand (TCP/IP mode, one virtual drive):

- `AAD 01` came back as `AAD 02` (one device). Send Accessory ID gave
  `DAB 10`, mass storage.
- `DIR` displayed `T1      AS`, the file already on that LIF image.
- On a scratch copy of the image: `SIZE 003`, `"DATA"`, `3 CREATE`, 11 22 33
  into R00 to R02, `WRTR`, `CLRG`, `READR`, `RCL 01` gives `22.0000`.
  983 frames each way. The LIF directory read on the host has `DATA`, type
  E0D0, with the BCD digits 11, 22 and 33 in its first sector.

Excerpt of the calculator-side log for the pyILPER loop setup:

        1.6272     11.429 > 581 RDY 81      AAD 01 sent
        1.6274     11.525 < 582 RDY 82      returned incremented: one device
        1.6368     11.815 > 563 RDY 63      SAI (send accessory ID)
        1.6369     11.880 < 010 DAB 10      mass storage

## Throughput

Measured with `tools/hpil_throughput.sh 20`: a keyed-in FOCAL loop sends a
15-character ALPHA string with `OUTA` 20 times (17 bytes each with CR LF) to
`hp41-ildev`. Times are emulated time at authentic speed (173 us per
instruction, V41's figure for a halfnut HP-41CX). `--delay-us` adds an
artificial delay to every frame at the device.

| Mode | Delay per frame | Sustained (LF to LF) | Inside one message | Frames per byte |
| --- | --- | --- | --- | --- |
| AUTOIO | 0 | 40.1 bytes/s | 160.0 bytes/s (6.25 ms per byte) | 1.6 |
| AUTOIO | 250 us | 38.9 | 148.4 | 1.6 |
| AUTOIO | 500 us | 38.6 | 144.1 | 1.6 |
| AUTOIO | 1 ms | 37.1 | 131.0 | 1.6 |
| AUTOIO | 2 ms | 34.6 | 111.6 | 1.6 |
| MANIO | 0 | 34.9 | 160.0 | 2.0 |
| MANIO | 1 ms | 32.7 | 131.1 | 2.0 |

Reading:

- Each `OUTA` call costs about 320 ms besides its bytes (FOCAL, re-addressing
  the loop with AAU and AAD before every call in AUTOIO, LAD, UNL). That
  overhead, not the loop, sets the sustained rate. MANIO does not help.
- Inside a message a byte costs 6.25 ms of CPU, about 36 instructions. Loop
  latency adds about 1.6 times the per-frame delay per byte.
- At these rates a full 480x320 8-bit screen (153,600 bytes) takes about
  64 minutes from FOCAL `OUTA` calls, or 16 minutes at the in-message rate.
  The graphics protocol has to keep drawing on the peripheral and send
  short commands, and bulk pixels cannot come from user code.

Caveats: this is the emulator's timing model. It has not been compared with
a real HP-41 and a real loop. The model counts V41's instruction cycle time
and the measured host latency. It does not model the HP-IL chip's own frame
transmission time on a real loop. A microcode XROM could cut the per-call
overhead; how far is unknown until one exists.

## Not done or not verified

- The no-partner case does not reproduce a real HP-41's error.
- Throughput against real hardware.
- HP-IL in `HP41.app` (the SDL front end does not call `StartHpil` yet).
- Not tried with Emu71's calculator in the same loop.
- The device's SRQ, status and power functions are present but untested.
