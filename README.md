# hp41-mac

A native Apple Silicon (arm64) HP-41C/CV/CX emulator for macOS, followed by a
simulated HP-IL colour graphics peripheral driven by a real HP-41 XROM.

Sister project to [emu71-mac](https://github.com/TIMBOTGPT/emu71-mac).

## Status

The V41 emulation core runs on arm64 with an SDL2 window (`HP41.app`) and a
headless command-line build. The headless build passes 7 smoke tests against
the HP-41CX ROM, and state persists across runs. Phase 1 is not complete. See docs/emulator-selection.md for exactly what
has and has not been verified.

| Phase | Goal | State |
| --- | --- | --- |
| 0 | Inspect candidate engines, build one, choose with evidence | V41 R9O chosen on build evidence |
| 1 | Working, stateful HP-41CX on arm64 that runs a stock XROM | in progress, headless only |
| 2 | Simulated HP-IL loop with a virtual test peripheral | not started |
| 3 | Graphics protocol and XROM | not started |
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
    ./bundle.sh     # builds HP41.app (needs SDL2: brew install sdl2)

## Running

Put `NUT-CX.MOD` in `~/Library/Application Support/hp41-mac/MOD/`, then open
`HP41.app`. State is saved on quit to `HP-41CX.lod` in the same folder, which
also lists the modules to load (`$MODULE name` lines).

Mac keyboard: digits, `Return` = ENTER, `- = ' /` = − + × ÷, `.`, `Backspace` = ←,
`Shift` = gold shift key, letters = the key carrying that letter in ALPHA mode,
`F2` or backtick = ON, `F3` USER, `F4` PRGM, `F5` or `Tab` ALPHA, `F6` SST,
`F7` R/S, `F9` toggles turbo.

Skins: `skin/hp41` is original artwork for this project. If a V41 source tree
is present, `tools/v41_skin.py` builds a local skin from V41's own keyboard
bitmap into `skin/local/` (not committed), and the app prefers it.

The V41 source and its MOD (ROM) files are downloaded separately from
hp.giesselink.com and are not part of this repository.
