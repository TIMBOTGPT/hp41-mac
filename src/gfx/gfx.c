/* hp41-mac: portable graphics peripheral core. See gfx.h. GPL v2 or later.
 * Box, circle and the 5x7 font follow emu71-mac's src/video.c (same owner,
 * GPL v2). The font is the widely circulated 5x7 ASCII table; its original
 * source is not recorded there. */
#include "gfx.h"
#include <string.h>
#include <stdio.h>

static const unsigned char font5x7[95][5] = {
{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x56,0x20,0x50},{0x00,0x05,0x03,0x00,0x00},
{0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x2A,0x1C,0x7F,0x1C,0x2A},{0x08,0x08,0x3E,0x08,0x08},
{0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
{0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},
{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
{0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
{0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
{0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
{0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x7F,0x10,0x28,0x44,0x00},
{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},
{0x7C,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
{0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},
{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},
{0x00,0x00,0x7F,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},{0x08,0x04,0x08,0x10,0x08}
};

size_t gfx_mem_size(uint16_t w, uint16_t h, uint16_t n)
  {
  return (size_t)n * ((size_t)w * h + 256 * 3);
  }

static void default_palette(uint8_t *p)
  {
  static const uint8_t base[16][3] = {
    {0,0,0},{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},{0,255,255},{255,0,255},
    {64,64,64},{192,192,192},{128,0,0},{0,128,0},{0,0,128},{128,128,0},{0,128,128},{128,0,128} };
  memcpy(p, base, sizeof base);
  static const uint8_t lv[6] = {0, 51, 102, 153, 204, 255};
  int i = 16;
  for (int r = 0; r < 6; r++) for (int gr = 0; gr < 6; gr++) for (int b = 0; b < 6; b++, i++)
    { p[i*3] = lv[r]; p[i*3+1] = lv[gr]; p[i*3+2] = lv[b]; }
  for (int k = 0; k < 24; k++, i++) { uint8_t v = (uint8_t)(8 + k * 10); p[i*3] = p[i*3+1] = p[i*3+2] = v; }
  }

/* nearest palette entry by squared RGB distance; ties go to the lower index */
static uint8_t nearest(const uint8_t *pal, uint8_t r, uint8_t gg, uint8_t b)
  {
  int best = 0; long bd = 1L << 30;
  for (int i = 0; i < 256; i++)
    {
    long dr = pal[i*3] - r, dg = pal[i*3+1] - gg, db = pal[i*3+2] - b, d = dr*dr + dg*dg + db*db;
    if (d < bd) { bd = d; best = i; if (!d) break; }
    }
  return (uint8_t)best;
  }

void gfx_reset(gfx *g)
  {
  memset(g->pix, 0, (size_t)g->nscreens * g->w * g->h);
  for (uint16_t s = 0; s < g->nscreens; s++) default_palette(g->pal + (size_t)s * 768);
  g->target = g->shown = 0;
  g->pen = 1; g->px = g->py = 0;
  g->scaled = 0;
  g->xmin = 0; g->xmax = (float)(g->w - 1); g->ymin = 0; g->ymax = (float)(g->h - 1);
  g->textscale = 1; g->transparent = -1; g->npoly = 0;
  g->err = 0; g->reply_len = 0; g->reply[0] = 0;
  }

int gfx_init(gfx *g, uint16_t w, uint16_t h, uint16_t n, void *mem, size_t memsize)
  {
  memset(g, 0, sizeof *g);
  if (!mem || memsize < gfx_mem_size(w, h, n) || n == 0) return -1;
  g->w = w; g->h = h; g->nscreens = n;
  g->pix = (uint8_t *)mem;
  g->pal = g->pix + (size_t)n * w * h;
  gfx_reset(g);
  return 0;
  }

void gfx_set_store(gfx *g, gfx_write_fn write, gfx_read_fn read, void *ctx)
  {
  g->store_write = write; g->store_read = read; g->store_ctx = ctx;
  }

void gfx_set_assets(gfx *g, void *pool, size_t size, gfx_load_fn load, void *ctx)
  {
  g->pool = (uint8_t *)pool; g->poolsize = pool ? size : 0; g->pooltop = 0;
  g->load = load; g->load_ctx = ctx;
  memset(g->assets, 0, sizeof g->assets);
  }

static uint8_t *scr(gfx *g, uint16_t s) { return g->pix + (size_t)s * g->w * g->h; }
static uint8_t *palp(gfx *g, uint16_t s) { return g->pal + (size_t)s * 768; }
const uint8_t *gfx_screen(const gfx *g, uint16_t s) { return g->pix + (size_t)s * g->w * g->h; }
const uint8_t *gfx_palette(const gfx *g, uint16_t s) { return g->pal + (size_t)s * 768; }

void gfx_clear(gfx *g, uint8_t index) { memset(scr(g, g->target), index, (size_t)g->w * g->h); }

void gfx_plot(gfx *g, int32_t x, int32_t y, uint8_t index)
  {
  if (x < 0 || y < 0 || x >= g->w || y >= g->h) return;
  scr(g, g->target)[(size_t)y * g->w + x] = index;
  }

int gfx_get(const gfx *g, uint16_t s, int32_t x, int32_t y)
  {
  if (s >= g->nscreens || x < 0 || y < 0 || x >= g->w || y >= g->h) return -1;
  return gfx_screen(g, s)[(size_t)y * g->w + x];
  }

/* Bresenham, every point clipped */
void gfx_drawline(gfx *g, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t index)
  {
  int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
  int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
  int32_t e = dx + dy;
  for (;;)
    {
    gfx_plot(g, x0, y0, index);
    if (x0 == x1 && y0 == y1) break;
    int32_t e2 = 2 * e;
    if (e2 >= dy) { e += dy; x0 += sx; }
    if (e2 <= dx) { e += dx; y0 += sy; }
    }
  }

static void hline(gfx *g, int32_t x0, int32_t x1, int32_t y, uint8_t c)
  {
  if (y < 0 || y >= g->h) return;
  if (x0 > x1) { int32_t t = x0; x0 = x1; x1 = t; }
  if (x0 < 0) x0 = 0;
  if (x1 >= g->w) x1 = g->w - 1;
  if (x0 > x1) return;
  memset(scr(g, g->target) + (size_t)y * g->w + x0, c, (size_t)(x1 - x0 + 1));
  }

void gfx_box(gfx *g, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int fill, uint8_t c)
  {
  if (x0 > x1) { int32_t t = x0; x0 = x1; x1 = t; }
  if (y0 > y1) { int32_t t = y0; y0 = y1; y1 = t; }
  if (fill) { for (int32_t y = y0 < 0 ? 0 : y0; y <= y1 && y < g->h; y++) hline(g, x0, x1, y, c); return; }
  hline(g, x0, x1, y0, c); hline(g, x0, x1, y1, c);
  gfx_drawline(g, x0, y0, x0, y1, c); gfx_drawline(g, x1, y0, x1, y1, c);
  }

/* midpoint circle */
void gfx_circle(gfx *g, int32_t cx, int32_t cy, int32_t r, int fill, uint8_t c)
  {
  if (r < 0 || r > 20000) return;
  int32_t x = r, y = 0, err = 1 - r;
  while (x >= y)
    {
    if (fill) { hline(g, cx - x, cx + x, cy + y, c); hline(g, cx - x, cx + x, cy - y, c);
                hline(g, cx - y, cx + y, cy + x, c); hline(g, cx - y, cx + y, cy - x, c); }
    else { gfx_plot(g, cx + x, cy + y, c); gfx_plot(g, cx - x, cy + y, c); gfx_plot(g, cx + x, cy - y, c); gfx_plot(g, cx - x, cy - y, c);
           gfx_plot(g, cx + y, cy + x, c); gfx_plot(g, cx - y, cy + x, c); gfx_plot(g, cx + y, cy - x, c); gfx_plot(g, cx - y, cy - x, c); }
    y++;
    if (err < 0) err += 2 * y + 1; else { x--; err += 2 * (y - x) + 1; }
    }
  }

/* 5x7 glyphs on a 6-pixel pitch, top left at x y, each font pixel s x s */
void gfx_text(gfx *g, int32_t x, int32_t y, const char *str, size_t n, uint8_t c)
  {
  int s = g->textscale;
  for (size_t i = 0; i < n; i++, x += 6 * s)
    {
    int ch = (unsigned char)str[i];
    if (ch < 32 || ch > 126) ch = '?';
    for (int col = 0; col < 5; col++)
      for (int row = 0; row < 7; row++)
        if (font5x7[ch - 32][col] & (1 << row))
          for (int a = 0; a < s; a++) for (int b = 0; b < s; b++) gfx_plot(g, x + col * s + a, y + row * s + b, c);
    }
  }

/* integer division rounding half away from zero */
static int32_t div_round(int64_t num, int64_t den)
  {
  if (den < 0) { num = -num; den = -den; }
  return (int32_t)(num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den));
  }

/* polygon: scanline fill (even-odd), then the outline; after emu71-mac's polypx */
void gfx_polygon(gfx *g, const int32_t *xs, const int32_t *ys, int n, int fill, uint8_t c)
  {
  if (n < 2) return;
  if (fill && n >= 3)
    {
    int32_t ymin = ys[0], ymax = ys[0];
    for (int i = 1; i < n; i++) { if (ys[i] < ymin) ymin = ys[i]; if (ys[i] > ymax) ymax = ys[i]; }
    if (ymin < 0) ymin = 0;
    if (ymax >= g->h) ymax = g->h - 1;
    for (int32_t y = ymin; y <= ymax; y++)
      {
      int32_t cx[GFX_POLY]; int k = 0;
      for (int i = 0; i < n; i++)
        {
        int32_t x0 = xs[i], y0 = ys[i], x1 = xs[(i + 1) % n], y1 = ys[(i + 1) % n];
        if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y))
          {
          cx[k++] = x0 + div_round((int64_t)(y - y0) * (x1 - x0), (int64_t)(y1 - y0));
          }
        }
      for (int i = 1; i < k; i++) { int32_t t = cx[i]; int j = i - 1; while (j >= 0 && cx[j] > t) { cx[j + 1] = cx[j]; j--; } cx[j + 1] = t; }
      for (int i = 0; i + 1 < k; i += 2) hline(g, cx[i], cx[i + 1], y, c);
      }
    }
  for (int i = 0; i < n; i++) gfx_drawline(g, xs[i], ys[i], xs[(i + 1) % n], ys[(i + 1) % n], c);
  }

const char *gfx_error_text(int e)
  {
  static const char *t[] = { "OK", "UNKNOWN CMD", "ARG COUNT", "BAD NUMBER", "RANGE", "OUTSIDE", "TOO LONG",
                             "BAD SCALE", "NO FILE", "NO MEMORY", "NO ASSET", "BAD FILE" };
  return (e >= 0 && e <= 11) ? t[e] : "?";
  }

/* ---- screens and blits ---- */

static void copy_rect(gfx *g, uint16_t src, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t dx, int32_t dy, int convert)
  {
  uint8_t map[256];
  for (int i = 0; i < 256; i++) map[i] = (uint8_t)i;
  if (convert)
    {
    const uint8_t *sp = palp(g, src), *tp = palp(g, g->target);
    for (int i = 0; i < 256; i++) map[i] = nearest(tp, sp[i*3], sp[i*3+1], sp[i*3+2]);
    }
  const uint8_t *s = scr(g, src); uint8_t *d = scr(g, g->target);
  /* walk in the order that is safe when source and target overlap */
  int up = !(src == g->target && dy > sy), left = !(src == g->target && dy == sy && dx > sx);
  for (int32_t jj = 0; jj < h; jj++)
    {
    int32_t j = up ? jj : h - 1 - jj;
    int32_t ys = sy + j, yd = dy + j;
    if (ys < 0 || ys >= g->h || yd < 0 || yd >= g->h) continue;
    for (int32_t ii = 0; ii < w; ii++)
      {
      int32_t i = left ? ii : w - 1 - ii;
      int32_t xs = sx + i, xd = dx + i;
      if (xs < 0 || xs >= g->w || xd < 0 || xd >= g->w) continue;
      uint8_t v = s[(size_t)ys * g->w + xs];
      if (g->transparent >= 0 && v == g->transparent) continue;
      d[(size_t)yd * g->w + xd] = map[v];
      }
    }
  }

/* ---- image assets ---- */

static int asset_load(gfx *g, int id, const char *name)
  {
  if (!g->load) return GFX_E_FILE;
  uint16_t w = 0, h = 0;
  const uint8_t *rgba = g->load(g->load_ctx, name, &w, &h);
  if (!rgba || !w || !h) return GFX_E_FILE;
  gfx_asset *a = &g->assets[id];
  if (a->used) { if (a->off + (size_t)a->w * a->h + ((size_t)a->w * a->h + 7) / 8 == g->pooltop) g->pooltop = a->off; a->used = 0; }
  size_t n = (size_t)w * h, need = n + (n + 7) / 8;
  if (g->pooltop + need > g->poolsize) return GFX_E_MEMORY;
  uint8_t defpal[768]; default_palette(defpal);
  uint8_t *ix = g->pool + g->pooltop, *mask = ix + n;
  memset(mask, 0, (n + 7) / 8);
  uint32_t last = 0xFFFFFFFF; uint8_t lastix = 0;
  for (size_t i = 0; i < n; i++)
    {
    const uint8_t *p = rgba + 4 * i;
    uint32_t key = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
    if (key != last) { lastix = nearest(defpal, p[0], p[1], p[2]); last = key; }
    ix[i] = lastix;
    if (p[3] >= 128) mask[i / 8] |= (uint8_t)(1 << (i % 8));
    }
  a->w = w; a->h = h; a->off = (uint32_t)g->pooltop; a->used = 1;
  g->pooltop += need;
  return GFX_OK;
  }

static void asset_place(gfx *g, const gfx_asset *a, int32_t x0, int32_t y0)
  {
  const uint8_t *ix = g->pool + a->off, *mask = ix + (size_t)a->w * a->h;
  for (int32_t y = 0; y < a->h; y++)
    for (int32_t x = 0; x < a->w; x++)
      {
      size_t i = (size_t)y * a->w + x;
      if (!(mask[i / 8] & (1 << (i % 8)))) continue;
      if (g->transparent >= 0 && ix[i] == g->transparent) continue;
      gfx_plot(g, x0 + x, y0 + y, ix[i]);
      }
  }

/* ---- saved screens ---- */

static int screen_save(gfx *g, const char *name)
  {
  if (!g->store_write) return GFX_E_FILE;
  uint8_t h[GFX_FILE_HEADER] = { 'H', '4', '1', 'G', GFX_FILE_VERSION, 0,
    (uint8_t)(g->w & 0xFF), (uint8_t)(g->w >> 8), (uint8_t)(g->h & 0xFF), (uint8_t)(g->h >> 8), 0, 0 };
  const uint8_t *part[3] = { h, palp(g, g->target), scr(g, g->target) };
  size_t size[3] = { sizeof h, 768, (size_t)g->w * g->h };
  return g->store_write(g->store_ctx, name, part, size, 3) ? GFX_E_FILE : GFX_OK;
  }

static int screen_load(gfx *g, const char *name)
  {
  if (!g->store_read) return GFX_E_FILE;
  uint8_t h[GFX_FILE_HEADER];
  long n = g->store_read(g->store_ctx, name, 0, h, sizeof h);
  if (n < 0) return GFX_E_FILE;
  if (n != (long)sizeof h || memcmp(h, "H41G", 4) || h[4] != GFX_FILE_VERSION || h[5] != 0 ||
      (h[6] | h[7] << 8) != g->w || (h[8] | h[9] << 8) != g->h) return GFX_E_BADFILE;
  size_t np = (size_t)g->w * g->h; uint8_t last;
  if (g->store_read(g->store_ctx, name, GFX_FILE_HEADER + 768 + np - 1, &last, 1) != 1) return GFX_E_BADFILE;  /* complete? */
  if (g->store_read(g->store_ctx, name, GFX_FILE_HEADER, palp(g, g->target), 768) != 768) return GFX_E_BADFILE;
  if (g->store_read(g->store_ctx, name, GFX_FILE_HEADER + 768, scr(g, g->target), np) != (long)np) return GFX_E_BADFILE;
  return GFX_OK;
  }

/* ---- parser ---- */

/* number: [-+]digits[.digits][E[-+]digits]; commas (ARCL digit grouping) skipped */
static int parse_number(const char *s, size_t n, float *out)
  {
  size_t i = 0; int neg = 0, digits = 0; double v = 0, scale = 1;
  if (i < n && (s[i] == '-' || s[i] == '+')) { neg = s[i] == '-'; i++; }
  for (; i < n && (s[i] == ',' || (s[i] >= '0' && s[i] <= '9')); i++)
    if (s[i] != ',') { v = v * 10 + (s[i] - '0'); digits++; }
  if (i < n && s[i] == '.')
    for (i++; i < n && s[i] >= '0' && s[i] <= '9'; i++) { scale /= 10; v += (s[i] - '0') * scale; digits++; }
  if (!digits) return -1;
  if (i < n && (s[i] == 'E' || s[i] == 'e'))
    {
    int eneg = 0, ex = 0, ed = 0; i++;
    if (i < n && (s[i] == '-' || s[i] == '+')) { eneg = s[i] == '-'; i++; }
    for (; i < n && s[i] >= '0' && s[i] <= '9'; i++) { ex = ex * 10 + (s[i] - '0'); ed++; if (ex > 99) return -1; }
    if (!ed) return -1;
    while (ex--) v = eneg ? v / 10 : v * 10;
    }
  if (i != n) return -1;
  *out = (float)(neg ? -v : v);
  return 0;
  }

static int32_t iround(float f) { return (int32_t)(f < 0 ? f - 0.5f : f + 0.5f); }

static void to_pixel(const gfx *g, float x, float y, int32_t *px, int32_t *py)
  {
  if (!g->scaled) { *px = iround(x); *py = iround(y); return; }
  *px = iround((x - g->xmin) * (float)(g->w - 1) / (g->xmax - g->xmin));
  *py = iround((g->ymax - y) * (float)(g->h - 1) / (g->ymax - g->ymin));
  }

static int32_t to_pixel_len(const gfx *g, float r)
  {
  if (!g->scaled) return iround(r);
  float k = (float)(g->w - 1) / (g->xmax - g->xmin);
  return iround(r * (k < 0 ? -k : k));
  }

static int as_index(float f, int max, int *out)
  {
  int32_t v = iround(f);
  if (f - (float)v > 0.001f || (float)v - f > 0.001f || v < 0 || v > max) return -1;
  *out = (int)v; return 0;
  }

static void reply(gfx *g, const char *s)
  {
  size_t n = strlen(s);
  if (n > GFX_REPLY - 1) n = GFX_REPLY - 1;
  memcpy(g->reply, s, n); g->reply[n] = 0; g->reply_len = (uint16_t)n;
  }

#define MAXARGS 8

/* one command, already split off from the line */
static int command(gfx *g, const char *c, size_t n)
  {
  size_t i = 0;
  while (i < n && c[i] == ' ') i++;
  char m[4] = {0, 0, 0, 0}; int ml = 0;
  while (i < n && c[i] != ' ' && ml < 3) m[ml++] = c[i++];
  if (ml == 0) return GFX_OK;                       /* empty command, e.g. trailing : */
  if (ml > 2 || (i < n && c[i] != ' ')) return GFX_E_UNKNOWN;
  #define IS(s) (!strcmp(m, s))
  static const char *known[] = { "GI","C","K","T","S","M","D","P","W","R","SC","SX","SY","SP","?","?E",
                                 "B","BF","O","OF","L","LS","KP","?P","CS","SW","SN","BL","BC","BT",
                                 "IL","IP","IF","?I","VS","VL","PV","PF","PO","AX","GR", NULL };
  int ok = 0;
  for (int q = 0; known[q]; q++) if (IS(known[q])) { ok = 1; break; }
  if (!ok) return GFX_E_UNKNOWN;
  g->commands++;
  if (IS("L"))                                      /* text: the rest of the line, as given */
    {
    if (i < n && c[i] == ' ') i++;
    gfx_text(g, g->px, g->py, c + i, n - i, g->pen);
    g->px += (int32_t)(n - i) * 6 * g->textscale;
    return GFX_OK;
    }
  char name[GFX_NAME] = "";
  if (IS("IL") || IS("VS") || IS("VL"))            /* the first argument is a name */
    {
    while (i < n && c[i] == ' ') i++;
    size_t s0 = i, nl = 0;
    while (i < n && c[i] != ' ')
      {
      char ch = c[i++];
      if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')) return GFX_E_FILE;
      if (nl < GFX_NAME - 1) name[nl++] = ch; else return GFX_E_FILE;
      }
    name[nl] = 0;
    if (i == s0) return GFX_E_ARGS;
    }
  float a[MAXARGS]; int na = 0;
  while (i < n)
    {
    while (i < n && c[i] == ' ') i++;
    if (i >= n) break;
    size_t s0 = i;
    while (i < n && c[i] != ' ') i++;
    if (na == MAXARGS) return GFX_E_ARGS;
    if (parse_number(c + s0, i - s0, &a[na++])) return GFX_E_NUMBER;
    }
  int k, k2;
  #define NEED(lo, hi) do { if (na < (lo) || na > (hi)) return GFX_E_ARGS; } while (0)
  #define SCREEN(f, out) do { if (as_index(f, g->nscreens - 1, &(out))) return GFX_E_RANGE; } while (0)
  if (IS("GI")) { NEED(0, 0); gfx_reset(g); return GFX_OK; }
  if (IS("C")) { NEED(0, 1); k = 0; if (na && as_index(a[0], 255, &k)) return GFX_E_RANGE; gfx_clear(g, (uint8_t)k); return GFX_OK; }
  if (IS("K")) { NEED(1, 1); if (as_index(a[0], 255, &k)) return GFX_E_RANGE; g->pen = (uint8_t)k; return GFX_OK; }
  if (IS("T")) { NEED(1, 1); SCREEN(a[0], k); g->target = (uint16_t)k; return GFX_OK; }
  if (IS("S")) { NEED(1, 1); SCREEN(a[0], k); g->shown = (uint16_t)k; return GFX_OK; }
  if (IS("SN")) { NEED(0, 0); g->shown = (uint16_t)((g->shown + 1) % g->nscreens); return GFX_OK; }
  if (IS("M") || IS("D") || IS("P") || IS("B") || IS("BF"))
    {
    NEED(2, 2);
    int32_t x, y; to_pixel(g, a[0], a[1], &x, &y);
    if (IS("D")) gfx_drawline(g, g->px, g->py, x, y, g->pen);
    else if (IS("P")) gfx_plot(g, x, y, g->pen);
    else if (m[0] == 'B') gfx_box(g, g->px, g->py, x, y, m[1] == 'F', g->pen);
    g->px = x; g->py = y;
    return GFX_OK;
    }
  if (IS("O") || IS("OF"))
    {
    NEED(1, 1);
    int32_t r = to_pixel_len(g, a[0]);
    if (r < 0) return GFX_E_RANGE;
    gfx_circle(g, g->px, g->py, r, m[1] == 'F', g->pen); return GFX_OK;
    }
  if (IS("LS")) { NEED(1, 1); if (as_index(a[0], 8, &k) || k < 1) return GFX_E_RANGE; g->textscale = (uint8_t)k; return GFX_OK; }
  if (IS("W") || IS("R"))
    {
    NEED(m[0] == 'W' ? 3 : 2, m[0] == 'W' ? 3 : 2);
    int x, y;
    if (as_index(a[0], g->w - 1, &x) || as_index(a[1], g->h - 1, &y)) return GFX_E_OUTSIDE;
    if (m[0] == 'W')
      {
      if (as_index(a[2], 255, &k)) return GFX_E_RANGE;
      gfx_plot(g, x, y, (uint8_t)k); return GFX_OK;
      }
    char b[8]; snprintf(b, sizeof b, "%d", gfx_get(g, g->target, x, y)); reply(g, b); return GFX_OK;
    }
  if (IS("KP"))
    {
    NEED(4, 4);
    int r, gg, b;
    if (as_index(a[0], 255, &k) || as_index(a[1], 255, &r) || as_index(a[2], 255, &gg) || as_index(a[3], 255, &b)) return GFX_E_RANGE;
    uint8_t *p = palp(g, g->target) + 3 * k; p[0] = (uint8_t)r; p[1] = (uint8_t)gg; p[2] = (uint8_t)b;
    return GFX_OK;
    }
  if (IS("?P"))
    {
    NEED(1, 1); if (as_index(a[0], 255, &k)) return GFX_E_RANGE;
    const uint8_t *p = palp(g, g->target) + 3 * k;
    char b[16]; snprintf(b, sizeof b, "%u %u %u", p[0], p[1], p[2]); reply(g, b); return GFX_OK;
    }
  if (IS("CS"))
    {
    NEED(1, 1); SCREEN(a[0], k);
    if (k != g->target)
      {
      memcpy(scr(g, g->target), scr(g, (uint16_t)k), (size_t)g->w * g->h);
      memcpy(palp(g, g->target), palp(g, (uint16_t)k), 768);
      }
    return GFX_OK;
    }
  if (IS("SW"))
    {
    NEED(2, 2); SCREEN(a[0], k); SCREEN(a[1], k2);
    if (k != k2)
      {
      uint8_t *p = scr(g, (uint16_t)k), *q = scr(g, (uint16_t)k2);
      for (size_t z = 0, zn = (size_t)g->w * g->h; z < zn; z++) { uint8_t t = p[z]; p[z] = q[z]; q[z] = t; }
      p = palp(g, (uint16_t)k); q = palp(g, (uint16_t)k2);
      for (size_t z = 0; z < 768; z++) { uint8_t t = p[z]; p[z] = q[z]; q[z] = t; }
      }
    return GFX_OK;
    }
  if (IS("BL") || IS("BC"))                         /* BL src x y w h dx dy, pixels */
    {
    NEED(7, 7); SCREEN(a[0], k);
    int32_t v[6];
    for (int q = 0; q < 6; q++) { v[q] = iround(a[q + 1]); if ((float)v[q] - a[q + 1] > 0.001f || a[q + 1] - (float)v[q] > 0.001f) return GFX_E_NUMBER; }
    if (v[2] < 0 || v[3] < 0) return GFX_E_RANGE;
    copy_rect(g, (uint16_t)k, v[0], v[1], v[2], v[3], v[4], v[5], m[1] == 'C');
    return GFX_OK;
    }
  if (IS("BT"))
    {
    NEED(0, 1);
    if (!na) { g->transparent = -1; return GFX_OK; }
    if (as_index(a[0], 255, &k)) return GFX_E_RANGE;
    g->transparent = (int16_t)k; return GFX_OK;
    }
  if (IS("VS") || IS("VL")) { NEED(0, 0); return m[1] == 'S' ? screen_save(g, name) : screen_load(g, name); }
  if (IS("PV"))
    {
    NEED(2, 2);
    if (g->npoly >= GFX_POLY) return GFX_E_RANGE;
    to_pixel(g, a[0], a[1], &g->polyx[g->npoly], &g->polyy[g->npoly]); g->npoly++;
    return GFX_OK;
    }
  if (IS("PF") || IS("PO"))
    {
    NEED(0, 0);
    if (g->npoly < 2) { g->npoly = 0; return GFX_E_ARGS; }
    gfx_polygon(g, g->polyx, g->polyy, g->npoly, m[1] == 'F', g->pen); g->npoly = 0;
    return GFX_OK;
    }
  if (IS("AX") || IS("GR"))
    {
    NEED(2, 2);
    if (a[0] < 0 || a[1] < 0) return GFX_E_RANGE;
    float xlo = g->xmin < g->xmax ? g->xmin : g->xmax, xhi = g->xmin < g->xmax ? g->xmax : g->xmin;
    float ylo = g->ymin < g->ymax ? g->ymin : g->ymax, yhi = g->ymin < g->ymax ? g->ymax : g->ymin;
    int32_t ox, oy; to_pixel(g, 0, 0, &ox, &oy);
    if (IS("AX")) { gfx_drawline(g, 0, oy, g->w - 1, oy, g->pen); gfx_drawline(g, ox, 0, ox, g->h - 1, g->pen); }
    for (int axis = 0; axis < 2; axis++)
      {
      float step = a[axis], lo = axis ? ylo : xlo, hi = axis ? yhi : xhi;
      if (step <= 0 || (hi - lo) / step > 1000) continue;
      long k0 = (long)(lo / step) - 1, k1 = (long)(hi / step) + 1;
      for (long k = k0; k <= k1; k++)
        {
        float v = (float)k * step;
        if (v < lo - step * 0.001f || v > hi + step * 0.001f) continue;
        int32_t px, py;
        if (axis == 0) { to_pixel(g, v, 0, &px, &py); if (IS("AX")) gfx_drawline(g, px, oy - 3, px, oy + 3, g->pen); else gfx_drawline(g, px, 0, px, g->h - 1, g->pen); }
        else { to_pixel(g, 0, v, &px, &py); if (IS("AX")) gfx_drawline(g, ox - 3, py, ox + 3, py, g->pen); else gfx_drawline(g, 0, py, g->w - 1, py, g->pen); }
        }
      }
    return GFX_OK;
    }
  if (IS("IL")) { NEED(1, 1); if (as_index(a[0], GFX_ASSETS - 1, &k)) return GFX_E_RANGE; return asset_load(g, k, name); }
  if (IS("IP"))
    {
    NEED(3, 3); if (as_index(a[0], GFX_ASSETS - 1, &k)) return GFX_E_RANGE;
    if (!g->assets[k].used) return GFX_E_ASSET;
    int32_t x, y; to_pixel(g, a[1], a[2], &x, &y);
    asset_place(g, &g->assets[k], x, y); return GFX_OK;
    }
  if (IS("IF") || IS("?I"))
    {
    NEED(1, 1); if (as_index(a[0], GFX_ASSETS - 1, &k)) return GFX_E_RANGE;
    gfx_asset *as = &g->assets[k];
    if (!as->used) return GFX_E_ASSET;
    if (m[0] == '?') { char b[24]; snprintf(b, sizeof b, "%u %u", as->w, as->h); reply(g, b); return GFX_OK; }
    size_t sz = (size_t)as->w * as->h, need = sz + (sz + 7) / 8;
    if (as->off + need == g->pooltop) g->pooltop = as->off;   /* only the newest frees space (v0) */
    as->used = 0; return GFX_OK;
    }
  if (IS("SC"))
    {
    NEED(4, 4);
    if (a[1] == a[0] || a[3] == a[2]) return GFX_E_SCALE;
    g->xmin = a[0]; g->xmax = a[1]; g->ymin = a[2]; g->ymax = a[3]; g->scaled = 1; return GFX_OK;
    }
  if (IS("SX") || IS("SY"))                      /* half of SC, so it fits ALPHA from user code */
    {
    NEED(2, 2);
    if (a[1] == a[0]) return GFX_E_SCALE;
    if (m[1] == 'X') { g->xmin = a[0]; g->xmax = a[1]; } else { g->ymin = a[0]; g->ymax = a[1]; }
    g->scaled = 1; return GFX_OK;
    }
  if (IS("SP")) { NEED(0, 0); g->scaled = 0; return GFX_OK; }
  if (IS("?"))
    {
    NEED(0, 0);
    char b[GFX_REPLY]; snprintf(b, sizeof b, "0 %u %u %u 256 B0", g->w, g->h, g->nscreens);   /* fits ALPHA (24) */
    reply(g, b); return GFX_OK;
    }
  /* ?E */
  NEED(0, 0);
  char b[GFX_REPLY];
  if (g->err) snprintf(b, sizeof b, "E %d %s", g->err, gfx_error_text(g->err)); else snprintf(b, sizeof b, "0");
  reply(g, b); g->err = 0; return GFX_OK;
  #undef NEED
  #undef SCREEN
  #undef IS
  }

int gfx_line(gfx *g, const char *line, size_t len)
  {
  g->lines++;
  while (len && (line[len - 1] == '\r' || line[len - 1] == '\n')) len--;
  if (len > GFX_LINE_MAX) { g->err = GFX_E_LONG; return g->err; }
  size_t s = 0;
  for (size_t i = 0; i <= len; i++)
    {
    /* L takes the rest of the line as text, separators included */
    size_t t = s; while (t < len && line[t] == ' ') t++;
    if (t < len && line[t] == 'L' && (t + 1 == len || line[t + 1] == ' ')) i = len;
    if (i == len || line[i] == ':' || line[i] == ';')    /* ':' is typable on the HP-41 */
      {
      int e = command(g, line + s, i - s);
      if (e) { g->err = (uint8_t)e; return e; }
      s = i + 1;
      }
    }
  return 0;
  }
