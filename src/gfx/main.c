/* hp41-mac: Mac program for the graphics peripheral (gfx.c + ildev.c).
 *
 * Standalone, no calculator:
 *   hp41-gfx --script FILE [--png OUT.png] [--screens N]
 *     Each line is a protocol line. Lines starting with '#' are comments.
 *     Assertions:  !reply TEXT    the last query replied TEXT
 *                  !err N         the last line ended with error N (0 = none)
 *                  !pixel S X Y V screen S pixel X Y holds index V
 *                  !hash S HEX    FNV-1a 32 of screen S's indices is HEX
 *                  !print S       print screen S's hash (to make a golden)
 * On the virtual HP-IL loop:
 *   hp41-gfx [--out 60001] [--in 60000] [--log FILE] [--png OUT.png] [--screens N] [--did NAME]
 *     The device listens on OUT, replies to the calculator's IN port.
 *     --log writes every frame and every received line.
 * --png writes the displayed screen as an indexed-colour PNG on exit.
 * --hash 1 prints the displayed screen's FNV-1a 32 hash on exit.
 * --window 1 [--scale N] shows the displayed screen live in a window. On the
 *   loop it keeps accepting the calculator after a disconnect; quit by
 *   closing the window. With --script it shows the result until closed.
 * GPL v2 or later.
 */
#include "gfx.h"
#include "ildev.h"
#include "window.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

static gfx G;
static FILE *g_log;
static pthread_mutex_t G_mx = PTHREAD_MUTEX_INITIALIZER;   /* guards G against the window thread */
static volatile int g_done;

static uint32_t fnv1a(const uint8_t *p, size_t n)
  {
  uint32_t h = 2166136261u;
  while (n--) { h ^= *p++; h *= 16777619u; }
  return h;
  }

/* ---- indexed PNG ---- */
static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t n)
  {
  uint8_t b[4]; put32(b, n); fwrite(b, 1, 4, f);
  fwrite(type, 1, 4, f); if (n) fwrite(data, 1, n, f);
  uint32_t c = (uint32_t)crc32(0, (const Bytef *)type, 4); c = (uint32_t)crc32(c, data, n);
  put32(b, c); fwrite(b, 1, 4, f);
  }
static int write_png(const char *path, uint16_t screen)
  {
  FILE *f = fopen(path, "wb");
  if (!f) { perror(path); return -1; }
  static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
  fwrite(sig, 1, 8, f);
  uint8_t ih[13]; put32(ih, G.w); put32(ih + 4, G.h); ih[8] = 8; ih[9] = 3; ih[10] = ih[11] = ih[12] = 0;
  chunk(f, "IHDR", ih, 13);
  chunk(f, "PLTE", gfx_palette(&G, screen), 768);
  size_t raw_n = (size_t)G.h * (G.w + 1);
  uint8_t *raw = malloc(raw_n);
  const uint8_t *px = gfx_screen(&G, screen);
  for (uint16_t y = 0; y < G.h; y++) { raw[(size_t)y * (G.w + 1)] = 0; memcpy(raw + (size_t)y * (G.w + 1) + 1, px + (size_t)y * G.w, G.w); }
  uLongf zn = compressBound(raw_n); uint8_t *z = malloc(zn);
  compress2(z, &zn, raw, raw_n, 9);
  chunk(f, "IDAT", z, (uint32_t)zn);
  chunk(f, "IEND", NULL, 0);
  free(raw); free(z); fclose(f);
  return 0;
  }

