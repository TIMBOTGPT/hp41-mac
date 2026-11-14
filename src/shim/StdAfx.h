// hp41-mac: replacement for V41's StdAfx.h.
// Provides just enough of Win32/MFC for the V41 emulation core to compile
// unmodified on macOS/arm64. No UI. Licensed GPL v2 or later, like V41.
#ifndef HP41MAC_STDAFX_H
#define HP41MAC_STDAFX_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cctype>
#include <ctime>
#include <cassert>
#include <string>
#include <vector>
#include <utility>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>

// ---- V41's own basic types (dword/ulong forced to 32 bit) ----
typedef unsigned char flag;
typedef unsigned char byte;
typedef unsigned short word;
typedef uint32_t dword;
typedef unsigned short ushort;
typedef uint32_t ulong;
typedef unsigned int uint;
typedef uint64_t uint64;

// ---- Win32 scalar types ----
typedef int BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef unsigned int UINT;
typedef int INT;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint64_t ULONGLONG;
typedef uint64_t UINT64;
typedef int64_t LONGLONG;
typedef int64_t INT64;
typedef char TCHAR;
typedef char CHAR;
typedef const char *LPCTSTR;
typedef const char *LPCSTR;
typedef char *LPTSTR;
typedef char *LPSTR;
typedef void *LPVOID;
typedef void *HWND;
typedef void *HINSTANCE;
typedef uintptr_t UINT_PTR;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t ULONG_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef DWORD COLORREF;
typedef UINT MMRESULT;
typedef int SOCKET;
typedef DWORD *LPDWORD;
#define VOID void
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define CALLBACK
#define WINAPI
#define __stdcall
#define __cdecl
#define __inline inline
#define _MAX_PATH 1024
#define MAX_PATH 1024
#define _MAX_DRIVE 8
#define _MAX_DIR 1024
#define _MAX_FNAME 256
#define _MAX_EXT 256
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)((c)>>8))
#define GetBValue(c) ((BYTE)((c)>>16))
#define LOBYTE(w) ((BYTE)((w)&0xff))
#define HIBYTE(w) ((BYTE)(((w)>>8)&0xff))
#define LOWORD(l) ((WORD)((l)&0xffff))
#define HIWORD(l) ((WORD)(((l)>>16)&0xffff))
#define MAKEWORD(a,b) ((WORD)(((BYTE)(a))|((WORD)((BYTE)(b)))<<8))
#define ASSERT(x) ((void)0)
#define VERIFY(x) ((void)(x))
#define TRACE(...) ((void)0)
#define UNREFERENCED_PARAMETER(x) ((void)(x))
#define _T(x) x
#define INFINITE 0xFFFFFFFFu
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258
#define MB_OK 0
#define MB_ICONEXCLAMATION 0
#define MB_ICONSTOP 0
#define MB_ICONINFORMATION 0
#define MB_ICONQUESTION 0
#define MB_YESNO 4
#define MB_YESNOCANCEL 3
#define MB_OKCANCEL 1
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#define SW_RESTORE 9
#define MB_SETFOREGROUND 0
#define TIME_CALLBACK_FUNCTION 0
#define strcmpi strcasecmp
#define _strcmpi strcasecmp
inline DWORD GetVersion() { return 0; }
inline int _inp(unsigned short) { return 0; }
inline int _outp(unsigned short, int v) { return v; }
inline BOOL MessageBeep(UINT) { return TRUE; }

