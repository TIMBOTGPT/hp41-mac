/* hp41-mac: byte-stream framing for the graphics peripheral (portable C99).
 *
 * Bytes from the transport (HP-IL data frames) go in one at a time. Text
 * lines end with LF (CR ignored) or with a byte that came in an END frame,
 * and run through gfx_line. A line that announces a binary block (BW, BU)
 * switches to counting: exactly the announced number of bytes is collected
 * in a fixed buffer, LF included as data, then applied whole, then one LF
 * must follow. BR produces a binary reply. Fixed memory, no allocation.
 * GPL v2 or later. */
#ifndef GFX_SESSION_H
#define GFX_SESSION_H
#include "gfx.h"

typedef struct gfx_session
  {
  gfx *g;
  char line[GFX_LINE_MAX + 1];
  uint16_t len;
  uint8_t too_long;
  uint8_t state;             /* 0 text, 1 block data, 2 LF after a block, 3 skip to LF */
  uint32_t need, got;
  uint8_t blk[GFX_BLOCK + 2];
  uint8_t out[GFX_BLOCK + 2 > GFX_REPLY + 2 ? GFX_BLOCK + 2 : GFX_REPLY + 2];
  uint32_t outlen;           /* reply bytes for the talker */
  uint32_t blocks, block_bytes;
  char last[GFX_LINE_MAX + 1];  /* the last complete text line, for logs */
  uint16_t lastlen;
  int last_err;                 /* its result */
  int block_err;                /* result of the last block (0, CRC, SYNC, ...) */
  } gfx_session;

void gfx_session_init(gfx_session *s, gfx *g);
/* one byte in; end = it came in an END frame. Returns 1 when the reply
 * changed (s->out, s->outlen; 0 bytes = nothing to send), else 0. */
int gfx_session_byte(gfx_session *s, uint8_t b, int end);

#endif
