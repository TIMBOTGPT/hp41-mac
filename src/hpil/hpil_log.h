// hp41-mac: optional observer for virtual HP-IL frames.
// The transport calls it for each frame: dir '>' = calculator to loop,
// '<' = loop to calculator. It runs on the CPU thread for '>' and on the
// TCP server thread for '<', so an implementation must be thread safe.
#pragma once
extern void (*g_hpil_frame_log)(char dir, WORD frame);