struct RECT { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };
struct CPoint { LONG x, y; };
union LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; }; LONGLONG QuadPart; };
struct TIMECAPS { UINT wPeriodMin, wPeriodMax; };
struct PALETTEENTRY { BYTE peRed, peGreen, peBlue, peFlags; };
struct LOGPALETTE { WORD palVersion, palNumEntries; PALETTEENTRY palPalEntry[1]; };
struct RGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; };
struct BITMAPINFOHEADER { DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage; LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant; };
struct BITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; };
#pragma pack(push,2)
struct BITMAPFILEHEADER { WORD bfType; DWORD bfSize; WORD bfReserved1, bfReserved2; DWORD bfOffBits; };
#pragma pack(pop)
struct BITMAPCOREHEADER { DWORD bcSize; WORD bcWidth, bcHeight, bcPlanes, bcBitCount; };
struct RGBTRIPLE { BYTE rgbtBlue, rgbtGreen, rgbtRed; };
#define BI_RGB 0
struct DCB { DWORD BaudRate; };
struct SOCKADDR_IN { int unused; };
struct SYSTEMTIME { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; };

// ---- empty GDI/MFC UI classes (members of HP41 that the headless build never uses) ----
struct CFont {}; struct CBitmap {}; struct CPalette {}; struct CBrush {}; struct CDC {}; struct CWnd {};

// ---- critical sections ----
typedef pthread_mutex_t CRITICAL_SECTION;
void InitializeCriticalSection(CRITICAL_SECTION *cs);
void DeleteCriticalSection(CRITICAL_SECTION *cs);
void EnterCriticalSection(CRITICAL_SECTION *cs);
void LeaveCriticalSection(CRITICAL_SECTION *cs);

// ---- time ----
BOOL QueryPerformanceFrequency(LARGE_INTEGER *p);
BOOL QueryPerformanceCounter(LARGE_INTEGER *p);
void Sleep(DWORD ms);
DWORD GetTickCount();
DWORD timeGetTime();
extern DWORD (*g_tick_source)();
void GetLocalTime(SYSTEMTIME *st);
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef void (CALLBACK *LPTIMECALLBACK)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
UINT_PTR SetTimer(HWND, UINT_PTR, UINT, TIMERPROC);
BOOL KillTimer(HWND, UINT_PTR);
#define TIMERR_NOERROR 0
#define TIME_PERIODIC 1
#define TIME_ONESHOT 0
MMRESULT timeGetDevCaps(TIMECAPS *tc, UINT cb);
MMRESULT timeBeginPeriod(UINT);
MMRESULT timeEndPeriod(UINT);
MMRESULT timeSetEvent(UINT delay, UINT res, LPTIMECALLBACK cb, DWORD_PTR user, UINT flags);
MMRESULT timeKillEvent(MMRESULT id);

// ---- handles: files, threads, events ----
struct ShimHandle;
typedef ShimHandle *HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_FLAG_SEQUENTIAL_SCAN 0
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define INVALID_FILE_SIZE 0xFFFFFFFFu
#define INVALID_SET_FILE_POINTER 0xFFFFFFFFu
HANDLE CreateFile(LPCTSTR name, DWORD access, DWORD share, void *sec, DWORD disp, DWORD attrs, HANDLE templ);
BOOL ReadFile(HANDLE h, void *buf, DWORD n, DWORD *nread, void *ov);
BOOL WriteFile(HANDLE h, const void *buf, DWORD n, DWORD *nwritten, void *ov);
BOOL CloseHandle(HANDLE h);
DWORD GetFileSize(HANDLE h, DWORD *high);
DWORD SetFilePointer(HANDLE h, LONG dist, LONG *high, DWORD method);
BOOL SetEndOfFile(HANDLE h);
inline BOOL DeleteFile(LPCTSTR n) { return unlink(n) == 0; }
inline BOOL MoveFile(LPCTSTR a, LPCTSTR b) { return rename(a, b) == 0; }
HANDLE CreateEvent(void *sec, BOOL manual, BOOL initial, LPCTSTR name);
BOOL SetEvent(HANDLE h);
BOOL ResetEvent(HANDLE h);
DWORD WaitForSingleObject(HANDLE h, DWORD ms);
typedef UINT (*ShimThreadFn)(LPVOID);
HANDLE CreateThread(void *sec, size_t stack, DWORD (*fn)(LPVOID), LPVOID arg, DWORD flags, DWORD *tid);
uintptr_t _beginthreadex(void *sec, unsigned stack, ShimThreadFn fn, void *arg, unsigned flags, unsigned *tid);
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

