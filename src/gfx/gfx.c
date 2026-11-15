/* hp41-mac: portable graphics peripheral core. See gfx.h. GPL v2 or later. */
#include "gfx.h"
#include <string.h>
#include <stdio.h>

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

void gfx_reset(gfx *g)
  {
  memset(g->pix, 0, (size_t)g->nscreens * g->w * g->h);
  for (uint16_t s = 0; s < g->nscreens; s++) default_palette(g->pal + (size_t)s * 768);
  g->target = g->shown = 0;
  g->pen = 1; g->px = g->py = 0;
  g->scaled = 0;
  g->xmin = 0; g->xmax = (float)(g->w - 1); g->ymin = 0; g->ymax = (float)(g->h - 1);
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

static uint8_t *scr(gfx *g, uint16_t s) { return g->pix + (size_t)s * g->w * g->h; }
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

/* Bresenham, every point clipped. Lines far outside the screen are rare
 * here, so no segment clipping yet. */
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

const char *gfx_error_text(int e)
  {
  static const char *t[] = { "OK", "UNKNOWN CMD", "ARG COUNT", "BAD NUMBER", "RANGE", "OUTSIDE", "TOO LONG", "BAD SCALE" };
  return (e >= 0 && e <= 7) ? t[e] : "?";
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

/* one command, already split off from the line */
static int command(gfx *g, const char *c, size_t n)
  {
  size_t i = 0;
  while (i < n && c[i] == ' ') i++;
  char m[4] = {0, 0, 0, 0}; int ml = 0;
  while (i < n && c[i] != ' ' && ml < 3) m[ml++] = c[i++];
  if (ml == 0) return GFX_OK;                       /* empty command, e.g. trailing ; */
  if (ml > 2 || (i < n && c[i] != ' ')) return GFX_E_UNKNOWN;
  float a[5]; int na = 0;
  while (i < n)
    {
    while (i < n && c[i] == ' ') i++;
    if (i >= n) break;
    size_t s0 = i;
    while (i < n && c[i] != ' ') i++;
    if (na == 5) return GFX_E_ARGS;
    if (parse_number(c + s0, i - s0, &a[na++])) return GFX_E_NUMBER;
    }
  int k;
  #define NEED(lo, hi) do { if (na < (lo) || na > (hi)) return GFX_E_ARGS; } while (0)
  #define IS(s) (!strcmp(m, s))
  if (!(IS("GI") || IS("C") || IS("K") || IS("T") || IS("S") || IS("M") || IS("D") || IS("P") ||
        IS("W") || IS("R") || IS("SC") || IS("SX") || IS("SY") || IS("SP") || IS("?") || IS("?E")))
    return GFX_E_UNKNOWN;
  g->commands++;
  if (IS("GI")) { NEED(0, 0); gfx_reset(g); return GFX_OK; }
  if (IS("C")) { NEED(0, 1); k = 0; if (na && as_index(a[0], 255, &k)) return GFX_E_RANGE; gfx_clear(g, (uint8_t)k); return GFX_OK; }
  if (IS("K")) { NEED(1, 1); if (as_index(a[0], 255, &k)) return GFX_E_RANGE; g->pen = (uint8_t)k; return GFX_OK; }
  if (IS("T")) { NEED(1, 1); if (as_index(a[0], g->nscreens - 1, &k)) return GFX_E_RANGE; g->target = (uint16_t)k; return GFX_OK; }
  if (IS("S")) { NEED(1, 1); if (as_index(a[0], g->nscreens - 1, &k)) return GFX_E_RANGE; g->shown = (uint16_t)k; return GFX_OK; }
  if (IS("M") || IS("D") || IS("P"))
    {
    NEED(2, 2);
    int32_t x, y; to_pixel(g, a[0], a[1], &x, &y);
    if (m[0] == 'D') gfx_drawline(g, g->px, g->py, x, y, g->pen);
    else if (m[0] == 'P') gfx_plot(g, x, y, g->pen);
    g->px = x; g->py = y;
    return GFX_OK;
    }
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
  #undef IS
  }

int gfx_line(gfx *g, const char *line, size_t len)
  {
  g->lines++;
  while (len && (line[len - 1] == '\r' || line[len - 1] == '\n')) len--;
  if (len > GFX_LINE_MAX) { g->err = GFX_E_LONG; return g->err; }
  size_t s = 0;
  for (size_t i = 0; i <= len; i++)
    if (i == len || line[i] == ':' || line[i] == ';')    /* ':' is typable on the HP-41 */
      {
      int e = command(g, line + s, i - s);
      if (e) { g->err = (uint8_t)e; return e; }
      s = i + 1;
      }
  return 0;
  }