/* ---- standalone script ---- */
static int run_script(const char *path)
  {
  FILE *f = fopen(path, "r");
  if (!f) { perror(path); return 2; }
  char line[512]; int lineno = 0, pass = 0, fail = 0, last_err = 0;
  while (fgets(line, sizeof line, f))
    {
    lineno++;
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
    if (!n || line[0] == '#') continue;
    if (line[0] != '!') { G.reply_len = 0; G.reply[0] = 0; last_err = gfx_line(&G, line, n); continue; }
    int ok = 0; char what[160];
    unsigned s, x, y, v; unsigned long hx;
    if (!strncmp(line, "!reply ", 7)) { ok = !strcmp(G.reply, line + 7); snprintf(what, sizeof what, "reply [%s]", G.reply); }
    else if (sscanf(line, "!err %u", &v) == 1) { ok = (int)v == last_err; snprintf(what, sizeof what, "error %d", last_err); }
    else if (sscanf(line, "!pixel %u %u %u %u", &s, &x, &y, &v) == 4) { int got = gfx_get(&G, (uint16_t)s, (int32_t)x, (int32_t)y); ok = got == (int)v; snprintf(what, sizeof what, "pixel %d", got); }
    else if (sscanf(line, "!hash %u %lx", &s, &hx) == 2) { uint32_t h = fnv1a(gfx_screen(&G, (uint16_t)s), (size_t)G.w * G.h); ok = h == (uint32_t)hx; snprintf(what, sizeof what, "hash %08x", h); }
    else if (sscanf(line, "!print %u", &s) == 1) { printf("line %d: screen %u hash %08x\n", lineno, s, fnv1a(gfx_screen(&G, (uint16_t)s), (size_t)G.w * G.h)); continue; }
    else { printf("line %d: unknown assertion %s\n", lineno, line); fail++; continue; }
    if (ok) pass++; else { fail++; printf("FAIL line %d: %s, got %s\n", lineno, line, what); }
    }
  fclose(f);
  printf("%s: %d assertions passed, %d failed, %u lines, %u commands\n", path, pass, fail, G.lines, G.commands);
  return fail ? 1 : 0;
  }

/* ---- HP-IL ---- */
static double t0;
static double now_ms(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6; }

static void on_message(void *ctx, ildev *d, const uint8_t *msg, uint16_t len)
  {
  (void)ctx;
  pthread_mutex_lock(&G_mx);
  G.reply_len = 0; G.reply[0] = 0;
  int e = gfx_line(&G, (const char *)msg, len);
  pthread_mutex_unlock(&G_mx);
  d->status = G.err;                               /* last error in the HP-IL status byte */
  if (G.reply_len)
    {
    uint8_t r[GFX_REPLY + 2]; memcpy(r, G.reply, G.reply_len); r[G.reply_len] = '\r'; r[G.reply_len + 1] = '\n';
    ildev_set_reply(d, r, (uint16_t)(G.reply_len + 2));
    }
  else ildev_set_reply(d, NULL, 0);
  if (g_log)
    {
    int n = len; while (n && (msg[n - 1] == '\n' || msg[n - 1] == '\r')) n--;
    fprintf(g_log, "%10.3f line [%.*s] -> %s%s%s\n", now_ms() - t0, n, (const char *)msg, e ? gfx_error_text(e) : "ok",
            G.reply_len ? ", reply " : "", G.reply_len ? G.reply : "");
    }
  }

static int listen_on(int port)
  {
  int s = socket(AF_INET, SOCK_STREAM, 0), one = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  struct sockaddr_in a; memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_port = htons(port); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(s, (struct sockaddr *)&a, sizeof a) || listen(s, 1)) { perror("listen"); exit(1); }
  return s;
  }
static int connect_to(int port)
  {
  for (int tries = 0; tries < 100; tries++)
    {
    int s = socket(AF_INET, SOCK_STREAM, 0), one = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
    struct sockaddr_in a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons(port); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(s, (struct sockaddr *)&a, sizeof a) == 0) return s;
    close(s); usleep(20000);
    }
  return -1;
  }

