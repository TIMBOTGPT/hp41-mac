# hp41-mac working brief

Revised 14 November 2026 from the original brief
([docs/original-brief-2026-11-14.md](docs/original-brief-2026-11-14.md)).
Where the two disagree, this document wins for Phases 0 to 2. The original
remains the requirements source for the graphics peripheral (Phases 3 and 4).

## Goal

1. A working HP-41CX emulator running natively on Apple Silicon.
2. A simulated HP-IL loop carrying traffic from authentic HP-41 code to a
   virtual peripheral.
3. A colour graphics peripheral (480x320, 8-bit indexed, 16 or more screens,
   pixel readback, sprites) that runs standalone on the Mac and is driven by
   a real XROM over that loop.

## End state (decided 14 Nov 2026)

The calculator side is either a real HP-41 or a faster emulated one. Both
reach the peripheral over HP-IL. The peripheral will eventually be an MCU
with a display and an HP-IL interface. The MCU choice (RP2350, ESP32 or
other) is deliberately open and no hardware code is written until the whole
chain works in simulation on the Mac.

Consequences:

- The emulator core does not need to be portable to an MCU. It only needs to
  be correct and maintainable on macOS.
- The renderer, asset store and protocol decoder do need to be portable.
  Plain C, no SDL, no macOS calls, no dynamic allocation assumptions that an
  MCU could not meet, configurable screen count and memory budget.
- The graphics peripheral must run and be testable as a standalone Mac
  program with no calculator attached.

## Rules

- Calculator first. No graphics, XROM or hardware work until Phase 1 passes.
- Authentic execution. Nut CPU running real ROM images. No interception of
  function names by host code.
- No unverified claims. Every report separates source inspection, compiled
  features, observed test output and intended work.
- No HP ROM images, HP skin artwork or HP branding in the repository.
- `emu71-mac` is a reference and is not edited by this project.

## Phase 0 - choose the engine with evidence

See [docs/emulator-selection.md](docs/emulator-selection.md). Output is a
filled-in evaluation and one engine that builds and boots on arm64.

## Phase 1 - working calculator

Required:

- arm64 native, no Rosetta.
- HP-41CX configuration (CV and C if cheap).
- Display, annunciators, keyboard from mouse and Mac keys.
- User programs can be entered, edited, run and single-stepped.
- RAM and program state persist across quit and relaunch. State location
  documented. Changing the module configuration must not silently wipe it.
- Plug-in ROM modules load, appear in the catalogue and execute.
- Selectable speed (authentic and unthrottled).
- SDL2 front end in the style of `emu71-mac`.
- **Headless test harness.** Scripted key input and display/register
  readback from the command line, so acceptance tests run without a person
  clicking. This is a Phase 1 deliverable, not a later convenience.

Acceptance tests, all scripted and repeatable:

1. Boot to a normal display.
2. `2 ENTER 3 +` gives `5`. Representative trig, STO/RCL and stack tests pass.
3. A keyed-in program with a loop or branch gives the expected result.
4. Save, quit, relaunch. Registers and programs are intact.
5. A stock plug-in ROM is loaded and one of its functions run. Proof that
   the Nut CPU executed module code is an instruction trace showing the
   program counter inside the module's page.
6. Clean checkout builds and launches on arm64 from documented dependencies.

## Phase 2 - simulated HP-IL

- HP-IL module emulated at the hardware-register level so the real HP-IL
  module ROM runs unmodified.
- Loop transport compatible with the virtual HP-IL TCP/IP convention already
  used by `emu71-mac` and pyILPER, if the chosen engine allows it.
- A minimal virtual device with identity, addressing, data, status and
  error behaviour.
- Driven first by the stock HP-IL module functions. No custom XROM yet.
- Frame capture and deterministic replay.
- Error cases. No device, broken loop, malformed command, interrupted
  transfer.
- **Throughput measurement at authentic calculator speed**, recorded in the
  repo. The graphics protocol is not designed until this number exists.

Acceptance: authentic HP-41 code sends a message through the emulated module,
the virtual device receives and acknowledges it, and both sides log the
exchange.

## Phase 3 - protocol and XROM (outline)

Full requirements are in the original brief, sections 6 to 8. Two changes:

1. **User-code XROM first.** The first XROM is a user-code (FOCAL) ROM image
   that calls the HP-IL module's own functions. It still executes on the Nut
   CPU, proves the protocol end to end and avoids writing HP-IL drivers in
   microcode before the protocol is stable. A microcode XROM follows.
2. The protocol is sized from the Phase 2 throughput figure. Drawing
   primitives execute on the peripheral. Bulk pixel transfer is designed
   around what a real loop can carry.

## Phase 4 - graphics peripheral (outline)

Requirements are in the original brief, section 7, unchanged. The peripheral
is built as a standalone Mac program first, with its own command-line test
driver and pixel-exact golden tests, then attached to the loop.

## Open questions

- Engine choice (Phase 0).
- Whether the HP-IL module's stock functions are sufficient for the
  user-code XROM, or whether the Extended I/O ROM is also needed.
- HP-IL throughput on a real HP-41, and whether full-screen readback is
  practical on hardware.
- Microcode toolchain for the later XROM.
- Palette conversion policy for cross-screen blits.
- Licence version, which follows the engine.
