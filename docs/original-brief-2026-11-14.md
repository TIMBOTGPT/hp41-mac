# HP-41 Mac Emulator and Virtual Graphics Peripheral

**Project brief and implementation requirements — 14 November 2026**  
**Status:** Design brief; implementation not yet started  
**Target development machine:** MacBook Air M4, Apple Silicon, macOS  
**Intended repository:** A **new, separate** repository (suggested name: `hp41-mac`) under `TIMBOTGPT`  
**Reference implementation:** <https://github.com/TIMBOTGPT/emu71-mac>

## 1. Purpose and overall vision

Develop a **genuinely functioning HP-41 calculator emulator first**, running natively on Apple Silicon, and subsequently build a simulated external colour graphics peripheral driven by a **real HP-41 XROM** through a **simulated HP-IL connection**. The whole calculator-to-peripheral chain must be testable on the Mac **before buying or constructing hardware**. The eventual physical display should execute substantially the same graphics commands and protocol as the simulator; the XROM must not need to know whether the connected peripheral is virtual or physical.

The user has already ported Christoph Giesselink's HP-71B Emu71 to Apple Silicon with Claude and added a graphical display system. This existing project is an architectural and design reference—not a codebase to modify in place. The HP-41 is a different calculator family, with a Nut processor and different ROM/extension architecture. Do not attempt to transplant the HP-71 Saturn CPU code, LEX module, or memory-mapped mailbox directly into the HP-41.

**Non-negotiable priority:** Achieve and verify a reliable working calculator before implementing a graphics peripheral or designing a new XROM. Avoid building a visually convincing shell around a nonfunctional or behavioural approximation.

## 2. Existing Emu71 reference project

Repository: <https://github.com/TIMBOTGPT/emu71-mac>

The repository README describes a native arm64 Emu71 port with SDL2 replacing Win32 UI; keyboard and LCD, scalable skins, calculator state saving, HP-IL over a TCP/IP loop, and experimental external video. It also documents launch/build scripts and test hooks.

The reference document `docs/video-architecture.md` is especially important. It describes a **host-side graphics engine** invoked through a **custom HP-71 soft-configured memory-mapped video mailbox**, with LEX commands providing BASIC-language graphics statements. **This graphical interface is not currently transported over HP-IL.** The Emu71 repository separately implements HP-IL for mass-storage/peripheral use. A new HP-41 implementation must bridge graphics through a different mechanism, preferably the HP-IL peripheral protocol for future physical compatibility.

Relevant existing design ideas to reuse/adapt:

- SDL2 user interface, original-style calculator skin, keyboard mapping, scalable layout, macOS packaging and state persistence.
- A host-side raster renderer with drawing primitives, scales, clipping, text, palettes, stored screens/layers and persistence.
- A clearly documented command protocol and automated image tests.
- An existing indexed-colour, multi-screen design including `GTARGET`, `GSHOW`, `GROLL`, `GBLIT`, `PGET`/`PSET` and `VREAD`/`VWRITE` concepts.
- HP-IL TCP/IP interoperability and independent testing with tools such as pyILPER.

Existing Emu71 video functionality is reported in its README as 800×600 by default with 16 indexed layers. The new HP-41 graphics system will have a different logical size: **480×320**.

**Before extracting/reusing source:** inspect the repository's actual implementation, its current version, its licence obligations, and unit tests. Prefer a self-contained renderer library whose public API does not depend on HP-71 BCD layouts, Saturn registers, SDL windows or HP-71-specific mailbox offsets.

## 3. Development philosophy

