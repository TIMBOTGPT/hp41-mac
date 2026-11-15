# Microcode XROM: research notes

Status 15 November 2026: research only, nothing implemented. Question: is a
microcode (MCODE) version of HP41GFX worth building, and how.

## Where the time goes now (measured)

From `tests/xrom.sh` (the sine program, stock speed, emulated time in the
frame log):

| Step | Time per call |
| --- | --- |
| `GMOVE`/`GDRAW` user code before `OUTA` (FIX 4, two `ARCL`, building ALPHA) | about 0.55 s |
| `OUTA` on the loop (addressing, 18 data bytes, `UNL`) | about 0.17 s |
| the user's own loop step with `SIN` | about 0.6 s |
| total per sine point | about 1.4 s |

So on a stock HP-41 the FOCAL code inside the XROM costs three times as much
as the HP-IL transfer. Bytes on the loop are not the bottleneck for drawing
commands. They are for bulk (`OUTXB` 2.6 B/s, see
`graphics-protocol.md`).

## What the HP-IL chip needs (from the 1LB6 docs and an `OUTA` trace)

Observed in the trace of `OUTA` (HP-IL module code driving the chip):

- `SELPF 0`, `CH= D0`: write R0 (controller active, talker).
- `SELPF 1`, `CH= 81` (CMD) or `A1` (RDY), then `HPIL=C 2`: frame class in
  R1W, data byte in R2.
- wait on `?F10` (ORAV), then check `?F9` (FRNS, frame not as sent).
- `RDPTRN 1` reads the received control bits.

From the chip documentation (OCR of the integrated circuit description):

- In the active controller the chip sends RFC by itself when a command
  returns without error, and sets ORAV only when that RFC has come back.
  A microcode sender does not source RFC.
- ORAV is the "next frame may be sent" signal for both talker and
  controller.

Not available: the HP-IL module's own listing. It may be on the NAS
(not mounted this session) or on hp41.org. Without it the module's entry
points, its RAM use and its error paths are unverified.

## Expected gain (estimates, not measured)

- Formatting two numbers in microcode and writing frames directly: tens of
  ms instead of 0.55 s. Moderate confidence.
- Sending coordinates as layer 2 binary (2 bytes each) instead of text:
  `D` drops from 17 bytes to about 6. Moderate confidence.
- Per drawing call: about 0.7 s down to roughly 0.1 to 0.2 s.
- Per sine point: about 1.4 s down to roughly 0.8 s, because the user's own
  FOCAL math stays. A plot program gets about 2x, not 5x.
- Bulk: a microcode loop that addresses once and streams should reach the
  chip and loop limit (hundreds of B/s or more on a stock machine; unknown
  until measured on hardware), against 2.6 B/s now.
- On a fast emulated or FPGA HP-41 the FOCAL overhead shrinks with CPU speed,
  so microcode matters mainly for stock hardware and for bulk.

## Options, ranked

1. **Microcode drives the chip directly (recommended, about 60%).** Keep
   `GRAPH` in user code (FINDID, SELECT, which leave the device addressed),
   add two microcode functions: one that formats and sends a command line
   from the stack, one that streams a block from data registers. Every other
   drawing function calls the first. Verifiable now in the emulator, with the
   frame pattern already traced. Against: it duplicates the module's error
   handling (retransmit, frame errors, IFC, timeouts), and a bug can hang the
   loop on real hardware where the emulator would not.
2. **Microcode calls the HP-IL module's internal routines (about 30%).** Uses
   tested code with the module's own error handling. Needs the listing and
   ties the XROM to one module ROM revision. Becomes first choice if the
   listing turns up and the module has only one revision in the field.
3. **Stay in user code and add composite commands (about 10%).** For example
   a polyline from data registers. The per-point FOCAL formatting stays, so
   the gain is small. Worth doing only as protocol commands that a microcode
   version would also use.

## What would change the ranking

- The HP-IL module listing found: option 2 moves up.
- Real-hardware access to test against: lowers the risk in option 1.
- If stock-machine speed matters less than the fast-machine end state,
  microcode can wait and the protocol work comes first.

## Before implementing

- Mount the NAS and look for the HP-IL module listing or entry point table.
- Confirm in the emulator that a microcode function can send one frame and
  see ORAV, using the traced register pattern.
- Decide how a microcode function reports a loop error (HP-41 error message
  versus the `?E` status).
