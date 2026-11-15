/* hp41-mac: Mac host image loader for hp41-gfx. ctx is the asset folder. */
#ifndef GFX_ASSETS_HOST_H
#define GFX_ASSETS_HOST_H
#include <stdint.h>
#include <stddef.h>
const uint8_t *gfx_host_load(void *ctx, const char *name, uint16_t *w, uint16_t *h);
/* saved screens in DIR/NAME.h41g; ctx is the folder */
int gfx_host_write(void *ctx, const char *name, const uint8_t *const *part, const size_t *size, int nparts);
long gfx_host_read(void *ctx, const char *name, size_t offset, uint8_t *buf, size_t len);
#endif
