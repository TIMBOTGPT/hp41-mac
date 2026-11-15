# HP41GFX XROM

User-code XROM v0 for the graphics peripheral. Source:
`xrom/src/hp41gfx.focal`. Built by `tools/build_xrom.py`, tested by
`tests/xrom.sh`. Last updated 15 November 2026.

## Toolchain

Not in this repository. Both tools are by Christophe Gottheimer, GPL v2,
from the hp41.org library download
(`~/Downloads/HP41-R3/Utilities/NUTASM-FOCALCOD`). They build unmodified
on arm64 with the Xcode command line tools:

    mkdir -p ~/Projects/hp41-upstream/toolchain && cd ~/Projects/hp41-upstream/toolchain
    tar xzf ~/Downloads/HP41-R3/Utilities/NUTASM-FOCALCOD/asmnut-0.40.1_RELEASE.tgz
    tar xzf ~/Downloads/HP41-R3/Utilities/NUTASM-FOCALCOD/focalcod-0.1.1_RELEASE.tgz
    (cd asmnut && make) ; (cd focalcod && make)

(tar reports "Failed to restore metadata" for a few files; the files are
extracted correctly.)

- `focalcod` 0.1.1 compiles FOCAL text and lists each line with its bytes.
  It resolves XROM names from its database (`ILMODULE-1H.XRM` for the HP-IL
  module, `XFUNS3-2D.XRM` for the CX Extended Functions).
- `asmnut` 0.40.1 assembles the page: FAT, the MCODE header function, the
  checksum, and writes the MOD file.
- `tools/build_xrom.py` sits between them and lays the user code out in
  ROM form (below). `focalcod` itself only writes RAM-form bytes.

Build by hand:

    python3 tools/build_xrom.py xrom/src/hp41gfx.focal xrom/build/HP41GFX.MOD \
      --xrom 8 --header "-HP41GFX 0A" --romid GF-0A --title "HP41GFX graphics v0" \
      --mcode xrom/src/hp41gfx_mc.src --listing xrom/build/hp41gfx.lst

Without `--mcode` the ROM is FOCAL only (`MCODE= tests/xrom.sh` tests that
build).

## Microcode functions

Added 15 November 2026. `xrom/src/hp41gfx_mc.src` (asmnut, Zencode
mnemonics) replaces 18 functions with microcode: GCLEAR GCOLOR GTARG GSHOW
GMOVE GDRAW GPLOT GPIXEL GBOX GFBOX GCIRC GFCIRC GNEXT GAXES GGRID GVERT
GPFILL GPOLY. A label `(NAME)` in that file for a global label `NAME` of the
FOCAL source makes the FAT entry point to the microcode, in the same FAT
position, so XROM numbers do not change. The FOCAL versions stay in the ROM
(COPY still copies them).

How they work: the entry puts the mnemonic and the argument count in B and
jumps to a shared body, which

1. saves the flags register (register 14) in N and sets FIX 4 and flag 29
   (FIX 0, flag 29 clear for colours and screens),
2. clears ALPHA and appends the mnemonic with the mainframe's `APNDNW`,
3. appends Y and X with `AFORMT`, the routine `ARCL` uses,
4. puts the user's flags back,
5. jumps to `OUTA` in the HP-IL module (7246).

The bytes sent are the FOCAL version's, byte for byte (the golden tests pass
unchanged). Differences for the user: the display format is restored, and in
PRGM mode a step shows `20 GDRAW` rather than `XROM "GDRAW"`.

Fixed addresses, checked against HP's VASM listings (mainframe and HP-IL
cassette/control ROM, revision 6/81A) and against the `CS1H` HP-IL ROM image
word for word: `APNDNW` 2D14, `AFORMT` 0628, `OUTA` 7246. Other HP-IL module
revisions are not checked.

Points that cost time:

- `NCGOREL` (and `NCXQREL`) go through the mainframe's `GOLONG`/`GOSUB`
  routines, which use C. Anything to keep across them goes in B.
- `AFORMT` leaves the Q pointer active. `FORMAT` needs P: `PT=P` before
  each call, or the second number gets digit-grouping commas in the wrong
  places.
- `AFORMT` needs two more return-stack levels, so it is called from the
  function's own level, as `ARCL` does, never from a subroutine.

