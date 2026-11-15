/* hp41-mac: byte-stream framing for the graphics peripheral. See session.h.
 * GPL v2 or later. */
#include "session.h"
#include <string.h>

void gfx_session_init(gfx_session *s, gfx *g)
  {
  memset(s, 0, sizeof *s);
  s->g = g;
  }

/* a complete text line: run it, then either start a block, produce a binary
 * reply, or set the text reply (CR LF terminated, for INA) */
static int line_done(gfx_session *s)
  {
  gfx *g = s->g;
  g->reply_len = 0; g->reply[0] = 0;
  int e;
  if (s->too_long) { g->lines++; g->err = GFX_E_LONG; e = GFX_E_LONG; }
  else e = gfx_line(g, s->line, s->len);
  memcpy(s->last, s->line, s->len); s->lastlen = s->len; s->last_err = e;
  s->len = 0; s->too_long = 0;
  if (!e && g->bulk_op == 'R')
    {
    s->outlen = gfx_bulk_read(g, s->out);
    return 1;
    }
  if (!e && (s->need = gfx_bulk_expect(g)) > 0)
    {
    s->state = 1; s->got = 0;
    s->outlen = 0;
    return 1;
    }
  g->bulk_op = 0;
  if (g->reply_len)
    {
    memcpy(s->out, g->reply, g->reply_len);
    s->out[g->reply_len] = '\r'; s->out[g->reply_len + 1] = '\n';
    s->outlen = g->reply_len + 2u;
    }
  else s->outlen = 0;
  return 1;
  }

int gfx_session_byte(gfx_session *s, uint8_t b, int end)
  {
  gfx *g = s->g;
  switch (s->state)
    {
    case 1:                                   /* block data, counted; LF is data here */
      s->blk[s->got++] = b;
      if (s->got < s->need) return 0;
      {
      int e = gfx_bulk_data(g, s->blk, s->need);
      if (e) g->err = (uint8_t)e;
      s->block_err = e;
      s->blocks++; s->block_bytes += s->need;
      }
      s->state = 2;
      return 0;
    case 2:                                   /* exactly one LF closes a block */
      if (b == '\r') return 0;
      if (b == '\n') { s->state = 0; return 0; }
      g->err = GFX_E_SYNC; s->block_err = GFX_E_SYNC; s->state = 3;
      return 0;
    case 3:                                   /* resynchronise at the next LF */
      if (b == '\n' || end) s->state = 0;
      return 0;
    default:                                  /* text */
      if (b == '\r') return 0;
      if (b == '\n') return line_done(s);
      if (s->len < GFX_LINE_MAX) s->line[s->len++] = (char)b; else s->too_long = 1;
      if (end) return line_done(s);
      return 0;
    }
  }
