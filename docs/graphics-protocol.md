# Graphics peripheral protocol, draft v0

Status: v0, agreed and implemented 15 November 2026: peripheral (`src/gfx`), user-code XROM (`docs/xrom.md`). Layer 2 (binary bulk) is not implemented.
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
- One or more commands per line, separated by `:` (typable in ALPHA on the
  HP-41 as the divide key; `;` is not on its keyboard). `;` is also
  accepted, for host tools. User code is limited to
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
| `GTARG` | `T` | `n` | drawing target screen |
| `GSHOW` | `S` | `n` | displayed screen |
| `PSET` | `W` | `x y index` | write one pixel |
| `PGET` | `R` | `x y` | query: reply is the pixel's index, e.g. `17` |
| `GSCALE` | `SC` | `xmin xmax ymin ymax` | later M D P use these user units; `SP` returns to pixels |
| `GSCALE` (from user code) | `SX`, `SY` | `min max` | half of `SC` each, for the x or y axis, so each fits ALPHA |
| `GBOX` | `B` | `x y` | box outline from the pen to the opposite corner x y; the pen moves there |
| `GFBOX` | `BF` | `x y` | filled box, likewise |
| `GCIRC` | `O` | `r` | circle outline centred on the pen (r in user x units when scaled) |
| `GFCIRC` | `OF` | `r` | filled circle |
| `GTEXT` | `L` | `text` | text at the pen in the pen colour; takes the rest of the line, separators included; the pen moves past it |
| (text size) | `LS` | `n` | text scale 1 to 8 (5x7 glyphs on a 6-pixel pitch) |
| `GPAL` | `KP` | `i r g b` | set palette entry i of the target screen |
| (palette read) | `?P` | `i` | query: reply `r g b` |
| (copy screen) | `CS` | `src` | copy screen src (pixels and palette) to the target |
| (swap screens) | `SW` | `a b` | swap screens a and b (pixels and palettes) |
| `GNEXT` | `SN` | none | show the next screen (wraps) |
| (blit) | `BL` | `src x y w h dx dy` | copy a rectangle of indices from screen src to the target at dx dy, pixels; overlap safe |
| (blit, convert) | `BC` | `src x y w h dx dy` | the same, but each colour maps to the nearest entry of the target palette |
| (transparency) | `BT` | `[i]` | index i is skipped by BL, BC and IP; `BT` alone turns it off |
| `GIMAGE` | `IL` | `name id` | load image NAME into asset slot id (0 to 31) |
| `GSPRAY` | `IP` | `id x y` | place asset id with its top left at x y on the target |
| (free image) | `IF` | `id` | free asset slot id |
| (image size) | `?I` | `id` | query: reply `w h` |
| (capabilities) | `?` | none | query: reply `version width height screens colours B<max block>`, e.g. `0 480 320 16 256 B0` (fits ALPHA) |
| (last error) | `?E` | none | query: reply `0` or `E n text` |

Coordinates: pixels, origin top left, x 0 to 479, y 0 to 319. With `SC`
the peripheral does the scaling, so the calculator sends user units (for a
sine curve, `X` and `SIN(X)` straight from the stack with `ARCL`). Drawing
clips at the screen edge. `PGET` outside the screen is an error.

Example, stock HP-41, axes and a sine curve:

    GI:C 0               clear
    SC -7 7 -1.2 1.2     user units
    K 1:M -7 0:D 7 0     x axis
    K 1:M 0 -1.2:D 0 1.2 y axis
    K 2:M -6.3 0         start of curve
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

## Decided 15 November 2026

1. Mnemonics are one or two letters. The frame log tool will decode them
   for readable captures.
2. Commands are silent on success. Errors set the HP-IL status byte and are
   read with `?E`.
3. Pixel origin is top left, x right, y down. `SC` user units are y up.
4. The XROM sets FIX and flag 29 itself before `ARCL`. It was to restore
   them afterwards; the user-code XROM does not (see `docs/xrom.md`).
5. Accessory ID: still open, until the HP-IL accessory class table has
   been checked. 66 is a placeholder.

Details fixed while implementing:

- Errors, read by `?E` as `E n text`, cleared by reading:
  1 unknown command, 2 wrong number of arguments, 3 bad number,
  4 screen or colour out of range, 5 pixel outside the screen (`R`, `W`),
  6 line too long, 7 bad scale (zero range), 8 no such image file,
  9 asset memory full, 10 no image in that slot.
  The HP-IL status byte holds the last error number (0 = none).
- An error stops the rest of that line. Earlier commands on the line stand.
- Default palette: 0 black, 1 white, 2 red, 3 green, 4 blue, 5 yellow,
  6 cyan, 7 magenta, 8 to 15 dark versions of 0 to 7 and grey, 16 to 231 a
  6x6x6 colour cube, 232 to 255 a grey ramp.
- Scaled coordinates round to the nearest pixel. Drawing outside the screen
  is clipped silently. Only `R` and `W` report a pixel outside.
- `GI` resets screens 0 to n-1 to index 0, all palettes to the default,
  target and shown to 0, pen 1 at 0 0, pixel coordinates, text scale 1, no
  transparent index. Loaded images stay.
- Images (`IL`): the host finds NAME.png or NAME.bmp (Mac: the asset
  folder, `--assets DIR`). Each pixel is quantised to the nearest entry of
  the default palette (squared RGB distance, no dithering); alpha below 128
  is transparent. Images are stored once and can be placed on any screen
  any number of times. In v0 freeing only reclaims memory when the freed
  image was the last loaded.
- Copy policies between screens with different palettes: `BL` and `CS`
  copy indices (the colours follow the target palette); `BC` keeps the
  appearance by mapping each colour to the target palette's nearest entry.
