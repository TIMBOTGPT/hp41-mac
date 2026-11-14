/* hp41-mac: portable HP-IL device core. See ildev.h. GPL v2 or later.
 * State machine ported from pyILPER pildevbase.py (cls_pildevbase.process,
 * __do_doe__, __do_cmd__, __do_rdy__). */
#include "ildev.h"
#include <stdio.h>
#include <string.h>

void ildev_init(ildev *d, uint8_t aid, const char *did)
  {
  memset(d, 0, sizeof(*d));
  d->aid = aid;
  if (did) strncpy(d->did, did, ILDEV_DIDLEN - 1);
  }

static void clear_device(ildev *d)
  {
  d->in_len = 0;
  d->status &= (uint8_t)~0x40;               /* reset service request */
  }

/* listener: a data byte arrived */
static void indata(ildev *d, uint16_t frame)
  {
  uint8_t b = (uint8_t)frame;
  d->data_in++;
  if (d->in_len < ILDEV_BUF) d->in[d->in_len++] = b; else d->overflow++;
  if ((frame & 0x600) == IL_END || b == 0x0A)  /* END frame or LF ends a message */
    {
    memcpy(d->last, d->in, d->in_len);
    d->last_len = d->in_len;
    d->in_len = 0;
    d->messages++;
    }
  }

/* talker: next data frame, or EOT when the message is done. All bytes go
 * as DAB frames, like pyILPER's terminal. (Sending the last byte as END was
 * tried: the HP-41 passed it on as DAB, which counts as a talker error.) */
static uint16_t outdata(ildev *d)
  {
  if (d->out_pos >= d->last_len) return IL_EOT;
  d->data_out++;
  return (uint16_t)(IL_DAB | d->last[d->out_pos++]);
  }

static uint16_t do_doe(ildev *d, uint16_t frame)
  {
  int talker_error = 0;
  if ((d->ilstate & 0xC0) == 0x40)            /* addressed talker */
    {
    if (d->ilstate & 0x03)                    /* active talker */
      {
      talker_error = (frame & 0x6FF) != (d->talker_frame & 0x6FF);
      if (!talker_error && (d->ilstate & 0x02))
        {
        uint16_t srq = frame & 0x100;
        if (d->ilstate & 0x01)                /* multibyte SST or SDI */
          {
          if (d->ptssi > 0) d->ptssi--;       /* one status byte only */
          if (d->ptsdi > 0)
            {
            if (d->ptsdi == strlen(d->did)) { frame = 0; d->ptsdi = 0; }
            else frame = (uint8_t)d->did[d->ptsdi++];
            }
          if (d->ptssi == 0 && d->ptsdi == 0) frame = IL_EOT;
          }
        else
          frame = outdata(d);                 /* SDA */
        frame |= srq;
        }
      else
        frame = IL_EOT;                       /* end of SAI, NRD or talker error */
      }
    if (frame == IL_EOT)
      {
      if (talker_error) { frame = IL_ETE; d->talker_errors++; }
      d->ilstate &= (uint8_t)~0x03;
      }
    d->talker_frame = frame;
    }
  if ((d->ilstate & 0xC0) == 0x80) indata(d, frame);
  return frame;
  }

static uint16_t do_cmd(ildev *d, uint16_t frame)
  {
  uint8_t n = (uint8_t)frame, t = n >> 5;
  d->cmds++;
  if (t == 0)
    {
    if (n == 4 && (d->ilstate & 0x80)) clear_device(d);   /* SDC */
    else if (n == 20) clear_device(d);                     /* DCL */
    }
  else if (t == 1)                                         /* LAD / UNL */
    {
    n &= 31;
    if (n == 31) { if (d->ilstate & 0xA0) d->ilstate &= 0x50; }
    else if (!(d->ilstate & 0x80) && n == (d->addr & 31))
      d->ilstate = (d->addr2nd & 0x80) ? 0x20 : 0x80;
    }
  else if (t == 2)                                         /* TAD / UNT */
    {
    n &= 31;
    if (n == (d->addr & 31)) d->ilstate = (d->addr2nd & 0x80) ? 0x10 : 0x40;
    else if (d->ilstate & 0x50) d->ilstate &= 0xA0;
    }
  else if (t == 3)                                         /* SAD */
    {
    if (d->ilstate & 0x30)
      {
      n &= 31;
      d->ilstate = (n == (d->addr2nd & 31)) ? (uint8_t)(d->ilstate << 2) : 0;
      }
    }
  else if (t == 4)
    {
    n &= 31;
    if (n == 16) d->ilstate = 0;                           /* IFC */
    else if (n == 26) { d->addr = d->defaddr; d->addr2nd = 0; }   /* AAU */
    }
  return frame;
  }

