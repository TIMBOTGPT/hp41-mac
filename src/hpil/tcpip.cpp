// hp41-mac: POSIX port of V41's virtual HP-IL transport (Tcpip.cpp).
//
// Based on V41 R9O Tcpip.cpp, Copyright (c) 2018 Christoph Giesselink,
// GPL v2 or later. Same wire convention as V41, Emu71 and pyILPER: every
// HP-IL frame is 16 bits, sent big-endian over TCP. Outgoing frames go to a
// client connection on (address, port out), incoming frames arrive on a
// server listening on port in. Changes from upstream:
//   - BSD sockets and pthreads instead of Winsock and Win32 threads
//   - the server listens on the loopback interfaces only (IPv4 and IPv6)
//   - the server thread polls with a timeout so it can be stopped reliably
//   - an optional frame log (hpil_log.h) records both directions
#include "StdAfx.h"
#include "TcpIp.h"
#include "hpil_log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

void (*g_hpil_frame_log)(char dir, WORD frame) = NULL;

TcpIp::TcpIp(HpilIf *pHpil, LPCTSTR lpszAddrOut, WORD wPortOut, WORD wPortIn)
  : m_pHpil(pHpil), m_strAddrOut(lpszAddrOut), m_wPortOut(wPortOut), m_wPortIn(wPortIn),
    m_dwConnectTimeout(0xFFFFFFFF), m_bRunning(false), m_hWorkerThread(NULL),
    m_nNumSocks(0), m_cfd(-1), m_sClient(-1)
  {
  }

TcpIp::~TcpIp() {}

VOID TcpIp::SetNonBlockedIO(unsigned long flag) const
  {
  int fl = fcntl(m_sClient, F_GETFL, 0);
  fcntl(m_sClient, F_SETFL, flag ? (fl | O_NONBLOCK) : (fl & ~O_NONBLOCK));
  }

static void no_sigpipe(int fd)
  {
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
  }

