# hp41-mac

A native Apple Silicon (arm64) HP-41C/CV/CX emulator for macOS, followed by a
simulated HP-IL colour graphics peripheral driven by a real HP-41 XROM.

Sister project to [emu71-mac](https://github.com/TIMBOTGPT/emu71-mac).

## Status

The V41 emulation core runs on arm64 with an SDL2 window (`HP41.app`) and a
headless command-line build. The six Phase 1 acceptance tests pass in the
headless build (`tests/basic.sh`, `tests/phase1.sh`, `tests/clean_build.sh`),
and the stock HP-IL module talks over a virtual loop to pyILPER and to this
project's own portable C test device (`tests/hpil.sh`, `tests/ildev.sh`,
[docs/hpil.md](docs/hpil.md), which also records the throughput measured).
Phase 3 has started: a portable graphics peripheral core (`src/gfx`), a
user-code XROM (`HP41GFX`, [docs/xrom.md](docs/xrom.md)) and the protocol
([docs/graphics-protocol.md](docs/graphics-protocol.md)). An HP-41 program
calling the XROM draws axes and a sine curve on the simulated peripheral. See docs/emulator-selection.md for exactly what
has and has not been verified.

| Phase | Goal | State |
| --- | --- | --- |
| 0 | Inspect candidate engines, build one, choose with evidence | V41 R9O chosen on build evidence |
| 1 | Working, stateful HP-41CX on arm64 that runs a stock XROM | acceptance tests pass (headless) |
| 2 | Simulated HP-IL loop with a virtual test peripheral | loop, test device, error cases, replay and throughput done in emulation |
| 3 | Graphics protocol and XROM | text protocol v0, bulk blocks, peripheral core and XROM working (18 drawing functions in microcode); microcode bulk sender to do |
| 4 | 480x320 indexed-colour multi-screen peripheral on the Mac | not started |

## Documents

- [docs/HANDOFF.md](docs/HANDOFF.md) - current state, next steps and technical notes
- [PROJECT_BRIEF.md](PROJECT_BRIEF.md) - working brief (Phases 0 to 2 in detail)
- [docs/emulator-selection.md](docs/emulator-selection.md) - engine candidates and open questions
- [docs/original-brief-2026-11-14.md](docs/original-brief-2026-11-14.md) - the original brief, including the full Phase 3/4 graphics requirements

## ROM images

HP firmware is not distributed here. See [roms/README.md](roms/README.md).

## Licence

GPL version 2 or later, matching V41 (Copyright Warren Furlow and
contributors), from which the emulation core is built.

## Build

    make            # needs the V41 R9O source at ~/Projects/hp41-upstream/v41r9o/src/V41R9
    tests/basic.sh  # or: make UPSTREAM=/path/to/V41R9
    tests/phase1.sh tests/hpil.sh tests/ildev.sh tests/gfx.sh tests/persist.sh tests/e2e_sine.sh
    tests/xrom.sh   # needs the focalcod/asmnut toolchain, see docs/xrom.md
    tests/clean_build.sh
    tools/hpil_throughput.sh   # HP-IL throughput at authentic speed
    ./bundle.sh     # builds HP41.app (needs SDL2: brew install sdl2)

## Running

Put `NUT-CX.MOD` in `~/Library/Application Support/hp41-mac/MOD/`, then open
`HP41.app`. State is saved on quit to `HP-41CX.lod` in the same folder, which
also lists the modules to load (`$MODULE name` lines).

Mac keyboard: digits, `Return` = ENTER, `- = ' /` = − + × ÷, `.`, `Backspace` = ←,
`Shift` = gold shift key, letters = the key carrying that letter in ALPHA mode,
`F2` or backtick = ON, `F3` USER, `F4` PRGM, `F5` or `Tab` ALPHA, `F6` SST,
`F7` R/S, `F9` toggles turbo.

### HP-IL and the graphics peripheral

`HP41.app` starts the virtual HP-IL loop when `HPIL.MOD` is in the module
list (the `$TCPIP` line of the state file sets the address and ports,
default localhost, out 60001, in 60000). To draw with the HP41GFX XROM:

    make xrom                                       # needs the toolchain, docs/xrom.md
    tools/setup_modules.sh --add HPIL --add HP41GFX # links MOD files, adds the modules
    build/hp41-gfx --window 1 &                     # the peripheral, in its own window
    open HP41.app

Then on the calculator: `XEQ "GRAPH"`, and for example
`100 ENTER 100 XEQ "GMOVE"`, `300 ENTER 200 XEQ "GDRAW"`.
`build/hp41 --state FILE.lod` runs the app on another state file (tests).

Skins: `skin/hp41` is original artwork for this project. If a V41 source tree
is present, `tools/v41_skin.py` builds a local skin from V41's own keyboard
bitmap into `skin/local/` (not committed), and the app prefers it.

The V41 source and its MOD (ROM) files are downloaded separately from
hp.giesselink.com and are not part of this repository.