Speed at stock speed, 20 `GDRAW` steps in a row (`tools/xrom_callrate.sh`):
0.56 s per call FOCAL, 0.39 s microcode, 0.17 s for an `OUTA` step alone.
The sine program of `tests/xrom.sh` goes from 1.41 to 1.24 s per point.
Where the 2,069 instructions of one microcode call go: HP-IL module 530,
mainframe byte fetch for `OUTA` 249, number formatting about 500, appending
to ALPHA 360, step and XROM dispatch about 300, our own code 58.

### GBYTES: block data from registers

`GBYTES` (XROM 8,29, microcode only; in a FOCAL-only build its stub stops
with `NEEDS MCODE`) sends the data part of a layer 2 block. The program
sends the header line itself, then the bytes:

    "BW 0 0 200 1"   OUTA         header: 200 pixels at 0,0
    1 ENTER 200      GBYTES       R01..R200 as 200 bytes, then LF

Each register holds an integer 0 to 255 (a number with a fraction sends its
integer part). All registers are checked before anything is sent:
NONEXISTENT if the range goes past SIZE, DATA ERROR for ALPHA data,
negative numbers, fractions below 1 and values over 255. So an error never
leaves the peripheral waiting for the rest of a block. The check reads the
BCD digits directly (sign 0, exponent 0 to 2, three-digit mantissa at most
255 compared as hex), the send pass converts with the mainframe's `BCDBIN`
and sends each byte with the HP-IL module's `SDATA0` after `SCHDEV` and
`LISTEN`, then ends with `UNLCHK`, as `OUTA` does.

Measured at stock speed: 72 bytes/s during the send, about 87 instructions
per byte for the whole call (own code 37, `BCDBIN` 35, HP-IL module 13);
`OUTXB` manages 2.6 bytes/s. One byte per register costs memory: a CX holds
at most 319 registers.

Pitfall found here: asmnut accepted `C=DATA` (the HP mnemonic) and assembled
nothing for it. The Zencode names are `RDATA` and `WDATA`.

## User code in ROM

Established 15 November 2026 from a forum description and checked against
HP's Math Pac and Stat Pac ROMs (local reads only, nothing committed):

- One program byte per 10-bit word. Bit 8 set on the first byte of each line.
- Before each chunk of programs (up to an END) two header words, used by
  COPY: the number of 7-byte registers the chunk needs, then `2b0` where b
  (1 to 7) is the number of bytes in the first register. Math Pac's first
  chunk is 983 bytes = 7 x 140 + 3 and is preceded by `08D 230`.
- The chunk ends with END, its third byte `20` with bit 9 set.
- FAT entries for user code have bit 9 set (`200` plus the address) and
  point at the global label.
- Compiled jumps hold plain word distances, unlike RAM. Fitted to all 293
  user-code jumps in the Math and Stat Pacs:
  - long `GTO` (`Dx`) and `XEQ` (`Ex`), 3 words: distance v =
    (low nibble of byte 1) << 8 | byte 2; byte 3 bit 7 set means toward
    higher addresses; target = first word + 1 +/- v.
  - short `GTO` (`B1` to `BF`), 2 words: v = byte 2 & 7F, bit 7 set means
    higher; target = first word + 2 +/- v.
- Uncompiled jumps do not work from ROM. The first build had them and the
  HP-41 jumped into the wrong code.

## Functions

XROM 8, ROM ID `GF0A`, 28 functions, header `-HP41GFX 0A`. Arguments follow
keying order.

XROM numbers 1 to 31 are all taken by some HP or third-party module. The
first build used 17, which is the 82184A Plotter module, the most likely
clash for a graphics user. 8 is the Stress Pac (SKWIDBC), which is rare and
unlikely to be plugged in alongside. If it is, change `--xrom` and rebuild;
programs keyed with the old number must be keyed again.

