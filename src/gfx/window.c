/* hp41-mac: SDL2 window for hp41-gfx. Mac host only; the portable cores
 * (gfx.c, ildev.c) know nothing about it. Shows the displayed screen at its
 * palette colours, scaled to the window, refreshed about 30 times a second.
 * GPL v2 or later. */
#include "window.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

int gfx_window_run(gfx *g, pthread_mutex_t *mx, volatile int *done, int scale, const char *title)
  {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return -1; }
  SDL_Window *win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     g->w * scale, g->h * scale, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");          /* nearest: keep pixels sharp */
  SDL_RenderSetLogicalSize(ren, g->w, g->h);
  SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, g->w, g->h);
  uint32_t *rgb = SDL_malloc((size_t)g->w * g->h * 4);
  uint32_t lastlines = 0xFFFFFFFF; uint16_t lastshown = 0xFFFF;
  int quit = 0;
  while (!quit)
    {
    SDL_Event e;
    while (SDL_PollEvent(&e))
      if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q && (e.key.keysym.mod & KMOD_GUI))) quit = 1;
    pthread_mutex_lock(mx);
    int changed = g->lines != lastlines || g->shown != lastshown;
    if (changed)
      {
      const uint8_t *px = gfx_screen(g, g->shown), *pal = gfx_palette(g, g->shown);
      for (size_t i = 0, n = (size_t)g->w * g->h; i < n; i++)
        {
        const uint8_t *c = pal + 3 * px[i];
        rgb[i] = 0xFF000000u | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | c[2];
        }
      lastlines = g->lines; lastshown = g->shown;
      char t[160];
      snprintf(t, sizeof t, "%s  -  screen %u, %u lines, %u commands%s%s", title, g->shown, g->lines, g->commands,
               g->err ? ", last error " : "", g->err ? gfx_error_text(g->err) : "");
      SDL_SetWindowTitle(win, t);
      }
    pthread_mutex_unlock(mx);
    if (changed) SDL_UpdateTexture(tex, NULL, rgb, g->w * 4);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
    SDL_Delay(30);
    if (*done && !quit) { /* calculator side finished: keep showing until closed */ }
    }
  SDL_free(rgb);
  SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
  }
