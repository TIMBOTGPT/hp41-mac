/* hp41-mac: portable HP-IL device core.
 *
 * Plain C99. No dynamic allocation, no operating system calls, no I/O. The
 * caller owns the struct and feeds it one frame at a time; ildev_process
 * returns the frame to pass on round the loop. Intended to move unchanged to
 * a microcontroller. Fault injection lives in the caller, not here.
 *
 * The addressing and talker/listener state machine follows pyILPER's
 * pildevbase.py (J-F Garnier, C. Giesselink, J. Siebold), itself derived from
 * ILPER and Christoph Giesselink's IldevBase.cpp. GPL v2 or later.
 *
 * Frames are 11 bits: C2 C1 C0 and an 8-bit data byte.
 *   0x000 DAB  0x200 END  0x400 CMD  0x500 RDY  0x600 IDY   (0x100 = SRQ)
 *
 * As listener it collects bytes into a message, ended by an END frame or LF.
 * A complete message goes to on_message if set; the application answers
 * with ildev_set_reply. Without on_message the device echoes: the message
 * becomes the reply, so OUTA followed by INA on an HP-41 returns what was
 * sent. As talker (SDA) the device sends the reply as DAB frames, then EOT.
 */
#ifndef ILDEV_H
#define ILDEV_H
#include <stdint.h>

#define ILDEV_BUF    256     /* bytes per message buffer */
#define ILDEV_DIDLEN 16

#define IL_DAB 0x000
#define IL_END 0x200
#define IL_CMD 0x400
#define IL_RDY 0x500
#define IL_IDY 0x600
#define IL_EOT 0x540         /* RDY 40, end of transmission */
#define IL_ETE 0x541         /* RDY 41, end of transmission with error */

typedef struct ildev
  {
  /* identity */
  uint8_t aid;               /* accessory ID, returned to SAI */
  char did[ILDEV_DIDLEN];    /* device ID, returned to SDI */
  uint8_t status;            /* status byte, returned to SST */

  /* addressing (pyILPER names) */
  uint8_t defaddr;           /* address after AAU */
  uint8_t addr;              /* primary address, bit 7 = assigned */
  uint8_t addr2nd;           /* secondary address, bit 7 = assigned */
  uint8_t ilstate;           /* 0x80 listener, 0x40 talker, 0x20/0x10 2nd addr mode,
                                bits 1..0 active talker: 1 SST/SDI/SAI/NRD, 2 SDA */
  uint16_t talker_frame;     /* last frame sent as talker */
  uint8_t ptsdi, ptssi;      /* output pointers for device ID and status */

  /* data */
  uint8_t in[ILDEV_BUF];     /* message being received */
  uint16_t in_len;
  uint8_t last[ILDEV_BUF];   /* reply sent on SDA (the echo, or set by the application) */
  uint16_t last_len;
  uint16_t out_pos;          /* talker position in last[] */

  /* application hook: a complete message arrived (LF or END included) */
  void (*on_message)(void *ctx, struct ildev *d, const uint8_t *msg, uint16_t len);
  void *ctx;

  /* counters */
  uint32_t frames, data_in, data_out, cmds, messages, overflow, talker_errors;
  } ildev;

void ildev_init(ildev *d, uint8_t aid, const char *did);
/* one frame from the previous device in, the frame for the next device out */
uint16_t ildev_process(ildev *d, uint16_t frame);
/* set the bytes the device sends when next made talker (SDA) */
void ildev_set_reply(ildev *d, const uint8_t *data, uint16_t len);
/* short mnemonic for logs, e.g. "LAD 1", "SDA", "DAB 41", "AAD 1" */
const char *ildev_frame_name(uint16_t frame, char *buf, int n);

#endif