// returns true when the frame could not be sent (no next device)
bool TcpIp::TcpSendFrame(WORD wFrame)
  {
  if (g_hpil_frame_log) g_hpil_frame_log('>', wFrame);
  unsigned char buf[2] = { (unsigned char)(wFrame >> 8), (unsigned char)wFrame };
  for (int attempt = 0; attempt < 2; attempt++)
    {
    if (m_sClient < 0)
      {
      char port[16]; snprintf(port, sizeof(port), "%u", m_wPortOut);
      struct addrinfo hints, *res = NULL;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;
      if (getaddrinfo(m_strAddrOut, port, &hints, &res) != 0) return true;
      for (struct addrinfo *ai = res; ai; ai = ai->ai_next)
        {
        m_sClient = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (m_sClient < 0) continue;
        int one = 1;
        setsockopt(m_sClient, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        no_sigpipe(m_sClient);
        bool nonblock = m_dwConnectTimeout < 0xFFFFFFFF;
        if (nonblock) SetNonBlockedIO(1);
        bool ok = connect(m_sClient, ai->ai_addr, ai->ai_addrlen) == 0;
        if (!ok && nonblock && errno == EINPROGRESS)
          {
          struct pollfd p = { m_sClient, POLLOUT, 0 };
          int ms = (int)((m_dwConnectTimeout + 999) / 1000);
          if (poll(&p, 1, ms) == 1)
            {
            int err = 0; socklen_t len = sizeof(err);
            getsockopt(m_sClient, SOL_SOCKET, SO_ERROR, &err, &len);
            ok = err == 0;
            }
          }
        if (!ok) { close(m_sClient); m_sClient = -1; continue; }
        if (nonblock) SetNonBlockedIO(0);
        break;
        }
      freeaddrinfo(res);
      if (m_sClient < 0) return true;
      }
    size_t sent = 0;
    while (sent < sizeof(buf))
      {
      ssize_t n = send(m_sClient, buf + sent, sizeof(buf) - sent, 0);
      if (n <= 0) break;
      sent += (size_t)n;
      }
    if (sent == sizeof(buf)) return false;
    close(m_sClient); m_sClient = -1;   // broken connection: reconnect once
    }
  return true;
  }

UINT __stdcall TcpIp::ThreadTcpIpServer(LPVOID pParam)
  {
  TcpIp *p = reinterpret_cast<TcpIp *>(pParam);
  while (p->m_bRunning)
    {
    struct pollfd pf[NFDS];
    for (int i = 0; i < p->m_nNumSocks; i++) { pf[i].fd = p->m_sockfds[i]; pf[i].events = POLLIN; pf[i].revents = 0; }
    if (poll(pf, p->m_nNumSocks, 100) <= 0) continue;
    for (int i = 0; i < p->m_nNumSocks && p->m_bRunning; i++)
      {
      if (!(pf[i].revents & POLLIN)) continue;
      int fd = accept(p->m_sockfds[i], NULL, NULL);
      if (fd < 0) continue;
      no_sigpipe(fd);
      p->m_cfd = fd;
      unsigned char buf[2]; size_t have = 0;
      while (p->m_bRunning)
        {
        struct pollfd c = { fd, POLLIN, 0 };
        int r = poll(&c, 1, 100);
        if (r == 0) continue;
        if (r < 0) break;
        ssize_t n = recv(fd, buf + have, sizeof(buf) - have, 0);
        if (n <= 0) break;                       // partner closed the connection
        have += (size_t)n;
        if (have < sizeof(buf)) continue;
        have = 0;
        WORD wFrame = (WORD)((buf[0] << 8) | buf[1]);
        if (g_hpil_frame_log) g_hpil_frame_log('<', wFrame);
        p->m_pHpil->RecvFrame(wFrame);
        }
      close(fd);
      p->m_cfd = -1;
      }
    }
  return 0;
  }

bool TcpIp::StartServer()
  {
  m_nNumSocks = 0;
  int fams[NFDS] = { AF_INET, AF_INET6 };
  for (int k = 0; k < NFDS; k++)
    {
    int fd = socket(fams[k], SOCK_STREAM, 0);
    if (fd < 0) continue;
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    int rc;
    if (fams[k] == AF_INET)
      {
      struct sockaddr_in a; memset(&a, 0, sizeof(a));
      a.sin_family = AF_INET; a.sin_port = htons(m_wPortIn); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      rc = bind(fd, (struct sockaddr *)&a, sizeof(a));
      }
    else
      {
      setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(one));
      struct sockaddr_in6 a; memset(&a, 0, sizeof(a));
      a.sin6_family = AF_INET6; a.sin6_port = htons(m_wPortIn); a.sin6_addr = in6addr_loopback;
      rc = bind(fd, (struct sockaddr *)&a, sizeof(a));
      }
    if (rc != 0 || listen(fd, 5) != 0) { close(fd); continue; }
    m_sockfds[m_nNumSocks++] = fd;
    }
  return m_nNumSocks == 0;   // true = failed
  }

VOID TcpIp::StopServer()
  {
  if (m_sClient >= 0) { close(m_sClient); m_sClient = -1; }
  m_bRunning = false;
  }

bool TcpIp::TcpCreateSvr()
  {
  if (StartServer()) return true;
  m_bRunning = true;
  m_hWorkerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadTcpIpServer, this, 0, NULL);
  return m_hWorkerThread == NULL;
  }

VOID TcpIp::TcpCloseSvr()
  {
  StopServer();
  if (m_hWorkerThread)
    {
    WaitForSingleObject(m_hWorkerThread, INFINITE);   // the thread polls m_bRunning every 100 ms
    CloseHandle(m_hWorkerThread);
    m_hWorkerThread = NULL;
    }
  for (int i = 0; i < m_nNumSocks; i++) close(m_sockfds[i]);
  m_nNumSocks = 0;
  }