1. **Simulation-first and incremental.** Every stage must run and be tested on the M4 Mac before hardware is considered.
2. **Faithful HP-41 execution.** Use a genuine Nut CPU/microcode or equivalent machine-code-capable emulator and authentic compatible ROM images where legally obtained. The custom XROM must execute normally, not via interception of function names by the Mac UI.
3. **Protocol realism.** A virtual HP-IL peripheral must receive the same logical addressing, command/data framing, acknowledgments, status and errors anticipated for future hardware. Instrument and record exchanges to support debugging and replay.
4. **Separate concerns.** Keep calculator core, ROM/extension loading, HP-IL transport, graphics protocol, renderer, and display UI distinct.
5. **Portable graphics engine.** Aim to share graphics command parsing and rendering algorithms between macOS and an eventual embedded device, with replaceable screen drivers, storage and transport backends.
6. **Testability over presentation.** Working CPU, keyboard, ROM loading and reproducible tests precede cosmetic enhancements.
7. **No unverified success claims.** Clearly distinguish source-inspection findings, compiled features, observed test output and intended work.

## 4. Phase 1 — Deliver a working native HP-41 calculator (FIRST MILESTONE)

### Required features

- Runs natively as an arm64 macOS program on the MacBook Air M4 (no Rosetta dependency).
- Working **HP-41C/CV/CX-family** calculator with priority given to an **HP-41CX-capable configuration**, subject to ROM availability and emulator support.
- Accurate display, annunciators and calculator keyboard behaviour; usable from mouse and Mac keyboard.
- Genuine HP-41 program execution, RPN stack, registers, modes and standard mathematical functions.
- Enter, edit, run and debug simple user programs, including conditional instructions and loops.
- Persistent calculator RAM/program state across quit/relaunch. Document where state lives; avoid silently destroying it when module configurations change.
- Load and execute compatible plug-in ROM modules/XROMs (verify correct function discovery/catalogue and execution).
- Reliable error reporting and basic diagnostic logging.
- A launchable native Mac app, with reproducible source build and instructions. SDL2 consistent with `emu71-mac` is preferred **if it does not delay core validation**.

### Emulator-source selection: evaluate before committing

**Candidate A: Nonpareil** — Eric Smith's open-source microcode-level HP calculator simulator, with HP-41 support. Its older GTK-based interface might need replacement. Investigate its codebase, licence, Apple Silicon toolchain, available HP-41 models, ROM loader, state handling and HP-IL functionality. Earlier discussion identified a MacPorts route, but **do not assume the version, current package support, or experimental HP-IL variant is functional without verifying them**.

**Candidate B: Emu41 / Emu41gg** — Jean-François Garnier's Emu41 family and a GCC-oriented port. Investigate code licensing, source availability, buildability on current macOS arm64, ROM/XROM support and how much virtual HP-IL functionality survives in the available port.

**Selection criteria in order:** (1) authentic CPU/ROM execution and robust XROM support; (2) reproducible native Apple Silicon build; (3) accessible, extensible source and suitable licence; (4) feasible HP-IL emulation; (5) maintainable UI integration. Choose based on inspected source and actual build/tests, not claims alone. If source licence or ROM redistribution is unclear, stop and document it rather than copying files unlawfully.

**Reference links for investigation:**

- Nonpareil: <https://github.com/brouhaha/nonpareil>
- Emu41: <https://hp.giesselink.com/emu41.htm> (verify correct source and derivative links)
- Existing Mac project: <https://github.com/TIMBOTGPT/emu71-mac>

**ROM legal requirement:** Do not commit HP proprietary ROM images to a public repository without appropriate redistribution rights. Make the build/load procedure work with user-supplied legitimate images and explain the required filenames and locations. Separate open-source project assets from firmware.

### Phase 1 acceptance tests (must actually pass)

- Start the emulator and observe normal calculator boot/display.
- Use physical/mouse keyboard to calculate `2 ENTER 3 +` yielding `5` and execute representative trig, storage/recall and stack operations.
- Enter and run a short HP-41 user program with a loop or branching; verify its result.
- Save state, quit completely, relaunch and verify registers/programs persist.
- Load a known compatible XROM image and execute one of its functions, confirming that the actual emulated CPU executes the module code.
- Build and launch from a clean checkout on arm64 using documented dependencies.

**Phase 1 is complete only when the tests above are repeatable. Do not defer them in favour of the graphics UI.**

