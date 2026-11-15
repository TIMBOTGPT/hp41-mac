/* hp41-mac: portable graphics peripheral core (renderer and text command
 * layer v0, see docs/graphics-protocol.md).
 *
 * Plain C99. No dynamic allocation: the caller provides one block of memory
 * for all screens and palettes (gfx_mem_size) and, optionally, a pool for
 * image assets, so screen count and memory budget are configurable for a
 * microcontroller. No I/O, no OS calls: loading an image file goes through
 * a callback the host supplies. GPL v2 or later.
 */
#ifndef GFX_H
#define GFX_H
#include <stddef.h>
#include <stdint.h>

#define GFX_W_DEFAULT 480
#define GFX_H_DEFAULT 320
#define GFX_REPLY 80
#define GFX_LINE_MAX 255
#define GFX_ASSETS 32            /* image asset slots, ids 0..31 */
#define GFX_NAME 32

enum gfx_error {
  GFX_OK = 0, GFX_E_UNKNOWN = 1, GFX_E_ARGS = 2, GFX_E_NUMBER = 3,
  GFX_E_RANGE = 4, GFX_E_OUTSIDE = 5, GFX_E_LONG = 6, GFX_E_SCALE = 7,
  GFX_E_FILE = 8, GFX_E_MEMORY = 9, GFX_E_ASSET = 10, GFX_E_BADFILE = 11
};

#define GFX_POLY 64              /* polygon vertices */
/* saved screen file, version 1, little endian:
 *   0  "H41G"   4 version (1)   5 flags (0 = raw indices)
 *   6  width (16 bits)   8 height (16 bits)   10 reserved (0, 16 bits)
 *   12 palette: 256 x RGB   780 pixels: width*height indices, row major */
#define GFX_FILE_VERSION 1
#define GFX_FILE_HEADER 12

typedef struct gfx_asset
  {
  uint16_t w, h;
  uint32_t off;              /* offset in the pool: w*h indices, then the w*h-bit opacity mask */
  uint8_t used;
  } gfx_asset;

/* host hook: load an image by name, return RGBA (8 bits each, row major) or
 * NULL. The buffer belongs to the host and must stay valid until the next call. */
typedef const uint8_t *(*gfx_load_fn)(void *ctx, const char *name, uint16_t *w, uint16_t *h);

/* host hooks for saved screens: write a file made of nparts pieces (0 = ok),
 * and read len bytes at offset (returns the number read, -1 if no file) */
typedef int (*gfx_write_fn)(void *ctx, const char *name, const uint8_t *const *part, const size_t *size, int nparts);
typedef long (*gfx_read_fn)(void *ctx, const char *name, size_t offset, uint8_t *buf, size_t len);

typedef struct gfx
  {
  uint16_t w, h;
  uint16_t nscreens;
  uint8_t *pix;              /* nscreens * w * h indices */
  uint8_t *pal;              /* nscreens * 256 * 3 RGB */
  uint16_t target, shown;
  uint8_t pen;               /* pen colour index */
  int32_t px, py;            /* pen position, pixels */
  int scaled;                /* M D P B IP in user units */
  float xmin, xmax, ymin, ymax;
  uint8_t textscale;         /* text size multiplier, 1..8 */
  int16_t transparent;       /* index skipped by blits and image placement, -1 none */
  /* image assets */
  gfx_asset assets[GFX_ASSETS];
  uint8_t *pool; size_t poolsize, pooltop;
  gfx_load_fn load; void *load_ctx;
  /* saved screens */
  gfx_write_fn store_write; gfx_read_fn store_read; void *store_ctx;
  /* polygon being built */
  int32_t polyx[GFX_POLY], polyy[GFX_POLY];
  uint8_t npoly;
  /* replies and errors */
  uint8_t err;               /* last error, cleared by ?E */
  char reply[GFX_REPLY];     /* reply to the last query */
  uint16_t reply_len;
  uint32_t commands, lines;  /* counters */
  } gfx;

size_t gfx_mem_size(uint16_t w, uint16_t h, uint16_t nscreens);
/* returns 0, or -1 if mem is too small */
int gfx_init(gfx *g, uint16_t w, uint16_t h, uint16_t nscreens, void *mem, size_t memsize);
void gfx_set_assets(gfx *g, void *pool, size_t size, gfx_load_fn load, void *ctx);
void gfx_set_store(gfx *g, gfx_write_fn write, gfx_read_fn read, void *ctx);
void gfx_reset(gfx *g);

/* one line of text commands (without the LF). Returns the error number
 * (0 = all commands done). A query leaves its reply in g->reply. */
int gfx_line(gfx *g, const char *line, size_t len);

/* direct operations, also used by the parser */
void gfx_clear(gfx *g, uint8_t index);
void gfx_plot(gfx *g, int32_t x, int32_t y, uint8_t index);   /* clipped */
void gfx_drawline(gfx *g, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t index);
void gfx_box(gfx *g, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int fill, uint8_t index);
void gfx_circle(gfx *g, int32_t cx, int32_t cy, int32_t r, int fill, uint8_t index);
void gfx_text(gfx *g, int32_t x, int32_t y, const char *s, size_t n, uint8_t index);
void gfx_polygon(gfx *g, const int32_t *xs, const int32_t *ys, int n, int fill, uint8_t index);
int gfx_get(const gfx *g, uint16_t screen, int32_t x, int32_t y);  /* -1 outside */
const uint8_t *gfx_screen(const gfx *g, uint16_t screen);
const uint8_t *gfx_palette(const gfx *g, uint16_t screen);
const char *gfx_error_text(int e);

#endif