static int run_loop(int out, int in, const char *did, int persist)
  {
  int srv = listen_on(out);
  printf("hp41-gfx %s %ux%u, %u screens, listening on %d, replies to %d\n", did, G.w, G.h, G.nscreens, out, in); fflush(stdout);
  t0 = now_ms();
  uint32_t frames = 0;
  do
    {
    ildev d; ildev_init(&d, 0x66, did);            /* fresh HP-IL state per connection; the screens stay */
    d.on_message = on_message;
    int c = accept(srv, NULL, NULL), back = -1;
    if (c < 0) break;
    for (;;)
      {
      unsigned char b[2]; size_t have = 0;
      while (have < 2) { ssize_t r = recv(c, b + have, 2 - have, 0); if (r <= 0) goto closed; have += (size_t)r; }
      uint16_t fin = (uint16_t)((b[0] << 8) | b[1]);
      uint16_t fout = ildev_process(&d, fin);
      if (g_log)
        {
        char x[32], y[32];
        fprintf(g_log, "%10.3f %03X %-8s -> %03X %s\n", now_ms() - t0, fin, ildev_frame_name(fin, x, 32), fout, ildev_frame_name(fout, y, 32));
        }
      if (back < 0 && (back = connect_to(in)) < 0) { fprintf(stderr, "cannot reach calculator on %d\n", in); break; }
      unsigned char o[2] = { (unsigned char)(fout >> 8), (unsigned char)fout };
      if (send(back, o, 2, 0) != 2) break;
      }
  closed:
    frames += d.frames;
    if (back >= 0) close(back);
    close(c);
    if (persist) { printf("calculator disconnected; waiting for the next connection\n"); fflush(stdout); }
    }
  while (persist && !g_done);
  close(srv);
  printf("hp41-gfx: %u frames, %u lines, %u commands, last error %u, shown screen %u\n", frames, G.lines, G.commands, G.err, G.shown);
  return 0;
  }

static int g_out, g_in; static const char *g_did;
static void *loop_thread(void *arg) { (void)arg; run_loop(g_out, g_in, g_did, 1); return NULL; }

int main(int argc, char **argv)
  {
  const char *script = NULL, *png = NULL, *logpath = NULL, *did = "HP41GFX";
  int out = 60001, in = 60000, screens = 16, hash = 0, window = 0, scale = 2;
  for (int i = 1; i < argc; i += 2)
    {
    const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;
    if (!v) { fprintf(stderr, "missing value for %s\n", a); return 2; }
    if (!strcmp(a, "--script")) script = v;
    else if (!strcmp(a, "--png")) png = v;
    else if (!strcmp(a, "--log")) logpath = v;
    else if (!strcmp(a, "--did")) did = v;
    else if (!strcmp(a, "--out")) out = atoi(v);
    else if (!strcmp(a, "--in")) in = atoi(v);
    else if (!strcmp(a, "--screens")) screens = atoi(v);
    else if (!strcmp(a, "--hash")) hash = atoi(v);
    else if (!strcmp(a, "--window")) window = atoi(v);
    else if (!strcmp(a, "--scale")) scale = atoi(v);
    else { fprintf(stderr, "unknown option %s\n", a); return 2; }
    }
  size_t need = gfx_mem_size(GFX_W_DEFAULT, GFX_H_DEFAULT, (uint16_t)screens);
  void *mem = malloc(need);                        /* the Mac host owns the memory; the core never allocates */
  if (gfx_init(&G, GFX_W_DEFAULT, GFX_H_DEFAULT, (uint16_t)screens, mem, need)) { fprintf(stderr, "gfx_init failed\n"); return 1; }
  if (logpath && !(g_log = fopen(logpath, "w"))) { perror(logpath); return 1; }
  int rc = 0;
  if (script)
    {
    rc = run_script(script);
    if (window) gfx_window_run(&G, &G_mx, &g_done, scale, "HP41GFX");
    }
  else if (window)
    {
    /* the window owns the main thread (macOS); HP-IL runs beside it and accepts
       the calculator again after it disconnects, so the app can be restarted */
    g_out = out; g_in = in; g_did = did;
    pthread_t th; pthread_create(&th, NULL, loop_thread, NULL); pthread_detach(th);
    gfx_window_run(&G, &G_mx, &g_done, scale, "HP41GFX");
    g_done = 1;
    }
  else rc = run_loop(out, in, did, 0);
  if (hash) printf("screen %u hash %08x\n", G.shown, fnv1a(gfx_screen(&G, G.shown), (size_t)G.w * G.h));
  if (png && write_png(png, G.shown) == 0) printf("wrote %s (screen %u)\n", png, G.shown);
  if (g_log) fclose(g_log);
  free(mem);
  return rc;
  }
