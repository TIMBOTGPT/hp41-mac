/* hp41-mac: portable graphics peripheral core (renderer and text command
 * layer v0, see docs/graphics-protocol.md).
 *
 * Plain C99. No dynamic allocation: the caller provides one block of memory
 * for all screens and palettes (gfx_mem_size), so screen count and memory
 * budget are configurable for a microcontroller. No I/O, no OS calls.
 * GPL v2 or later.
 */
#ifndef GFX_H
#define GFX_H
#include <stddef.h>
#include <stdint.h>

#define GFX_W_DEFAULT 480
#define GFX_H_DEFAULT 320
#define GFX_REPLY 80
#define GFX_LINE_MAX 255

enum gfx_error {
  GFX_OK = 0, GFX_E_UNKNOWN = 1, GFX_E_ARGS = 2, GFX_E_NUMBER = 3,
  GFX_E_RANGE = 4, GFX_E_OUTSIDE = 5, GFX_E_LONG = 6, GFX_E_SCALE = 7
};

typedef struct gfx
  {
  uint16_t w, h;
  uint16_t nscreens;
  uint8_t *pix;              /* nscreens * w * h indices */
  uint8_t *pal;              /* nscreens * 256 * 3 RGB */
  uint16_t target, shown;
  uint8_t pen;               /* pen colour index */
  int32_t px, py;            /* pen position, pixels */
  int scaled;                /* M D P in user units */
  float xmin, xmax, ymin, ymax;
  uint8_t err;               /* last error, cleared by ?E */
  char reply[GFX_REPLY];     /* reply to the last query */
  uint16_t reply_len;
  uint32_t commands, lines;  /* counters */
  } gfx;

size_t gfx_mem_size(uint16_t w, uint16_t h, uint16_t nscreens);
/* returns 0, or -1 if mem is too small */
int gfx_init(gfx *g, uint16_t w, uint16_t h, uint16_t nscreens, void *mem, size_t memsize);
void gfx_reset(gfx *g);

/* one line of text commands (without the LF). Returns the error number
 * (0 = all commands done). A query leaves its reply in g->reply. */
int gfx_line(gfx *g, const char *line, size_t len);

/* direct operations, also used by the parser */
void gfx_clear(gfx *g, uint8_t index);
void gfx_plot(gfx *g, int32_t x, int32_t y, uint8_t index);   /* clipped */
void gfx_drawline(gfx *g, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t index);
int gfx_get(const gfx *g, uint16_t screen, int32_t x, int32_t y);  /* -1 outside */
const uint8_t *gfx_screen(const gfx *g, uint16_t screen);
const uint8_t *gfx_palette(const gfx *g, uint16_t screen);
const char *gfx_error_text(int e);

#endif