`COPY` works on the ROM. In the emulator `COPY "GRAPH"` took free memory from
219 to 127 registers (92 registers, the whole ROM's user code), and RAM copies
of `GRAPH`, `GCOLOR`, `PSET` and `PGET` ran correctly.

| Function | Stack in | Sends | Stack out |
| --- | --- | --- | --- |
| `GRAPH` | none | `FINDID HP41GFX`, `SELECT`, `GI:C 0` | X Y Z kept, T lost |
| `GCLEAR` | X: index | `C i` | unchanged |
| `GCOLOR` | X: index | `K i` | unchanged |
| `GTARG` | X: screen | `T n` | unchanged |
| `GSHOW` | X: screen | `S n` | unchanged |
| `GMOVE` | Y: x, X: y | `M x y` | unchanged |
| `GDRAW` | Y: x, X: y | `D x y` | unchanged |
| `GPLOT` | Y: x, X: y | `P x y` | unchanged |
| `GSCALE` | T: xmin, Z: xmax, Y: ymin, X: ymax | `SX`, `SY` | unchanged |
| `GPIXEL` | none | `SP` | unchanged |
| `PSET` | Z: x, Y: y, X: index | `W x y i` | unchanged |
| `PGET` | Y: x, X: y | `R x y`, `INA` | index in X, stack lifted |
| `GBOX` | Y: x, X: y | `B x y` | unchanged |
| `GFBOX` | Y: x, X: y | `BF x y` | unchanged |
| `GCIRC` | X: radius | `O r` | unchanged |
| `GFCIRC` | X: radius | `OF r` | unchanged |
| `GTEXT` | ALPHA: text (up to 22 characters) | `L text` | T lost |
| `GPAL` | T: index, Z: r, Y: g, X: b | `KP i r g b` | unchanged |
| `GNEXT` | none | `SN` | unchanged |
| `GIMAGE` | ALPHA: name, X: slot | `IL name id` | T lost |
| `GSPRAY` | Z: slot, Y: x, X: y | `IP id x y` | unchanged |
| `GSAVE` | ALPHA: name | `VS name` | T lost |
| `GLOAD` | ALPHA: name | `VL name` | T lost |
| `GAXES` | Y: x tick, X: y tick | `AX tx ty` | unchanged |
| `GGRID` | Y: dx, X: dy | `GR dx dy` | unchanged |
| `GVERT` | Y: x, X: y | `PV x y` | unchanged |
| `GPFILL` | none | `PF` | unchanged |
| `GPOLY` | none | `PO` | unchanged |
| `GBYTES` | Y: first register r, X: count n | the bytes of Rr .. Rr+n-1, then LF (microcode only) | unchanged, ALPHA kept |

The FAT follows the order of the global labels in the source, so new
functions are only ever added at the end: reordering would renumber
XROM 8,nn and break programs already keyed in. `tests/xrom.sh` checks the
whole list in order.

`GTEXT`, `GSAVE`, `GLOAD` and `GIMAGE` put the command in front of ALPHA by appending it and
rotating ALPHA right with the CX function `AROT`. All functions overwrite
ALPHA. `GRAPH` shows `NO HP41GFX` and stops if the
peripheral is not on the loop. Coordinates are sent in FIX 4, integers in
FIX 0 with flag 29 clear.

Differences from what was agreed:

- The 18 microcode functions restore the user's display format. The ten
  still in FOCAL (GRAPH GSCALE PSET PGET GTEXT GPAL GIMAGE GSPRAY GSAVE
  GLOAD) do not: they leave FIX 4 with flag 29 set (the HP-41 default).
  User code cannot save and restore the format without disturbing the
  stack or synthetic tricks.
- `GTARGET` became `GTARG`: `focalcod` rejects seven-character labels.

## Observed

`tests/xrom.sh`, 13 checks pass, including sessions for the functions added
after v0 (exact lines sent; `PGET` reading a sprayed image, and a pixel of a
triangle saved with `GSAVE` and loaded back with `GLOAD`).
The first 9 checks: build, FAT names, checksum, `PSET` then
`PGET` returns 5, and a keyed-in program of XROM calls draws axes and a sine
curve. The peripheral receives the expected 73 lines, the image is pixel
identical to the golden one, no errors. In program mode the steps display
as `XROM "GDRAW"` and so on.

At stock HP-41CX speed the drawing takes 96 s of emulated time: 7.2 s of
setup, then 1.40 s per curve segment (a hand-written FOCAL program doing
the same with `OUTA` takes 1.33 s per segment).

## Not done or not verified

- `COPY` of the XROM programs to RAM: the global label chain links are
  left as `focalcod` writes them, not computed for the ROM.
- ROM ID (`@@@@` now), XROM number conflict check, and a microcode XROM.
- A real HP-41 has not run this ROM.
