/* hp41-mac: Mac host image loader for hp41-gfx (IL name id). Looks for
 * NAME.png, then NAME.bmp, in the asset folder and decodes it to RGBA with
 * stb_image (public domain / MIT). The core only sees the gfx_load_fn hook.
 * GPL v2 or later. */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "../app/stb_image.h"
#include "assets.h"
#include <stdio.h>
#include <string.h>

static unsigned char *last;

const uint8_t *gfx_host_load(void *ctx, const char *name, uint16_t *w, uint16_t *h)
  {
  const char *dir = (const char *)ctx;
  static const char *ext[] = { ".png", ".bmp" };
  if (last) { stbi_image_free(last); last = NULL; }
  for (int i = 0; i < 2; i++)
    {
    char path[1024]; int x, y, n;
    snprintf(path, sizeof path, "%s/%s%s", dir, name, ext[i]);
    unsigned char *p = stbi_load(path, &x, &y, &n, 4);
    if (!p) continue;
    if (x > 4096 || y > 4096) { stbi_image_free(p); return NULL; }
    last = p; *w = (uint16_t)x; *h = (uint16_t)y;
    return p;
    }
  return NULL;
  }

/* ---- saved screens: DIR/NAME.h41g ---- */
#include <stdlib.h>
#include <sys/stat.h>

int gfx_host_write(void *ctx, const char *name, const uint8_t *const *part, const size_t *size, int nparts)
  {
  const char *dir = (const char *)ctx;
  char path[1024], tmp[1100];
  mkdir(dir, 0755);
  snprintf(path, sizeof path, "%s/%s.h41g", dir, name);
  snprintf(tmp, sizeof tmp, "%s.tmp", path);
  FILE *f = fopen(tmp, "wb");
  if (!f) return -1;
  int ok = 1;
  for (int i = 0; i < nparts; i++) if (fwrite(part[i], 1, size[i], f) != size[i]) ok = 0;
  if (fclose(f) != 0) ok = 0;
  if (!ok || rename(tmp, path) != 0) { remove(tmp); return -1; }   /* write whole or not at all */
  return 0;
  }

long gfx_host_read(void *ctx, const char *name, size_t offset, uint8_t *buf, size_t len)
  {
  const char *dir = (const char *)ctx;
  char path[1024];
  snprintf(path, sizeof path, "%s/%s.h41g", dir, name);
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  long n = 0;
  if (fseek(f, (long)offset, SEEK_SET) == 0) n = (long)fread(buf, 1, len, f);
  fclose(f);
  return n;
  }
