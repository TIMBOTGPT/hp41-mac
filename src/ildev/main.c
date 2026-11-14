/* hp41-mac: Mac wrapper for the portable HP-IL test device (ildev.c).
 *
 * Live:   hp41-ildev [--out 60001] [--in 60000] [--log FILE] [--aid HEX] [--did NAME]
 *                    [--delay-us N] [--drop N] [--drop-from N] [--corrupt N]
 *                    [--wrongclass N] [--cut N] [--did-crlf 0|1]
 *   The device ID is sent as given; --did-crlf 1 appends CR LF.
 *   Listens on OUT for the calculator's frames, passes each through the
 *   device and sends the result to the calculator's IN port. Frame numbers
 *   for faults count from 1, in arrival order:
 *     --drop N        frame N is not passed on
 *     --drop-from N   no frame from N on is passed on (broken loop)
 *     --corrupt N     data bit 0 of frame N is inverted
 *     --wrongclass N  frame N leaves as a DAB frame whatever it was
 *     --cut N         both connections close after frame N (interrupted transfer)
 * Replay: hp41-ildev --replay FILE [--aid HEX] [--did NAME]
 *   Feeds the '>' frames of an hp41-headless --ilog capture to a fresh device
 *   and checks each result against the '<' frame that followed in the capture.
 *
 * GPL v2 or later.
 */
#include "ildev.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

static double now_ms(void)
  {
  struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
  }

static int replay(const char *path, uint8_t aid, const char *did)
  {
  FILE *f = fopen(path, "r");
  if (!f) { perror(path); return 2; }
  ildev d; ildev_init(&d, aid, did);
  char line[256]; unsigned n = 0, bad = 0, pending = 0; uint16_t expect_from = 0, got = 0;
  int have = 0;
  while (fgets(line, sizeof line, f))
    {
    if (line[0] == '#') continue;
    double t1, t2; char dir; unsigned fr;
    if (sscanf(line, "%lf %lf %c %x", &t1, &t2, &dir, &fr) != 4) continue;
    if (dir == '>')
      {
      if (have) { pending++; }               /* a frame never came back */
      got = ildev_process(&d, (uint16_t)fr); expect_from = (uint16_t)fr; have = 1; n++;
      }
    else if (dir == '<' && have)
      {
      have = 0;
      if (got != (uint16_t)fr)
        {
        char a[32], b[32], c[32];
        if (bad < 20) printf("frame %u: in %s, device gave %s, capture had %s\n", n,
                             ildev_frame_name(expect_from, a, 32), ildev_frame_name(got, b, 32), ildev_frame_name((uint16_t)fr, c, 32));
        bad++;
        }
      }
    }
  fclose(f);
  printf("replay: %u frames, %u differ, %u without a reply; device received %u messages, sent %u data bytes\n",
         n, bad, pending, d.messages, d.data_out);
  return bad ? 1 : 0;
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

int main(int argc, char **argv)
  {
  int out = 60001, in = 60000; const char *logpath = NULL, *replay_path = NULL;
  unsigned aid = 0x66; const char *did = "ILTESTD";
  int did_crlf = 0;
  long delay_us = 0, drop = 0, drop_from = 0, corrupt = 0, wrongclass = 0, cut = 0;
  for (int i = 1; i < argc; i += 2)
    {
    const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;
    if (!v) { fprintf(stderr, "missing value for %s\n", a); return 2; }
    if (!strcmp(a, "--out")) out = atoi(v);
    else if (!strcmp(a, "--in")) in = atoi(v);
    else if (!strcmp(a, "--log")) logpath = v;
    else if (!strcmp(a, "--replay")) replay_path = v;
    else if (!strcmp(a, "--aid")) aid = (unsigned)strtoul(v, NULL, 16);
    else if (!strcmp(a, "--did")) did = v;
    else if (!strcmp(a, "--did-crlf")) did_crlf = atoi(v);
    else if (!strcmp(a, "--delay-us")) delay_us = atol(v);
    else if (!strcmp(a, "--drop")) drop = atol(v);
    else if (!strcmp(a, "--drop-from")) drop_from = atol(v);
    else if (!strcmp(a, "--corrupt")) corrupt = atol(v);
    else if (!strcmp(a, "--wrongclass")) wrongclass = atol(v);
    else if (!strcmp(a, "--cut")) cut = atol(v);
    else { fprintf(stderr, "unknown option %s\n", a); return 2; }
    }
  char didbuf[ILDEV_DIDLEN];
  snprintf(didbuf, sizeof didbuf, did_crlf ? "%s\r\n" : "%s", did);
  did = didbuf;
  if (replay_path) return replay(replay_path, (uint8_t)aid, did);

  ildev d; ildev_init(&d, (uint8_t)aid, did);
  FILE *log = logpath ? fopen(logpath, "w") : NULL;
  if (log) fprintf(log, "# host_ms n in -> out  (device %s, AID %02X)\n", did, aid);
  int srv = listen_on(out);
  printf("ildev %.*s AID %02X listening on %d, replies to %d\n", (int)strcspn(did, "\r"), did, aid, out, in); fflush(stdout);
  int c = accept(srv, NULL, NULL), back = -1;
  double t0 = now_ms();
  long n = 0;
  for (;;)
    {
    unsigned char b[2]; size_t have = 0;
    while (have < 2) { ssize_t r = recv(c, b + have, 2 - have, 0); if (r <= 0) goto done; have += (size_t)r; }
    uint16_t fin = (uint16_t)((b[0] << 8) | b[1]);
    n++;
    uint16_t fout = ildev_process(&d, fin);
    const char *fault = "";
    int send_it = 1;
    if (n == drop || (drop_from && n >= drop_from)) { send_it = 0; fault = " DROPPED"; }
    if (n == corrupt) { fout ^= 0x001; fault = " CORRUPTED"; }
    if (n == wrongclass) { fout &= 0x0FF; fault = " WRONG CLASS"; }
    if (log)
      {
      char x[32], y[32];
      fprintf(log, "%10.3f %6ld %03X %-8s -> %03X %-8s%s\n", now_ms() - t0, n, fin, ildev_frame_name(fin, x, 32), fout, ildev_frame_name(fout, y, 32), fault);
      fflush(log);
      }
    if (delay_us) usleep((useconds_t)delay_us);
    if (send_it)
      {
      if (back < 0 && (back = connect_to(in)) < 0) { fprintf(stderr, "cannot reach calculator on %d\n", in); break; }
      unsigned char o[2] = { (unsigned char)(fout >> 8), (unsigned char)fout };
      if (send(back, o, 2, 0) != 2) break;
      }
    if (cut && n >= cut) { if (log) fprintf(log, "# connection cut after frame %ld\n", n); break; }
    }
done:
  if (back >= 0) close(back);
  close(c); close(srv);
  printf("ildev: %ld frames, %u messages received, %u data bytes in, %u out, last message \"%.*s\"\n",
         n, d.messages, d.data_in, d.data_out, (int)(d.last_len > 60 ? 60 : d.last_len), (const char *)d.last);
  if (log) fclose(log);
  return 0;
  }
