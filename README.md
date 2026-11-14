# hp41-mac

A native Apple Silicon (arm64) HP-41C/CV/CX emulator for macOS, followed by a
simulated HP-IL colour graphics peripheral driven by a real HP-41 XROM.

Sister project to [emu71-mac](https://github.com/TIMBOTGPT/emu71-mac).

## Status

**Nothing is built yet.** This repository currently holds the project brief
and the emulator-engine evaluation. No emulator code, no test results.

| Phase | Goal | State |
| --- | --- | --- |
| 0 | Inspect candidate engines, build one, choose with evidence | in progress |
| 1 | Working, stateful HP-41CX on arm64 that runs a stock XROM | not started |
| 2 | Simulated HP-IL loop with a virtual test peripheral | not started |
| 3 | Graphics protocol and XROM | not started |
| 4 | 480x320 indexed-colour multi-screen peripheral on the Mac | not started |

## Documents

- [PROJECT_BRIEF.md](PROJECT_BRIEF.md) - working brief (Phases 0 to 2 in detail)
- [docs/emulator-selection.md](docs/emulator-selection.md) - engine candidates and open questions
- [docs/original-brief-2026-11-14.md](docs/original-brief-2026-11-14.md) - the original brief, including the full Phase 3/4 graphics requirements

## ROM images

HP firmware is not distributed here. See [roms/README.md](roms/README.md).

## Licence

Not yet set. Every candidate engine is GPL, so this project will be GPL too.
The exact version follows the engine chosen in Phase 0.
