// hp41-mac: replacement for V41's application header (V41.h).
// Supplies the few application-level objects the emulation core refers to.
#ifndef __V41_H__
#define __V41_H__
#define __AFXWIN_H__
#include "HP41.h"
#include "WFile.h"
#include "MODFile.h"

struct CMainWindow
  {
  void InvalidateRect(const RECT *, BOOL = TRUE) {}
  void OpenConsole() {}
  void OpenBreakpoints() {}
  void UpdateRamView() {}
  BOOL IsIconic() { return FALSE; }
  void ShowWindow(int) {}
  void SetForegroundWindow() {}
  };

struct CApp
  {
  char szAppFullPath[_MAX_PATH];
  char szSessionName[_MAX_PATH];
  static const char szUntitledName[];
  flag fConsole, fSingle;
  int nFileCount;
  LOGPALETTE *pPalForeground;
  CMainWindow *pMainWnd, *m_pMainWnd;
  void AddToRecentFileList(LPCTSTR) {}
  };
extern CApp theApp;
inline CApp *AfxGetApp() { return &theApp; }

struct CWaveSound
  {
  void SetDuration(DWORD) {}
  int Put(word) { return 0; }
  };
extern CWaveSound WaveSound;
#endif
