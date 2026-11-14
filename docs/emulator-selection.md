# Emulator engine selection

Status: **V41 selected as the working engine** on build evidence (see the
last section). Candidates B and C have not been inspected or built. Each claim is marked with where it comes from.
This document is complete only when the "to verify" columns are filled from
actual inspection and build output.

## Selection criteria, in order

1. Authentic Nut CPU and ROM execution with robust plug-in ROM support.
2. Working HP-IL module emulation, ideally compatible with virtual HP-IL
   over TCP/IP as used by emu71-mac and pyILPER.
3. Reproducible arm64 macOS build.
4. Accessible source under a usable licence.
5. Effort to replace the user interface with SDL2.

MCU portability of the core is not a criterion. See PROJECT_BRIEF.md.

## Candidates

### A. V41 release 9 (Warren Furlow, Christoph Giesselink) - provisional first choice

- Reported: GPL. Windows program. Release 9 added a virtual HP-IL
  implementation, and later releases tied HP-IL controller timeouts to Nut
  instruction cycles. Giesselink documents V41 and Emu71 running in the
  same TCP/IP loop. (hpcalc.org listing, HP Museum forum release notes,
  hp.giesselink.com)
- Why first: it is the only candidate where HP-IL is reported to exist in
  the maintained version and to speak the transport this project already
  uses. The port is the same kind of job as emu71-mac (Giesselink Win32
  code to SDL2).
- To verify: where the current source is published and its exact licence
  version. How tightly the core is bound to the Windows UI. MOD file and
  plug-in ROM handling. Whether the HP-IL module is emulated at register
  level with the real ROM.

### B. Nonpareil (Eric Smith)

- Reported: GPL-2. Microcode-level, HP-41CV and CX supported, 82143A
  printer simulated. GTK+ 2 UI. MacPorts carries 0.79. HP-IL is not in the
  main line. A third-party patch from 2009 (Khanh-Dang Nguyen Thu Lam)
  added it. (GitHub README, MacPorts, HP Museum forum archive)
- Why second: cleanest and best-known Nut core, but Phase 2 would mean
  porting an old patch or writing HP-IL emulation.
- To verify: whether the 2009 HP-IL patch is still obtainable and applies.
  State of the Nut core in the current GitHub tree.
- Local note: `~/Projects/nonpareil-mba` is a macOS fork covering other
  calculator families. Whether it contains anything reusable for the 41 has
  not been checked.

### C. Emu41 / Emu41gg (Jean-Francois Garnier, Michael Moller)

- Reported: Emu41 is a 16-bit DOS program with built-in HP-IL and virtual
  devices. Source released under GPL at version 2.50 in December 2009,
  declared final. Emu41gg is a gcc port by Michael Moller. Home is
  jeffcalc.hp41.eu, not hp.giesselink.com as the original brief states.
  (jeffcalc.hp41.eu, hpcalc.org)
- Why third: proven HP-IL logic, but DOS heritage and no development since
  2009. Most useful as a reference for HP-IL behaviour.
- To verify: implementation language and how much is x86-specific. State
  and location of Emu41gg.

## What would change the ranking

- V41 source proves unavailable, or the core cannot be separated from the
  Windows UI at reasonable cost. Then B with HP-IL ported from A or C.
- V41's HP-IL turns out to be a host-level shortcut and not register-level
  module emulation. Then B or C.

## Evaluation record

| Check | V41 | Nonpareil | Emu41gg |
| --- | --- | --- | --- |
| Source obtained, version, licence text read | R9O source, GPL v2 or later (file headers) | | |
| Core separable from UI | Yes. Nine core files compile unmodified against a shim | | |
| Builds on arm64 | Yes, headless (`make`), Mach-O arm64 | | |
| Boots HP-41CX ROM | Yes, `MEMORY LOST`, then `2 ENTER 3 +` gives `5.0000` | | |
| Loads a plug-in ROM | `HPIL.MOD` and `MATH.MOD` load. No function executed yet | | |
| HP-IL module ROM runs | | | |
| Talks to pyILPER | | | |

