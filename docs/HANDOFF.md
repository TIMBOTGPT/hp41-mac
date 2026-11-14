# hp41-mac handoff

Last updated 14 November 2026 (late evening). Read this first when resuming work.

## Where things are

| What | Where |
| --- | --- |
| This repository (local) | `~/Projects/hp41-mac` on the MacBook Air M4 |
| Public repository | https://github.com/TIMBOTGPT/hp41-mac |
| Upstream V41 R9O source and MOD (ROM) files | `~/Projects/hp41-upstream/v41r9o/src/V41R9` (not in git) |
| Run-time data: state file and MOD folder | `~/Library/Application Support/hp41-mac/` (`HP-41CX.lod`, `MOD` is a symlink to the upstream MOD folder) |
| hp41.org library download (manuals, SDK41R6, NutStudio, MOD files) | `~/Downloads/HP41-R3`, index `HP41.html` |
| HP Museum DVD set (manuals, books) | `/Volumes/NAS RAID/12_Hobbies/COMPUTER HOBBY/HP Calc DVDs and Docs` |
| HP internal documentation (HP-IL chip, Nut CPU, VASM listings) | `/Volumes/NAS RAID/12_Hobbies/HP DVDs and Docs/HP Stuff/Internal Documentation` |
| Reference project | `~/Projects/emu71-mac` (do not edit from this project) |
| pyILPER 1.9.1 (own venv) | `~/Projects/hp41-upstream/pyilper-venv/bin/pyilper`; settings in `~/Library/Application Support/pyilper/pyilper2`, drive image `~/pyilper_test/disk1.dat` (use a copy for write tests) |

The NAS folder `12_Hobbies/HP DVDs and Docs/HP41-DVD` is a mostly empty
directory skeleton. Use `~/Downloads/HP41-R3` instead.

## State of the work

Engine: V41 R9O (Warren Furlow, Christoph Giesselink), GPL v2 or later.
Chosen on build evidence. Nonpareil and Emu41 were never inspected or built.

Built and observed working:

- `make` builds `build/hp41-headless` (command line) and `build/hp41` (SDL2).
  `./bundle.sh` builds `HP41.app`.
- Nine V41 core files compile unmodified against `src/shim`.
- Phase 1 acceptance tests 1 to 6 pass in the headless build:
  `tests/basic.sh` (7), `tests/phase1.sh` (7), `tests/clean_build.sh`
  (clone of HEAD builds, arm64 only, all tests pass, app launches).
  - Keyed-in 17-step n! program with a GTO loop and X=0? branch: 5 gives 120,
    8 gives 40,320.
  - Advantage Pac `AND` (MCODE, XROM 22,08): 12 AND 10 gives 8, and the trace
    shows `GOTOC` at 2FBD jumping to 8309, the function's entry in page 8.
    Math Pac functions do not prove this: they are FOCAL programs in ROM.
  - Time module: `SETIME 0`, 60 emulated seconds, `TIME` shows 12:01:04 AM.
- `HP41.app`: skin, mouse and Mac keyboard, 14-segment LCD, annunciators,
  authentic speed, `F9` turbo, `F10` all-segments test. Owner approved the LCD.
- State persists across runs through V41's `.lod` format.
- Virtual HP-IL (Phase 2, all items done in emulation, see `docs/hpil.md`):
  - `src/hpil/tcpip.cpp` replaces the stub. `tests/hpil.sh` (5) passes.
  - Against pyILPER 1.9.1 by hand: `DIR` lists the LIF image, and
    `CREATE`/`WRTR`/`CLRG`/`READR` round trips data.
  - Our own device: `src/ildev/ildev.c` (portable C99 core, ported from
    pyILPER's device base class) and `build/hp41-ildev` (Mac wrapper with
    fault injection and `--replay`). `tests/ildev.sh` (13) passes: `FINDID`,
    `OUTA`, `INA`, five fault cases, replay including a committed golden
    capture in `tests/data/`.
  - Throughput (`tools/hpil_throughput.sh`): FOCAL `OUTA` loop sustains about
    40 bytes/s, 160 bytes/s inside one message, CPU-bound below about 1 ms
    loop delay per frame. Emulator timing model only.

Not done or not verified:

- Program editing and SST/BST are not tested on their own.
- Timer in `HP41.app` runs on host time and its accuracy is not checked.
- HP-IL is headless only. `HP41.app` never calls `StartHpil`.
- An unreachable loop partner is treated as a closed empty loop (V41
  design), not as the transmission error a real HP-41 would show.
- A frame returned with the wrong class (LAD as DAB) and `OUTA` to an absent
  address are not detected by the HP-IL ROM in emulation. Real behaviour
  unknown.
- Throughput and error behaviour have not been compared with real hardware.
- 82143A printer output channel (`Udp`) is discarded. No sound.
- No way to change the module list from the app. Edit the `$MODULE` lines in
  `HP-41CX.lod` by hand.
- LCD glyphs for the digit 1, the digit 4, K and the special characters have
  not been compared with a real unit.

## Next steps, in order

1. Phase 3, protocol and user-code XROM. Size the protocol from the
   throughput above: short commands, drawing on the peripheral, no bulk
   pixels from user code. The first XROM is FOCAL calling `OUTA`/`INA`
   (and `FINDID` to locate the peripheral). Evaluate NutStudio
   (`~/Downloads/HP41-R3/Utilities/NutStudio`, runs on macOS) before SDK41
   (DOS, would need DOSBox, produces `.ROM` not `.MOD`).
   The device side starts from `src/ildev`: the graphics peripheral becomes
   another device built on the same core.
