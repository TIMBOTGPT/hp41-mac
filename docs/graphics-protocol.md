# Graphics peripheral protocol, draft v0

Status: v0, agreed and implemented 15 November 2026: peripheral (`src/gfx`), user-code XROM (`docs/xrom.md`), layer 2 binary blocks (no microcode sender yet).
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
- Device ID `HP41GFX` (seven characters, so `FINDID` works). Accessory ID 66
  (class 6, graphic i/o; see decision 5).
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
| `GSAVE` | `VS` | `name` | save the target screen (palette and pixels) as NAME |
| `GLOAD` | `VL` | `name` | load screen NAME into the target |
| `GVERT` | `PV` | `x y` | add a polygon vertex (up to 64; kept across lines) |
| `GPFILL` | `PF` | none | fill and outline the polygon in the pen colour, then clear it |
| `GPOLY` | `PO` | none | outline the closed polygon, then clear it |
| `GAXES` | `AX` | `tx ty` | axes through the user origin, ticks every tx and ty (0 = none) |
| `GGRID` | `GR` | `dx dy` | grid lines every dx and dy user units (0 = none) |
| (capabilities) | `?` | none | query: reply `version width height screens colours B<max block>`, e.g. `0 480 320 16 256 B4096` (fits ALPHA) |
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

## Layer 2: binary bulk (capability `B4096`, implemented 15 Nov 2026)

- A text line announces a block. Exactly the announced number of bytes
  follows as data frames, length-counted, so LF inside the block is data.
  Then one LF. The block command must be the last command on its line.
- `BW x y w h [c]`: w*h index bytes follow, written to the target
  rectangle at x y (clipped), row by row.
- `BR x y w h [c]`: query; the reply is w*h index bytes (and the CRC if
  c is 1), read as bytes by the calculator. The rectangle must lie inside
  the screen.
- `BU id w h [c]`: w*h index bytes follow and become image asset id, every
  pixel opaque (`BT` gives transparency when it is placed).
- At most 4096 bytes per block (w*h). The peripheral collects a block in a
  fixed buffer and applies it whole: a block with a bad CRC changes nothing.
- c = 1: a CRC-16/CCITT-FALSE (polynomial 1021, initial FFFF) of the data
  follows it, high byte first. c = 0 or absent: no CRC. Decided while
  implementing: CRC is optional because HP-IL already checks every frame
  (the sender compares each frame returning round the loop, and the tests
  show corrupted frames ending in TRANSMIT ERR), and computing a CRC on a
  stock HP-41 is costly. It matters for other transports.
- If the byte after a block is not LF: error 14 (SYNC), and the peripheral
  skips to the next LF.
- Errors: 12 CRC mismatch, 13 block larger than 4096, 14 SYNC.
- The HP-IL device consumes its reply: a read interrupted by NRD (for
  example the Extended I/O module's `INXB`, one byte per call) continues
  where it stopped on the next read.

From user code `OUTA` and `INA` cannot carry arbitrary bytes. The Extended
I/O module's `OUTXB` and `INXB` can, one byte per call, and the tests use
them: a block written with `OUTXB` and read back with `INXB` from a real
HP-41 program (`tests/xrom.sh`). That path is slow: 387 ms per byte at
stock speed (2.6 bytes/s), 8.3 ms at 50x (120 bytes/s), because every call
addresses the loop again. The XROM's microcode `GBYTES` addresses once and
streams the block from data registers (one byte each): 72 bytes/s at stock
speed (`docs/xrom.md`). Reading a block back (`BR`) still needs `INXB`.

## Decided 15 November 2026

1. Mnemonics are one or two letters. The frame log tool will decode them
   for readable captures.
2. Commands are silent on success. Errors set the HP-IL status byte and are
   read with `?E`.
3. Pixel origin is top left, x right, y down. `SC` user units are y up.
4. The XROM sets FIX and flag 29 itself before `ARCL`, and restores them
   afterwards. Done in the 18 microcode functions; the ten still in FOCAL
   do not restore them (see `docs/xrom.md`).
5. Accessory ID 66, checked 15 November 2026 against the HP-IL reference
   specification (1980 DEC 1, appendix D): class 0110 is "graphic i/o", no
   devices defined in it then. The status byte follows the same appendix:
   D8 clear means the other bits are designer specified, D7 is reserved for
   "needs service"; our error numbers 1 to 14 fit below D7. The 1981 HP-IL
   module listing has its own device type table (plotter 50), used only by
   the module's printer search, not the accessory ID. HP's 7470A plotter
   answers 60 in pyILPER. Whether a later HP device used 66 is not checked.

Details fixed while implementing:

- Errors, read by `?E` as `E n text`, cleared by reading:
  1 unknown command, 2 wrong number of arguments, 3 bad number,
  4 screen or colour out of range, 5 pixel outside the screen (`R`, `W`),
  6 line too long, 7 bad scale (zero range), 8 no such image file,
  9 asset memory full, 10 no image in that slot, 11 saved screen damaged or
  of another version or size, 12 CRC mismatch, 13 block too large, 14 SYNC
  (no LF after a block).
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
- Saved screens (`VS`, `VL`), file version 1, little endian: `H41G`,
  version byte 1, flags byte 0 (raw indices), width and height (16 bits
  each), 2 reserved bytes, then the 256 x RGB palette, then width x height
  indices row by row: 154,380 bytes at 480 x 320. The host stores them
  (Mac: `NAME.h41g` in the screens folder, `--store DIR`), written whole or
  not at all. `VL` checks magic, version, flags, size and length before it
  changes the target screen. Images are not saved: they reload by name.
- Copy policies between screens with different palettes: `BL` and `CS`
  copy indices (the colours follow the target palette); `BC` keeps the
  appearance by mapping each colour to the target palette's nearest entry.
