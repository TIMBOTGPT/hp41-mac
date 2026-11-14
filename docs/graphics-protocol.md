# Graphics peripheral protocol, draft v0

Status: draft for review, 15 November 2026. Nothing here is implemented yet.
Requirements come from `docs/original-brief-2026-11-14.md` sections 6 to 8.

## Decision this draft follows

Agreed 15 November 2026: two layers.

1. A compact **text command layer** that every calculator must be able to
   use, sized for a stock HP-41 on a real loop (about 150 to 200 bytes/s
   inside a message, about 0.3 s per `OUTA` call from FOCAL).
2. A **binary bulk layer** for fast emulated or FPGA HP-41s, announced by
   the peripheral's capabilities and used for pixel blocks and assets.

A real HP-41 must work with layer 1 alone. Layer 2 is optional.

Measured basis (`docs/hpil.md`, emulator timing model): FOCAL `OUTA` loop
40 bytes/s sustained at 1x, 386 at 10x, 1.8 k at 50x, 5.5 k at 200x. The
real loop caps at about 7.1 kB/s with one device (HP Journal, 1983).

## Transport

- Ordinary HP-IL device-dependent data. The calculator makes the peripheral
  listener and sends data frames (as `OUTA` does), or makes it talker and
  reads (as `INA` does). No custom HP-IL frames.
- Device ID `HP41GFX` (seven characters, so `FINDID` works). Accessory ID to
  be chosen; 66 is a placeholder.
- Errors also set the HP-IL status byte (SST), so a calculator can poll
  without reading text.

## Layer 1: text commands

Framing:

- ASCII. A line ends with LF; CR is ignored. This is what `OUTA` sends.
- One or more commands per line, separated by `;`. User code is limited to
  24 characters per `OUTA` (the ALPHA register), so packing two short
  commands per line halves the per-call cost. The peripheral accepts lines
  of up to 255 bytes.
- A command is a mnemonic of one or two letters, then arguments separated
  by spaces.
- Numbers: optional `-`, digits, optional `.` and fraction. Commas are
  ignored, because `ARCL` inserts digit grouping (`40,320.0000`). The XROM
  sets the display format before `ARCL` so numbers arrive in this form. A
  mode command may later switch the radix for flag 28 users.
- Commands are silent when they succeed. Only queries produce a reply,
  which is one LF-terminated line read with `INA`.

First command set, mapped to the brief's first XROM subset:

| XROM function (proposal) | Command | Arguments | Effect |
| --- | --- | --- | --- |
| `GRAPH` | `GI` | none | reset: screen 0 target and shown, default palette, pen 1, pixel coordinates |
| `GCLEAR` | `C` | `[index]` | clear the target screen to index (default 0) |
| `GMOVE` | `M` | `x y` | move the pen without drawing |
| `GDRAW` | `D` | `x y` | line from the pen to x y, pen moves |
| `GPLOT` | `P` | `x y` | set one pixel at x y in the pen colour, pen moves |
| (pen colour) | `K` | `index` | pen colour for later drawing |
| `GTARGET` | `T` | `n` | drawing target screen |
| `GSHOW` | `S` | `n` | displayed screen |
| `PSET` | `W` | `x y index` | write one pixel |
| `PGET` | `R` | `x y` | query: reply is the pixel's index, e.g. `17` |
| `GSCALE` | `SC` | `xmin xmax ymin ymax` | later M D P use these user units; `SP` returns to pixels |
| (capabilities) | `?` | none | query: reply e.g. `HP41GFX 0 480 320 16 256 B255` |
| (last error) | `?E` | none | query: reply `0` or `E n text` |

Coordinates: pixels, origin top left, x 0 to 479, y 0 to 319. With `SC`
the peripheral does the scaling, so the calculator sends user units (for a
sine curve, `X` and `SIN(X)` straight from the stack with `ARCL`). Drawing
clips at the screen edge. `PGET` outside the screen is an error.

Example, stock HP-41, axes and a sine curve:

    GI;C 0               clear
    SC -7 7 -1.2 1.2     user units
    K 7;M -7 0;D 7 0     x axis
    M 0 -1.2;D 0 1.2     y axis
    K 2;M -6.3 0         start of curve
    D -6.2 0.083         one segment per D, two per line where they fit

Rough cost on a stock HP-41: a 120-segment curve is about 60 to 120
`OUTA` calls, about 25 to 50 s plus the time to compute the sines. At 50x
it is about a second.

## Layer 2: binary bulk (capability `B<n>`)

- A text command announces a binary block, then exactly that many raw
  bytes follow as data frames, length-counted, so LF inside the block is
  data. Then a two-byte CRC-16 (CCITT), then LF.
- `BW x y w h` then w*h index bytes: write a pixel rectangle on the target.
- `BR x y w h`: query; the reply is w*h index bytes and the CRC, read as
  binary by the calculator (not with `INA`).
- `B<n>` in the capability reply is the largest block the peripheral
  accepts in one command.
- Sprite and image assets (load once, place many times) use the same
  block mechanism. Details deferred to the asset design.

From user code `INA` and `OUTA` cannot carry arbitrary bytes, so layer 2
needs a microcode XROM or the Extended I/O module. That fits: layer 2 is
for fast machines.

## Open points for review

1. Mnemonics: one or two letters (as above) or short words (`DRAW`,
   `MOVE`)? Words are easier to read in captures; letters halve the bytes.
2. Silent success versus an acknowledgement per line. Silent is faster;
   acknowledgements catch lost commands. HP-IL already detects frame errors.
3. Origin top left (screen convention) or bottom left (plotter convention)
   for pixel coordinates. `SC` makes this matter less.
4. Whether the first XROM sets FIX and flag 29 itself before `ARCL`, or
   leaves the display format to the user program.
5. Accessory ID class for the peripheral.