## 5. Phase 2 — Prove simulated HP-IL independently

The eventual physical-device strategy requires a genuine HP-IL connection. The HP-41C/CV/CX does **not** include HP-IL without an HP-IL module or an equivalent simulation. Model the module in the emulator; later hardware will require a compatible HP-IL interface.

- Enable and validate HP-IL module/ROM support within the selected emulator.
- Connect to a **virtual HP-IL peripheral** on the Mac, ideally via a well-defined loop or frame interface; assess pyILPER compatibility and reuse proven transport conventions when possible.
- Implement peripheral identity/addressing, command/data handling, statuses, error handling and loop behaviour—not simply a direct host callback triggered by an XROM function.
- Before any custom XROM, use **existing HP-41 HP-IL functions** to send test messages to a minimal virtual device; confirm traffic in logs and verify responses.
- Support deterministic packet/frame capture and replay to drive regression tests.
- Include error tests (device not present, broken loop, malformed command, out-of-range request, interrupted transfer).
- Separate application-level graphics commands from HP-IL framing so a different physical interface can be implemented later without changing the commands.

**Acceptance:** the emulator running authentic HP-41 code transmits a test message through the simulated HP-IL module; the external virtual peripheral receives and acknowledges it; both sides record the exchange.

## 6. Phase 3 — Define and prove the graphics protocol and XROM

Build a custom **genuine HP-41 XROM** with named graphics functions. The XROM consumes arguments from the four-level RPN stack and ALPHA register as appropriate and emits protocol messages through the simulated HP-IL interface. Its code must run in the emulator's Nut CPU execution environment, not be substituted by host-language implementations behind familiar function names.

The first XROM subset should be deliberately small: `GRAPH`, `GCLEAR`, `GMOVE`, `GDRAW`, `GPLOT`, `GTARGET`, `GSHOW`, `PGET` and `PSET`. Names are proposals; validate HP-41 function-name length, syntax, ROM ID, addressing, stack effects, existing-name conflicts and SDK/toolchain requirements before finalising. For drawing functions, document precise stack argument order and whether the function consumes/restores stack elements. Avoid assuming HP-71 BASIC command signatures carry over directly.

Next add `GSCALE`, `GBOX`, `GCIRC`, `GTEXT`, palette operations, `GROLL`, `GBLIT`, bulk pixel transfer, sprite/image operations and image persistence. Prefer the smallest useful command packets, with integer coordinates and efficiently packed data where possible. The HP-41 has little RAM and is slow compared with the Mac; higher-level drawing primitives should run on the peripheral, not via per-pixel instructions on the calculator.

Investigate HP-41 ROM assembly and XROM packaging tools (including SDK41-related resources), and build a repeatable toolchain producing a standard loadable ROM image. Provide readable source and automated tests for the binary.

**Acceptance:** a user program calls the actual XROM to draw axes and a sine curve; captured HP-IL traffic corresponds to the documented protocol; the simulated peripheral renders the correct image.

## 7. Phase 4 — Graphics peripheral capabilities (agreed requirements)

### Display model and colour

- **Fixed logical resolution: 480 × 320 pixels** (3:2), scalable in the Mac window without changing logical coordinates.
- **8-bit indexed colour per pixel**: 256 palette entries per framebuffer. Retain RGB palette information for each indexed colour.
- **Independent per-screen palettes** are the working design preference. A pixel read returns the *index* (0–255); add a separate operation to query its RGB palette entry.
- When copying between screens with different palettes, define explicit **copy-index** versus **preserve-appearance/colour-convert** behaviour. Do not silently remap indices.
- Retain the ability to display graphics on a standard Mac screen, and eventually on commodity 480×320 LCD/TFT hardware.

### Multi-screen/layer architecture

