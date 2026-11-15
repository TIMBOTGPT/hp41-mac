# Microcode XROM: research notes

Status 15 November 2026. Question: is a microcode (MCODE) version of
HP41GFX worth building, and how. First result: 18 drawing functions are now
microcode (`docs/xrom.md`, "Microcode functions").

## Sources found

On the NAS (`12_Hobbies/HP DVDs and Docs`), local reads only, nothing
committed:

- `HP Stuff/Internal Documentation/82161A HPIL Cassette-Control VASM
  Listing.txt`: HP's source of the HP-IL module's control ROM (page 7,
  `-MASS ST 1H`, revision 6/81A), including `OUTA`, `SDATA`, `RDDFRM`,
  `UNL`, `LISTEN`.
- `82162A HPIL Printer VASM Listing.txt`: the module's page 6.
- `Coconut VASM GFF.txt`: the HP-41C mainframe (`AFORMT`, `FORMAT`,
  `APNDNW`).
- `HPIL Reference Specification.pdf` (1980, scanned, OCR with tesseract):
  accessory ID classes, appendix D.
- `HP41-DVD/Modules-ROMs/DISASM 4D/41CRAM/HPIL.pdf`: chip registers and a
  short entry point table.

The page 7 listing matches the `CS1H` ROM image used here word for word at
`SDATA0` 7126, `SDATA` 7128, `RDDFRM` 7110, `UNL` 70AF, `LISTEN` 70DD; the
GOSUB targets the listing leaves to the linker resolve to the listed
routines (for example `SCHDEV` calls `PILEN` at 7063).

## HP-IL module routines a microcode sender can call

- `SDATA0` 7126: enable the flag line, then `SDATA`.
- `SDATA` 7128: send C[1:0] as a data frame, wait for ORAV (timeout about
  40 s), S9 set on timeout or frame not as sent.
- `LISTEN` 70DD: address the selected device as listener.
- `SCHDEV` 72DD: enable the chip, check errors, get the device number.
- `UNLCHK` 7336: send UNL, then report any error.
- `OUTA` 7246: all of the above for ALPHA, adding CR LF unless flag 17.

From the chip documents: in the active controller the chip sends RFC by
itself when a command returns without error, and sets ORAV only when the
RFC has come back. A sender does not source RFC.

## Measured (stock speed, emulated)

`tools/xrom_callrate.sh`, 20 `GDRAW` steps in a row:

| ROM | per call |
| --- | --- |
| FOCAL only | 0.56 s |
| microcode (AFORMT into ALPHA, then OUTA) | 0.39 s |
| an `OUTA` step alone (ALPHA already set) | 0.17 s |

Sine program of `tests/xrom.sh`: 1.41 s per point FOCAL, 1.24 s microcode.

Instructions in one microcode `GDRAW` (2,069): HP-IL module 530, mainframe
byte fetch for `OUTA` 249, number formatting about 500, appending to ALPHA
360, step and XROM dispatch about 300, own code 58.

My earlier estimate (0.1 to 0.2 s per call) was wrong. It assumed the
FOCAL interpreter was most of the cost. Without it, HP's own routines
remain: appending characters to ALPHA shifts four registers per character,
and `OUTA` fetches each byte back out of ALPHA through the mainframe, about
30 instructions (5 ms) per byte. The earlier breakdown (0.55 s of FOCAL
before each `OUTA`) also counted the user's own program steps between calls.

## What would make it faster, ranked

1. **Bulk sender (recommended next, about 60%).** Address once, then
   `SDATA` per byte from data registers or an extended memory file. The
   per-byte cost drops to roughly 10 to 15 instructions (2 to 3 ms, about
   300 to 500 B/s at stock speed, estimate) against 2.6 B/s for `OUTXB`
   now. This is the largest gap left on a stock HP-41.
2. **Send without ALPHA (about 30%).** Format with `FORMAT` (display
   registers) or straight from the BCD digits, and send each byte with
   `SDATA`. Saves the ALPHA shuffling and the byte fetch, roughly 600 to
   800 of the 2,069 instructions: about 0.25 s per call instead of 0.39 s
   (estimate). Exact bytes would have to be rebuilt by hand (rounding, digit
   grouping), so the golden tests would need care.
3. **Binary drawing commands (about 10%).** Coordinates as 2-byte integers
   (layer 2), about 6 bytes per `D` instead of 18. Needs protocol additions
   and is mostly worth it on fast machines, where formatting is cheap anyway.

On a fast emulated or FPGA HP-41 all of this scales with CPU speed, so the
stock machine is the only case where these matter.

## Risks still open

- Only the `CS1H` HP-IL ROM is checked. Other revisions may move routines.
- Real hardware is untested: a bug in a sender can hang a real loop where
  the emulator recovers.
- Loop errors are reported as the HP-IL module reports them (`TRANSMIT ERR`
  and so on), because the module's own routines are used.
