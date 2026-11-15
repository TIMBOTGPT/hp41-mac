/* hp41-mac: SDL2 window for hp41-gfx (Mac host only). GPL v2 or later. */
#ifndef GFX_WINDOW_H
#define GFX_WINDOW_H
#include <pthread.h>
#include "gfx.h"
/* Runs on the main thread until the window is closed. Reads g under mx. */
int gfx_window_run(gfx *g, pthread_mutex_t *mx, volatile int *done, int scale, const char *title);
#endif
