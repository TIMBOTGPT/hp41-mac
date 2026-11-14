// hp41-mac: implementation of the Win32/MFC subset declared in StdAfx.h.
#include "StdAfx.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

// ---- critical sections (recursive, as on Windows) ----
void InitializeCriticalSection(CRITICAL_SECTION *cs)
  {
  pthread_mutexattr_t a; pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(cs, &a); pthread_mutexattr_destroy(&a);
  }
void DeleteCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_destroy(cs); }
void EnterCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_lock(cs); }
void LeaveCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_unlock(cs); }

// ---- time ----
static int64_t now_ns() { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec; }
BOOL QueryPerformanceFrequency(LARGE_INTEGER *p) { p->QuadPart = 1000000000LL; return TRUE; }
BOOL QueryPerformanceCounter(LARGE_INTEGER *p) { p->QuadPart = now_ns(); return TRUE; }
void Sleep(DWORD ms) { usleep((useconds_t)ms * 1000); }
// A driver may substitute an emulated millisecond clock (the headless build
// does, so timer tests are deterministic). The default is the host clock.
DWORD (*g_tick_source)() = NULL;
DWORD GetTickCount() { return g_tick_source ? g_tick_source() : (DWORD)(now_ns() / 1000000LL); }
DWORD timeGetTime() { return GetTickCount(); }
void GetLocalTime(SYSTEMTIME *st)
  {
  struct timeval tv; gettimeofday(&tv, NULL); struct tm t; time_t s = tv.tv_sec; localtime_r(&s, &t);
  st->wYear = t.tm_year + 1900; st->wMonth = t.tm_mon + 1; st->wDayOfWeek = t.tm_wday; st->wDay = t.tm_mday;
  st->wHour = t.tm_hour; st->wMinute = t.tm_min; st->wSecond = t.tm_sec; st->wMilliseconds = (WORD)(tv.tv_usec / 1000);
  }
// The headless build drives the CPU itself, so Windows timers are inert.
UINT_PTR SetTimer(HWND, UINT_PTR, UINT, TIMERPROC) { return 1; }
BOOL KillTimer(HWND, UINT_PTR) { return TRUE; }
MMRESULT timeGetDevCaps(TIMECAPS *tc, UINT) { tc->wPeriodMin = 1; tc->wPeriodMax = 1000; return TIMERR_NOERROR; }
MMRESULT timeBeginPeriod(UINT) { return TIMERR_NOERROR; }
MMRESULT timeEndPeriod(UINT) { return TIMERR_NOERROR; }
MMRESULT timeSetEvent(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT) { return 1; }
MMRESULT timeKillEvent(MMRESULT) { return TIMERR_NOERROR; }

// ---- handles ----
enum { HK_FILE = 1, HK_EVENT, HK_THREAD };
struct ShimHandle
  {
  int kind;
  int fd;
  pthread_mutex_t m; pthread_cond_t c; bool signaled, manual;
  pthread_t th; ShimThreadFn fn; DWORD (*fn2)(LPVOID); void *arg;
  };

HANDLE CreateFile(LPCTSTR name, DWORD access, DWORD, void *, DWORD disp, DWORD, HANDLE)
  {
  int fl = 0;
  if ((access & GENERIC_READ) && (access & GENERIC_WRITE)) fl = O_RDWR;
  else if (access & GENERIC_WRITE) fl = O_WRONLY;
  else fl = O_RDONLY;
  switch (disp)
    {
    case CREATE_NEW: fl |= O_CREAT | O_EXCL; break;
    case CREATE_ALWAYS: fl |= O_CREAT | O_TRUNC; break;
    case OPEN_ALWAYS: fl |= O_CREAT; break;
    default: break;
    }
  int fd = open(name, fl, 0644);
  if (fd < 0) return INVALID_HANDLE_VALUE;
  ShimHandle *h = new ShimHandle(); h->kind = HK_FILE; h->fd = fd; return h;
  }
BOOL ReadFile(HANDLE h, void *buf, DWORD n, DWORD *nread, void *)
  {
  ssize_t r = read(h->fd, buf, n); if (nread) *nread = r < 0 ? 0 : (DWORD)r; return r >= 0;
  }
BOOL WriteFile(HANDLE h, const void *buf, DWORD n, DWORD *nw, void *)
  {
  ssize_t r = write(h->fd, buf, n); if (nw) *nw = r < 0 ? 0 : (DWORD)r; return r == (ssize_t)n;
  }
DWORD GetFileSize(HANDLE h, DWORD *high)
  {
  struct stat st; if (fstat(h->fd, &st) != 0) return INVALID_FILE_SIZE; if (high) *high = 0; return (DWORD)st.st_size;
  }
DWORD SetFilePointer(HANDLE h, LONG dist, LONG *, DWORD method)
  {
  off_t r = lseek(h->fd, dist, method == FILE_BEGIN ? SEEK_SET : method == FILE_CURRENT ? SEEK_CUR : SEEK_END);
  return r < 0 ? INVALID_SET_FILE_POINTER : (DWORD)r;
  }
BOOL SetEndOfFile(HANDLE h) { off_t p = lseek(h->fd, 0, SEEK_CUR); return ftruncate(h->fd, p) == 0; }
BOOL CloseHandle(HANDLE h)
  {
  if (!h || h == INVALID_HANDLE_VALUE) return FALSE;
  if (h->kind == HK_FILE) close(h->fd);
  else if (h->kind == HK_EVENT) { pthread_mutex_destroy(&h->m); pthread_cond_destroy(&h->c); }
  delete h; return TRUE;
  }