- **At least 16** complete, independent 480×320 indexed-colour framebuffers; design addressing and protocol for **32 or 64** (or more if practical).
- A clear distinction between **drawing target** (`GTARGET`) and **currently displayed screen** (`GSHOW`); allow off-screen drawing followed by instant switching.
- Rotate/cycle through screens, swap screens, clear/create/free screens and copy/blit regions between them. Maintain deterministic semantics for operations such as `GROLL`, `GSWAP` and `GBLIT`.
- Initially, one screen is visible at a time; simultaneous alpha compositing of multiple overlays is a **possible later extension**, not a Phase 1 requirement. Transparency can initially be supported for blit/sprite operations.
- A screen may be saved and restored independently, including its palette and pixel data. Persistent storage format must be documented and versioned.

### Complete pixel read/write

- `PSET(x,y,index)` writes an individual pixel into the selected target screen.
- `PGET(x,y)` **reads the actual framebuffer pixel index**, including after drawing, sprite placement or image blits; results must return through the protocol to the HP-41.
- `VWRITE` and `VREAD` efficiently write/read runs of pixels; add rectangle/block forms such as `RWRITE`/`RREAD` where justified.
- Define coordinate bounds, clipping, invalid reads, orientation, indexing order, transfer chunk size, status/error reporting and readback consistency.
- Pixel reads must be genuine peripheral operations, not shortcuts that inspect only a host-side test fixture.
- A palette-read command should resolve an index to the stored RGB value.

### Drawing functions

- Scaled Cartesian coordinates and/or raw pixel coordinates.
- Plot, move, draw, line, rectangle/box, circle, polygon, fill, clipping, frame/grid/axes and text labels.
- Primitive commands operate on the selected target screen and preserve 8-bit indexed semantics.
- Reuse graphics code from the Emu71 project where feasible, with isolated input conversion for HP-41 formats.

### Sprites and image 'spraying'

- Ability to **load an image asset and spray/blit it at positions on any screen**, repeatedly, without transferring the entire source image every time.
- Import common Mac-host formats such as PNG and BMP (JPEG optional); convert to indexed colour with documented quantisation/dithering. For later hardware, assess stored preconverted assets on flash/SD rather than expecting an HP-41 to transmit photographs.
- Maintain a separate **image/sprite asset store** so a source bitmap is stored once and can be placed on multiple framebuffers.
- Support transparent palette index/mask for sprites, clipping at screen edges, position and eventually simple transformations if affordable.
- `PGET` must read the **resulting framebuffer index** after a sprite is drawn, not merely look up the original asset.
- Define asset IDs/names, load/list/free/save operations, file naming, missing-file errors and memory limits.
- Potential API concepts: `GIMAGE`, `GSPRITE`, `GFREEIMG` (exact naming/HP-41 call convention to be designed).

### Memory accounting

At 480×320 there are **153,600 pixels** per framebuffer; at 8 bits/pixel that is **153,600 bytes = 150 KiB per screen**. Thus:

| Stored screens | Pixel memory only |
| ---: | ---: |
| 16 | 2,400 KiB = 2.34375 MiB |
| 32 | 4,800 KiB = 4.6875 MiB |
| 64 | 9,600 KiB = 9.375 MiB |

Add **768 bytes per screen** for a 256×RGB888 palette, plus allocator metadata, asset memory, working buffers and possibly double buffering. The Mac can support all these configurations comfortably. A later embedded build may require external PSRAM/SD storage. Make screen count and memory budget configurable for realistic simulations rather than fixing hardware requirements prematurely.

**Important:** independent palettes imply that copying raw indices between screens does not necessarily preserve apparent colours. Define both policies explicitly and test them.

## 8. A transport-independent peripheral architecture

Target conceptual architecture:

```text
                 Native macOS HP-41 emulator
           +----------------------------------+
           | Nut CPU + HP-41 firmware          |
           | Normal program + custom XROM     |
           | Emulated HP-IL module            |
           +----------------+-----------------+
                            |
                    simulated HP-IL loop
                            |
           +----------------v-----------------+
           | Virtual graphics HP-IL peripheral|
           | HP-IL frame handling + status    |
           | Graphics command decoder         |
           +----------------+-----------------+
                            |
           +----------------v-----------------+
           | Portable renderer / asset store  |
           | 480x320; 8-bit; >=16 screens     |
           | Pixel R/W; palettes; sprites     |
           +----------------+-----------------+
                            |
                  macOS display window

     Future replacement for virtual peripheral:
     real HP-IL electrical interface -> MCU -> LCD/SD/PSRAM
     (same XROM and application-level command protocol)
```