static uint16_t do_rdy(ildev *d, uint16_t frame)
  {
  uint8_t n = (uint8_t)frame;
  if (n <= 127)
    {
    if (d->ilstate & 0x40)                    /* addressed talker */
      {
      if (n == 66) { d->ptsdi = d->ptssi = 0; d->ilstate = 0x41; }       /* NRD */
      else if (n == 96)                                                  /* SDA */
        {
        d->out_pos = 0;
        if (d->last_len)                      /* with nothing to send, SDA passes on unchanged (pyILPER base device) */
          {
          frame = outdata(d);
          d->ilstate = 0x42; d->talker_frame = frame;
          }
        }
      else if (n == 97)                                                  /* SST */
        {
        d->status &= (uint8_t)~0x40;
        d->ptssi = 1;
        frame = d->status;
        d->ilstate = 0x43; d->talker_frame = frame;
        }
      else if (n == 98 && d->did[0])                                     /* SDI */
        {
        frame = (uint8_t)d->did[0];
        d->ptsdi = 1;
        d->ilstate = 0x43; d->talker_frame = frame;
        }
      else if (n == 99)                                                  /* SAI */
        {
        frame = d->aid;
        d->ilstate = 0x41; d->talker_frame = frame;
        }
      }
    }
  else if (n < 0x80 + 31)                                                /* AAD */
    {
    if (!(d->addr & 0x80) && d->addr2nd == 0) { d->addr = n; frame++; }
    }
  else if (n >= 0xA0 && n < 0xA0 + 31)                                   /* AEP */
    {
    if (!(d->addr & 0x80) && (d->addr2nd & 0x80)) d->addr = n & 0x9F;
    }
  else if (n >= 0xC0 && n < 0xC0 + 31)                                   /* AES */
    {
    if (!(d->addr & 0x80)) { d->addr2nd = n & 0x9F; frame++; }
    }
  return frame;
  }

uint16_t ildev_process(ildev *d, uint16_t frame)
  {
  frame &= 0x7FF;
  d->frames++;
  if ((frame & 0x400) == 0) frame = do_doe(d, frame);
  else if ((frame & 0x700) == IL_CMD) frame = do_cmd(d, frame);
  else if ((frame & 0x700) == IL_RDY) frame = do_rdy(d, frame);
  if (d->status & 0x40)                       /* service request: set SRQ on DAB, END, IDY */
    {
    uint16_t c = frame & 0x700;
    if (c == IL_DAB || c == IL_END || c == IL_IDY) frame |= 0x100;
    }
  return frame;
  }

const char *ildev_frame_name(uint16_t f, char *buf, int n)
  {
  static const char *cmd0[32] = { "NUL","GTL","?","?","SDC","PPD","?","?","GET","?","?","?","?","?","?","ELN",
                                  "NOP","LLO","?","?","DCL","PPU","?","?","EAR","?","?","?","?","?","?","?" };
  static const char *cmd4[32] = { "?","?","?","?","?","?","?","?","?","?","?","?","?","?","?","?",
                                  "IFC","?","REN","NRE","?","?","?","?","?","?","AAU","LPD","?","?","?","?" };
  uint8_t d = (uint8_t)f;
  switch (f & 0x700)
    {
    case 0x000: case 0x100: snprintf(buf, n, "DAB %02X", d); break;
    case 0x200: case 0x300: snprintf(buf, n, "END %02X", d); break;
    case 0x600: case 0x700: snprintf(buf, n, "IDY %02X", d); break;
    case IL_CMD:
      switch (d >> 5)
        {
        case 0: snprintf(buf, n, "%s", cmd0[d & 31]); break;
        case 1: if ((d & 31) == 31) snprintf(buf, n, "UNL"); else snprintf(buf, n, "LAD %u", d & 31); break;
        case 2: if ((d & 31) == 31) snprintf(buf, n, "UNT"); else snprintf(buf, n, "TAD %u", d & 31); break;
        case 3: snprintf(buf, n, "SAD %u", d & 31); break;
        case 4: snprintf(buf, n, "%s", cmd4[d & 31]); break;
        default: snprintf(buf, n, "CMD %02X", d); break;
        }
      break;
    case IL_RDY:
      if (d == 0x00) snprintf(buf, n, "RFC");
      else if (d == 0x40) snprintf(buf, n, "EOT");
      else if (d == 0x41) snprintf(buf, n, "ETE");
      else if (d == 0x42) snprintf(buf, n, "NRD");
      else if (d == 0x60) snprintf(buf, n, "SDA");
      else if (d == 0x61) snprintf(buf, n, "SST");
      else if (d == 0x62) snprintf(buf, n, "SDI");
      else if (d == 0x63) snprintf(buf, n, "SAI");
      else if (d == 0x64) snprintf(buf, n, "TCT");
      else if (d >= 0x80 && d < 0x9F) snprintf(buf, n, "AAD %u", d & 31);
      else if (d >= 0xA0 && d < 0xBF) snprintf(buf, n, "AEP %u", d & 31);
      else if (d >= 0xC0 && d < 0xDF) snprintf(buf, n, "AES %u", d & 31);
      else snprintf(buf, n, "RDY %02X", d);
      break;
    }
  return buf;
  }