HANDLE CreateEvent(void *, BOOL manual, BOOL initial, LPCTSTR)
  {
  ShimHandle *h = new ShimHandle(); h->kind = HK_EVENT; pthread_mutex_init(&h->m, NULL); pthread_cond_init(&h->c, NULL);
  h->signaled = initial != 0; h->manual = manual != 0; return h;
  }
BOOL SetEvent(HANDLE h) { pthread_mutex_lock(&h->m); h->signaled = true; pthread_cond_broadcast(&h->c); pthread_mutex_unlock(&h->m); return TRUE; }
BOOL ResetEvent(HANDLE h) { pthread_mutex_lock(&h->m); h->signaled = false; pthread_mutex_unlock(&h->m); return TRUE; }
DWORD WaitForSingleObject(HANDLE h, DWORD ms)
  {
  if (h->kind == HK_THREAD) { pthread_join(h->th, NULL); return WAIT_OBJECT_0; }
  DWORD rc = WAIT_OBJECT_0;
  pthread_mutex_lock(&h->m);
  if (ms == INFINITE) { while (!h->signaled) pthread_cond_wait(&h->c, &h->m); }
  else
    {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000; ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    while (!h->signaled) if (pthread_cond_timedwait(&h->c, &h->m, &ts) == ETIMEDOUT) break;
    if (!h->signaled) rc = WAIT_TIMEOUT;
    }
  if (rc == WAIT_OBJECT_0 && !h->manual) h->signaled = false;
  pthread_mutex_unlock(&h->m);
  return rc;
  }
static void *thread_tramp(void *p) { ShimHandle *h = (ShimHandle *)p; if (h->fn) h->fn(h->arg); else h->fn2(h->arg); return NULL; }
uintptr_t _beginthreadex(void *, unsigned, ShimThreadFn fn, void *arg, unsigned, unsigned *tid)
  {
  ShimHandle *h = new ShimHandle(); h->kind = HK_THREAD; h->fn = fn; h->fn2 = NULL; h->arg = arg;
  if (pthread_create(&h->th, NULL, thread_tramp, h) != 0) { delete h; return 0; }
  if (tid) *tid = 1; return (uintptr_t)h;
  }
HANDLE CreateThread(void *, size_t, DWORD (*fn)(LPVOID), LPVOID arg, DWORD, DWORD *tid)
  {
  ShimHandle *h = new ShimHandle(); h->kind = HK_THREAD; h->fn = NULL; h->fn2 = fn; h->arg = arg;
  if (pthread_create(&h->th, NULL, thread_tramp, h) != 0) { delete h; return NULL; }
  if (tid) *tid = 1; return h;
  }

// ---- CRT ----
// POSIX paths: "drive" is always empty, "dir" keeps its trailing slash.
void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
  {
  if (drive) drive[0] = 0;
  const char *slash = strrchr(path, '/');
  const char *base = slash ? slash + 1 : path;
  if (dir) { size_t n = base - path; memcpy(dir, path, n); dir[n] = 0; }
  const char *dot = strrchr(base, '.');
  if (dot == base) dot = NULL;
  if (fname) { size_t n = dot ? (size_t)(dot - base) : strlen(base); memcpy(fname, base, n); fname[n] = 0; }
  if (ext) strcpy(ext, dot ? dot : "");
  }
void _makepath(char *path, const char *, const char *dir, const char *fname, const char *ext)
  {
  path[0] = 0;
  if (dir && *dir) { strcpy(path, dir); size_t n = strlen(path); if (path[n - 1] != '/') strcat(path, "/"); }
  if (fname) strcat(path, fname);
  if (ext && *ext) { if (ext[0] != '.') strcat(path, "."); strcat(path, ext); }
  }
char *_strupr(char *s) { for (char *p = s; *p; p++) *p = (char)toupper((unsigned char)*p); return s; }
char *_strlwr(char *s) { for (char *p = s; *p; p++) *p = (char)tolower((unsigned char)*p); return s; }
char *_itoa(int v, char *buf, int radix)
  {
  if (radix == 16) sprintf(buf, "%x", v); else if (radix == 8) sprintf(buf, "%o", v); else sprintf(buf, "%d", v);
  return buf;
  }

int AfxMessageBox(LPCTSTR text, UINT, UINT) { fprintf(stderr, "[V41 message] %s\n", text ? text : ""); return IDOK; }

// ---- CString ----
void CString::FormatV(const char *fmt, va_list ap)
  {
  va_list ap2; va_copy(ap2, ap);
  int n = vsnprintf(NULL, 0, fmt, ap2); va_end(ap2);
  std::string out(n > 0 ? n : 0, '\0');
  if (n > 0) vsnprintf(&out[0], n + 1, fmt, ap);
  s = out;
  }
void CString::Format(const char *fmt, ...) { va_list ap; va_start(ap, fmt); FormatV(fmt, ap); va_end(ap); }
void CString::AppendFormat(const char *fmt, ...) { CString t; va_list ap; va_start(ap, fmt); t.FormatV(fmt, ap); va_end(ap); s += t.s; }
int CString::Replace(const char *a, const char *b)
  {
  int n = 0; size_t la = strlen(a), lb = strlen(b), pos = 0;
  if (!la) return 0;
  while ((pos = s.find(a, pos)) != std::string::npos) { s.replace(pos, la, b); pos += lb; n++; }
  return n;
  }