2. Phase 4: the 480x320 graphics peripheral, standalone on the Mac first.
   Requirements are in `docs/original-brief-2026-11-14.md`, section 7.
3. Loose ends from Phase 2, when useful: HP-IL in `HP41.app`, a real HP-41
   error for an unreachable partner, the same loop with Emu71's calculator.

## Decisions already made

- End state: a real HP-41 or a faster emulated one, talking HP-IL to an MCU
  peripheral with a display. MCU choice (RP2350 or ESP32) is open. No
  hardware code until the whole chain runs in simulation on the Mac.
- The graphics peripheral must work standalone on the Mac.
- Graphics resolution stays 480x320, 8-bit indexed, 16 or more screens.
- The owner wants a visible replica to judge by hand, so the UI came before
  the remaining Phase 1 tests.
- Skin: the app prefers `skin/local/v41` (V41's own photographic bitmap,
  built by `tools/v41_skin.py`, never committed). `skin/hp41` is original
  artwork and is the committed fallback.

## Rules for this repository

- Public repository. No HP ROM images, no HP manuals, no V41 bitmaps, no
  SDK41 files, no long manual excerpts. hp41.org's terms allow personal and
  professional use and reserve all other rights. No source was found for HP
  permitting HP-41 ROM redistribution.
- ROM images are used locally only. Committed trace output must be short
  excerpts, not disassembly dumps.
- Commits use the identity `TIMBOTGPT <TIMBOTGPT@users.noreply.github.com>`
  (set in the local git config). No personal names or email addresses.
- Reports separate what was observed from what is unverified.

## Technical notes that cost time to find

- V41 sources use `#include"StdAfx.h"` with quotes, which searches the
  source file's own folder first. The Makefile therefore copies the core
  files into `build/gen` and supplies its own `StdAfx.h` and `V41.h` from
  `src/shim`.
- `dword` and `ulong` must be 32 bits (MOD file structures). The shim defines
  them as `uint32_t`, and the Makefile strips the conflicting typedefs from
  `MODFile.h` when copying it.
- Do not compile with `-funsigned-char`. MSVC's `char` is signed.
- `HP41Graphics.cpp` is copied without its `Draw` routine (lines 123 to 224).
- The CPU is driven by calling `HP41::Execute()` directly. Windows timers are
  stubbed. `HP41::TimerProc` must be called every 10 ms for the clock.
- `Wakeup()` needs a loaded page 0 and `eKeyboard != eKeyboardNone`, so call
  `SetKeyboard` before loading.
- `LoadConfig` and `SaveConfig` resolve MOD files and their temp file
  relative to the folder of `theApp.szAppFullPath`, then its `MOD` subfolder.
- LCD character code: `(DIS_C_REG << 8) | (DIS_B_REG << 4) | DIS_A_REG`,
  decoded with `decode_lcdchar` from `MODFile.c`.
- `DIS_ANNUN_REG` is 12 bits, BAT at bit 11 down to ALPHA at bit 0.
- Key codes: ON 18, USER c6, PRGM c5, ALPHA c4. Rows of five: 10 30 70 80 c0,
  11 31 71 81 c1, 12 32 72 82 c2. ENTER 13, CHS 73, EEX 83, back c3. Then
  14 34 74 84 (minus 7 8 9), 15 35 75 85, 16 36 76 86, 17 37 77 87.
- The headless driver runs on emulated time: 173 us per loop step, Time
  module stepped every 10 ms of it (`g_tick_source` in the shim replaces the
  host clock). `WAIT:<seconds>` in `--keys` lets time pass. Keystrokes cost
  about 0.55 s each.
- `--modinfo FILE.MOD` prints pages and the FAT, marking MCODE versus USER
  CODE functions. Use it to choose a function for trace proofs.
- `--displog` prints each display change. Needed for HP-IL functions such as
  `DIR` that show results briefly and return to X.
- `FINDID` matches seven characters. Device IDs must be seven characters
  long (the test device is `ILTESTD`).
- In `--keys`, numbers are separate digit keys (`2 0`, not `20`), and digits
  inside ALPHA need `SHIFT`.
- `--hpil HOST:OUT:IN` needs `HPIL.MOD` loaded. Tests use 61001/61000 so they
  do not collide with a pyILPER on 60001/60000.
- Scripted keys hold each key for 3000 instructions, then run until the CPU
  sleeps. These are fixed guesses that work for the current tests. A long
  running program will need `R/S` handling and a larger limit.
- HP-IL in V41 is register level (`HPIL_REG[9]`, `HpilIf::SendFrame` and
  `RecvFrame` with a 16-bit frame). `HP41::StartHpil()` creates the `TcpIp`
  object and the auto-IDY thread. The headless driver calls it for `--hpil`.
- Skin PNGs are rendered from SVG with headless Chrome
  (`tools/render_skin.sh`). Layout files are plain text, 1x pixel units.
- Screenshots for checking: `build/hp41 --keys "..." --screenshot out.bmp`.
  This also saves state, so use `--fresh` or back up `HP-41CX.lod` first.