// ---- CRT ----
void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext);
void _makepath(char *path, const char *drive, const char *dir, const char *fname, const char *ext);
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define wsprintf sprintf
#define lstrcpy strcpy
#define lstrcpyn(d,s,n) (strncpy((d),(s),(n)-1),(d)[(n)-1]=0,(d))
#define lstrlen (int)strlen
#define lstrcmp strcmp
#define lstrcmpi strcasecmp
#define lstrcat strcat
char *_strupr(char *s);
char *_strlwr(char *s);
#define strupr _strupr
#define strlwr _strlwr
char *_itoa(int v, char *buf, int radix);
#define itoa _itoa

// ---- message box ----
int AfxMessageBox(LPCTSTR text, UINT type = 0, UINT help = 0);

// ---- CString ----
class CString
  {
  public:
  std::string s;
  CString() {}
  CString(const char *p) : s(p ? p : "") {}
  CString(const char *p, int n) : s(p, n) {}
  CString(char c, int n = 1) : s(n, c) {}
  CString(const std::string &o) : s(o) {}
  operator LPCTSTR() const { return s.c_str(); }
  int GetLength() const { return (int)s.size(); }
  BOOL IsEmpty() const { return s.empty(); }
  void Empty() { s.clear(); }
  char GetAt(int i) const { return s[i]; }
  char operator[](int i) const { return s[i]; }
  void SetAt(int i, char c) { s[i] = c; }
  CString &operator=(const char *p) { s = p ? p : ""; return *this; }
  CString &operator=(char c) { s.assign(1, c); return *this; }
  CString &operator+=(const char *p) { if (p) s += p; return *this; }
  CString &operator+=(char c) { s += c; return *this; }
  CString &operator+=(const CString &o) { s += o.s; return *this; }
  void Format(const char *fmt, ...);
  void AppendFormat(const char *fmt, ...);
  void FormatV(const char *fmt, va_list ap);
  CString Left(int n) const { if (n < 0) n = 0; return CString(s.substr(0, n)); }
  CString Right(int n) const { if (n > (int)s.size()) n = (int)s.size(); return CString(s.substr(s.size() - n)); }
  CString Mid(int i) const { return i >= (int)s.size() ? CString() : CString(s.substr(i)); }
  CString Mid(int i, int n) const { return i >= (int)s.size() ? CString() : CString(s.substr(i, n)); }
  int Find(char c, int start = 0) const { size_t p = s.find(c, start); return p == std::string::npos ? -1 : (int)p; }
  int Find(const char *p, int start = 0) const { size_t q = s.find(p, start); return q == std::string::npos ? -1 : (int)q; }
  int ReverseFind(char c) const { size_t p = s.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  void MakeUpper() { for (auto &c : s) c = (char)toupper((unsigned char)c); }
  void MakeLower() { for (auto &c : s) c = (char)tolower((unsigned char)c); }
  int Compare(const char *p) const { return strcmp(s.c_str(), p); }
  int CompareNoCase(const char *p) const { return strcasecmp(s.c_str(), p); }
  void TrimLeft() { size_t i = 0; while (i < s.size() && isspace((unsigned char)s[i])) i++; s.erase(0, i); }
  void TrimRight() { while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back(); }
  void Trim() { TrimLeft(); TrimRight(); }
  char *GetBuffer(int n = 0) { if ((int)s.size() < n) s.resize(n); return &s[0]; }
  char *GetBufferSetLength(int n) { s.resize(n); return &s[0]; }
  void ReleaseBuffer(int n = -1) { if (n < 0) n = (int)strlen(s.c_str()); s.resize(n); }
  int Replace(const char *a, const char *b);
  int Delete(int i, int n = 1) { s.erase(i, n); return (int)s.size(); }
  int Insert(int i, const char *p) { s.insert(i, p); return (int)s.size(); }
  };
inline CString operator+(const CString &a, const CString &b) { return CString(a.s + b.s); }
inline CString operator+(const CString &a, const char *b) { return CString(a.s + (b ? b : "")); }
inline CString operator+(const char *a, const CString &b) { return CString(std::string(a ? a : "") + b.s); }
inline CString operator+(const CString &a, char b) { return CString(a.s + b); }
inline bool operator==(const CString &a, const char *b) { return a.s == (b ? b : ""); }
inline bool operator==(const CString &a, const CString &b) { return a.s == b.s; }
inline bool operator!=(const CString &a, const char *b) { return !(a == b); }
inline bool operator!=(const CString &a, const CString &b) { return a.s != b.s; }

class CStringArray
  {
  public:
  std::vector<CString> v;
  int Add(const char *p) { v.push_back(CString(p)); return (int)v.size() - 1; }
  int GetSize() const { return (int)v.size(); }
  int GetCount() const { return (int)v.size(); }
  CString GetAt(int i) const { return v[i]; }
  CString &operator[](int i) { return v[i]; }
  void RemoveAll() { v.clear(); }
  };

// ---- CPtrList (doubly linked, POSITION is a node pointer) ----
struct ShimNode { ShimNode *prev, *next; void *data; };
typedef ShimNode *POSITION;
class CPtrList
  {
  public:
  ShimNode *head, *tail; int count;
  CPtrList() : head(NULL), tail(NULL), count(0) {}
  ~CPtrList() { RemoveAll(); }
  POSITION AddTail(void *p) { ShimNode *n = new ShimNode{tail, NULL, p}; if (tail) tail->next = n; else head = n; tail = n; count++; return n; }
  POSITION AddHead(void *p) { ShimNode *n = new ShimNode{NULL, head, p}; if (head) head->prev = n; else tail = n; head = n; count++; return n; }
  POSITION GetHeadPosition() const { return head; }
  POSITION GetTailPosition() const { return tail; }
  void *&GetNext(POSITION &pos) const { ShimNode *n = pos; pos = n->next; return n->data; }
  void *&GetPrev(POSITION &pos) const { ShimNode *n = pos; pos = n->prev; return n->data; }
  void *&GetAt(POSITION pos) const { return pos->data; }
  void *&GetHead() const { return head->data; }
  void *&GetTail() const { return tail->data; }
  void RemoveAt(POSITION pos) { if (pos->prev) pos->prev->next = pos->next; else head = pos->next; if (pos->next) pos->next->prev = pos->prev; else tail = pos->prev; delete pos; count--; }
  void *RemoveHead() { void *d = head->data; RemoveAt(head); return d; }
  void *RemoveTail() { void *d = tail->data; RemoveAt(tail); return d; }
  void RemoveAll() { while (head) RemoveAt(head); }
  BOOL IsEmpty() const { return count == 0; }
  int GetCount() const { return count; }
  POSITION Find(void *p, POSITION start = NULL) const { for (ShimNode *n = start ? start->next : head; n; n = n->next) if (n->data == p) return n; return NULL; }
  };

// ---- COleDateTime (local time only) ----
class COleDateTime
  {
  public:
  struct tm t;
  double m_dt;   // OLE automation date: days since 1899-12-30, local time
  COleDateTime() : m_dt(0) { memset(&t, 0, sizeof(t)); }
  static COleDateTime GetCurrentTime()
    {
    COleDateTime d; time_t now = time(NULL); localtime_r(&now, &d.t);
    d.m_dt = ((double)now + (double)d.t.tm_gmtoff) / 86400.0 + 25569.0;
    return d;
    }
  int GetYear() const { return t.tm_year + 1900; }
  int GetMonth() const { return t.tm_mon + 1; }
  int GetDay() const { return t.tm_mday; }
  int GetHour() const { return t.tm_hour; }
  int GetMinute() const { return t.tm_min; }
  int GetSecond() const { return t.tm_sec; }
  int GetDayOfWeek() const { return t.tm_wday + 1; }
  int GetDayOfYear() const { return t.tm_yday + 1; }
  };

#endif
