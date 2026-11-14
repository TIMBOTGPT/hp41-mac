// hp41-mac headless build: stand-ins for the parts of V41 that are user
// interface, sound or networking. The emulation core itself is upstream V41.
// SetKeyDown, SetKeyUp and IsKeyPressed are taken from V41's HP41Keyboard.cpp
// (Copyright (c) 1989-2002 Warren Furlow, GPL v2 or later).
#include "StdAfx.h"
#include "V41.h"
#include "TcpIp.h"
#include "Udp.h"

CApp theApp;
const char CApp::szUntitledName[] = "Untitled";
CWaveSound WaveSound;

// ---- keyboard ----
void HP41::SetKeyboard(int eKbd, flag, flag, flag) { eKeyboard = eKbd; }
int HP41::GetKeyboard() const { return eKeyboard; }

bool HP41::IsKeyPressed() const
  {
  bool bKeyDown = false;
  for (uint KeyCode = 0; !bKeyDown && ++KeyCode < sizeof(keytable) / sizeof(keytable[0]);)
    bKeyDown = keytable[KeyCode] != 0;
  return bKeyDown;
  }

void HP41::SetKeyDown(byte KeyCode)
  {
  if (eKeyboard == eKeyboardNone)
    return;
  if (keytable[KeyCode] == 0)
    {
    MinCLRKEY = 3;
    keytable[KeyCode] = 1;
    if (eKeystate == eKbdIdle)
      {
      if (eDeepSleep != IsSleeping() || KeyCode == 0x18)
        {
        KEYDOWN = TRUE;
        KEY_REG = KeyCode;
        eKeystate = eKbdPressed;
        }
      }
    if ((eLightSleep == IsSleeping()) || ((eDeepSleep == IsSleeping()) && (KeyCode == 0x18)))
      Wakeup();
    }
  }

void HP41::SetKeyUp(byte KeyCode)
  {
  if (eKeyboard == eKeyboardNone)
    return;
  keytable[KeyCode] = 0;
  }

// ---- virtual HP-IL transport: not connected in this build ----
TcpIp::TcpIp(HpilIf *pHpil, LPCTSTR lpszAddrOut, WORD wPortOut, WORD wPortIn)
  : m_pHpil(pHpil), m_strAddrOut(lpszAddrOut), m_wPortOut(wPortOut), m_wPortIn(wPortIn),
    m_dwConnectTimeout(0), m_bRunning(false), m_hWorkerThread(NULL), m_nNumSocks(0), m_cfd(-1), m_sClient(-1) {}
TcpIp::~TcpIp() {}
bool TcpIp::TcpCreateSvr() { return false; }
VOID TcpIp::TcpCloseSvr() {}
bool TcpIp::TcpSendFrame(WORD) { return true; }

// ---- 82143A printer output channel: discarded ----
Udp *Udp::pInstance_ = NULL;
Udp::Udp() : m_fpSendByte(NULL) {}
Udp::~Udp() {}
void Udp::deleteInstance() { delete pInstance_; pInstance_ = NULL; }
bool Udp::IsComPort(LPCTSTR) { return false; }
VOID Udp::SetParam(LPCSTR, DWORD) {}
VOID Udp::Reset() {}
bool Udp::SendByte(BYTE) { return true; }
