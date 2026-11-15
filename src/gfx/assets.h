/* hp41-mac: Mac host image loader for hp41-gfx. ctx is the asset folder. */
#ifndef GFX_ASSETS_HOST_H
#define GFX_ASSETS_HOST_H
#include <stdint.h>
const uint8_t *gfx_host_load(void *ctx, const char *name, uint16_t *w, uint16_t *h);
#endif