A transport-independent API is preferable to exposing the HP-71 mailbox's 16-nibble BCD and Saturn-specific addressing as the shared interface. Create adapters as needed. Make protocol versioning, command IDs, maximum payloads, little/big-endian numeric formats, text encoding, packet checks, capabilities and result formats explicit.

A useful design is for the peripheral to advertise capabilities such as resolution, screen count, colour depth, asset support and maximum transfer size. The XROM can then fail gracefully when commands are unsupported.

## 9. Simulation and verification strategy

Maintain separate testing levels:

1. **Calculator correctness:** firmware, keyboard, arithmetic, modes, RPN stack and program execution.
2. **ROM execution:** known external XROM runs unmodified, followed by custom XROM image loading and execution.
3. **HP-IL correctness:** transactions, status, loop failures, timing constraints and replayable captures.
4. **Protocol correctness:** encode/decode tests, malformed messages, range checks and capability negotiation.
5. **Graphics correctness:** deterministic pixel comparison and image-output golden tests.
6. **End-to-end fidelity:** an HP-41 program executes custom XROM functions, simulated HP-IL carries the commands, and the graphics renderer produces expected output.

Suggested end-to-end regression program sequence:

- Open 480×320 graphics, clear screen 0 and draw axes plus a scaled sine curve.
- Draw a full-colour indexed test pattern; use `PGET` to confirm chosen pixels and `PSET` to change them.
- Create at least 16 screens, draw unique content on each, cycle/rotate the visible screen and verify each screen retains its contents and palette.
- Load one sprite/image asset, spray it to several locations/screens and read back pixels inside and outside the sprite, including transparent regions.
- Write a block of pixel indices and read back an identical block; test boundary clipping and failure responses.
- Save the screens and assets, quit, restart, reload and verify matching pixels and palettes.
- Repeat tests with constrained memory allocations and artificial HP-IL delays, to reveal future hardware limitations.

For visual comparison, save rendered PNGs (or a lossless indexed image output) and compare expected pixel arrays; report exact mismatches, not only screenshot similarity. Keep peripheral simulator tests runnable even when the calculator GUI is not open.

## 10. Suggested source tree

```text
hp41-mac/
  README.md
  PROJECT_BRIEF.md
  LICENSES.md
  docs/
    architecture.md
    emulator-selection.md
    hp-il-protocol.md
    graphics-protocol.md
    xrom-api.md
    test-plan.md
    hardware-notes.md
  src/
    app/                 # native macOS/SDL2 application
    emulator/            # upstream core integration or separate submodule
    hpil/                # simulated transport/module and tests
    graphics/            # portable renderer, palettes, screen store
    peripheral/          # simulated HP-IL graphics device
  xrom/
    src/                 # HP-41 ROM assembler sources
    build/               # ignored generated ROM images
  assets/
    skins/               # original/licensed skin files
    test-images/         # freely redistributable graphics fixtures
  tools/
  tests/
  .gitignore
```

This is a proposed eventual layout, not a reason to create empty modules in Phase 1. Respect upstream licence requirements and use submodules, vendoring with attribution, or documented upstream checkout as appropriate. Do not copy copyrighted ROMs into git.

## 11. Delivery process for Claude / coding agent

**Start with research, source inspection and a real build.** The initial coding-agent task should be:

> Create a new `hp41-mac` project folder on this Apple Silicon Mac, without editing `emu71-mac`. Inspect the two candidate HP-41 emulator engines, document their licensing/build/ROM/XROM/HP-IL capabilities and choose a foundation with evidence. Obtain/build it with the available toolchain. Implement only Phase 1 until an HP-41 calculator actually runs and passes the explicit Phase 1 acceptance tests. Produce an arm64 executable/app, reproducible build instructions and an honest test report. Do not begin graphics, XROM design or hardware work until Phase 1 passes.

Suggested sequence of commits:

- Initial project brief, emulator evaluation, licensing and reproducible build.
- Authentic calculator core and working keyboard/display.
- Persistent state and known-XROM loading tests.
- HP-IL loop and virtual test peripheral.
- Shared graphics command protocol and software screen implementation.
- First genuine graphics XROM.
- Layer stack, readback, image/sprite and stress tests.

At every milestone provide the actual commands executed, test output and outstanding limitations. Do not claim a tested feature merely because a source file or UI element exists.

## 12. Key risks and questions to resolve

- **Source suitability:** the most recently packaged emulator is not necessarily the easiest to extend. Validate Nonpareil and Emu41gg by building and testing, especially XROM and HP-IL support.
- **Licensing/firmware:** the emulator source and HP firmware may be under different licences; do not assume legal redistribution rights.
- **HP-IL module:** a real HP-41 requires a compatible HP-IL module/interface. Simulated implementation must execute actual module logic or be shown to provide hardware-faithful behaviour.
- **XROM practicality:** callable name lengths, available ROM pages, ROM IDs, assembler/linker support and numeric/string conventions must be confirmed against HP-41 specifications.
- **HP-IL throughput:** large bitmap transfers or pixel-by-pixel drawing could be slow. Prefer peripheral-side primitives, block reads/writes and preloaded sprite assets. Benchmark using simulated realistic latencies.
- **Layer count:** 16 screens is a hard minimum, 32/64 a software target; later hardware memory is unresolved.
- **Palette handling:** choose precise copy-index versus colour-converting semantics for cross-screen blits.
- **Readback semantics:** `PGET` returns the target framebuffer's indexed pixel after operations; displayed and drawing-target screens may differ.
- **Images and persistence:** define asset identifiers, palette matching, alpha/index transparency, storage format and memory quotas.
- **UI priority:** a native skin matters, but should not displace a working calculator build.
- **Future MCU selection:** intentionally deferred until the software stack works, memory use and HP-IL throughput are measured.

## 13. Current decisions versus deferred decisions

**Agreed:** Native Mac calculator first; authentic HP-41 execution; loadable custom XROM; software-only proof before hardware; simulated HP-IL transport; 480×320 graphics; 8-bit indexed colour; minimum 16 complete screens with ability to rotate/switch; pixel-level and block-level read/write; independent target/visible screens; retained graphics primitives; reusable sprite/image assets; portable peripheral architecture.

**Preferred but to validate:** HP-41CX-first configuration; SDL2 native UI; per-screen palettes; expandable 32/64-screen stack; Nonpareil as the initial engine to evaluate; source extraction/refactoring from Emu71 where legal and practical.

**Not decided:** Exact HP-41 emulator engine; actual installed toolchain; precise XROM function names and argument conventions; HP-IL framing and protocol byte layout; embedded MCU/PSRAM/SD hardware; palette conversion policy for all operations; sprite scaling/transparency rules; physical display controller; whether later simultaneous multi-layer compositing is worthwhile.

## 14. Definition of success

The project succeeds in stages. **Stage 1 success** is a demonstrably working, stateful, native arm64 HP-41 calculator which executes a standard external XROM. **Software prototype success** is a normal HP-41 program invoking a real custom XROM that communicates via simulated HP-IL to an independently implemented 480×320 colour peripheral, with at least 16 screens, readable/writable pixels, graphics primitives and reusable images. **Hardware readiness** requires that the exact same application-level protocol and, ideally, the same XROM binary can be used against a physical HP-IL-connected graphics peripheral without special Mac-only function interception.

The immediate instruction to the coding agent is simple: **get the calculator working, verify it, and do not skip ahead.**