## ROM images

Emulators in this family expect user-supplied ROM images. None are
committed here.

## V41 R9O source inspection (14 Nov 2026)

Source inspection only. Nothing has been compiled.

- Obtained `V41R9O-Source.zip` from hp.giesselink.com/V41/.
  SHA-256 `36cf4d72a7a45102497528c70e1191b1423d571945839cff5e8abd840abb8f2a`.
- Licence. Source file headers state GPL version 2 or any later version,
  copyright Warren Furlow.
- Build system is Visual Studio. `StdAfx.h` pulls in MFC (`afxwin.h` and
  related), Winsock2 and `mmsystem.h`. The UI is MFC.
- Core files and a rough count of lines that reference MFC or Win32 types:

  | File | Lines | Win/MFC references |
  | --- | ---: | ---: |
  | HP41Kernel.cpp (Nut CPU) | 2486 | 11 |
  | HP41Hpil.cpp | 478 | 2 |
  | HP41Timer.cpp | 543 | 4 |
  | HP41Display.cpp | 513 | 1 |
  | HP41Trace.cpp | 2628 | 11 |
  | HP41File.cpp (MOD loading, state) | 2782 | 55 |
  | HP41Keyboard.cpp | 1425 | 61 |
  | Tcpip.cpp | 553 | 40 |

  The CPU, HP-IL, timer and display logic are thinly coupled. File handling,
  keyboard and TCP/IP need a shim or rewrite, as with emu71-mac.
- HP-IL is behind a two-method abstract interface (`HpilIf` with
  `SendFrame` and `RecvFrame` taking a 16-bit frame). That is the natural
  attachment point for a virtual peripheral and for frame capture.
- The source package ships MOD images including `NUT-C`, `NUT-CV`, `NUT-CX`,
  `HPIL`, `HPIL-DEV`, `EXT-FUNS`, `EXT-IO`, `EXT-IL`, `TIME`, `PLOTTER` and
  `PRINTER`. These are HP firmware. They are used locally from the upstream
  download and are not committed to this repository.
- Still to verify: that the HP-IL module is emulated at register level with
  `HPIL.MOD` executing, the instruction trace facility in `HP41Trace.cpp`,
  and an actual arm64 build.

## V41 R9O headless build on arm64 (14 Nov 2026)

Observed output, MacBook Air M4, Apple clang.

- Compiled unmodified from the upstream download: `HP41Kernel`, `HP41Display`,
  `HP41Timer`, `HP41Hpil`, `HP41File`, `HP41Trace`, `HP41Printer`,
  `HP41Infrared`, `WFile`, plus `MODFile.c` and the text-only part of
  `HP41Graphics.cpp`. The build copies them into `build/gen`. Two mechanical
  edits are made at copy time: the `Draw` routine is dropped from
  `HP41Graphics.cpp`, and three typedefs are removed from `MODFile.h` so that
  `dword` stays 32 bits.
- `src/shim` supplies the Win32/MFC subset the core needs. `src/headless`
  supplies stand-ins for the UI, sound, printer channel and TCP/IP, and a
  command-line driver with scripted keys.
- `tests/basic.sh`: 7 of 7 pass (cold start, `2 ENTER 3 +`, `sin 30`,
  `STO`/`RCL`, stack roll and swap, PRGM mode showing `00 REG 219`, ALPHA entry).
- `NUT-CX.MOD` reports 4 memory modules, 1 extended memory module and the timer.
- V41's own instruction trace works (`--trace FILE`).
- Source reading confirms HP-IL is emulated at register level
  (`Hpil::exec_perph_hpil`, `HPIL_REG[9]`), so the real HP-IL ROM executes.
  Not yet exercised.

Not done or not verified:

- No UI. No persistence test. No user program test. No plug-in function run.
- The timer module clock does not tick. Windows multimedia timers are stubbed.
- HP-IL transport is a stub. Nothing is sent or received.
- Key hold time and run limits in the driver are fixed guesses that work for
  the tests above.
