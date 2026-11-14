// hp41-mac headless driver.
// Loads MOD files, runs the V41 Nut core, feeds scripted keys and prints
// the display. Used for automated acceptance tests.
//
//   hp41-headless [--trace FILE] [--state FILE] --mod A.MOD [--mod B.MOD ...] [--keys "ON 2 ENTER 3 +"]
#include "StdAfx.h"
#include "V41.h"
#include <map>
#include <string>
#include <sstream>

static HP41 *hp;
static uint64_t g_instr = 0;

static std::string trim_right(const char *s)
  {
  std::string r(s);
  while (!r.empty() && (r.back() == ' ' || (unsigned char)r.back() == 0xa0)) r.pop_back();
  return r;
  }

static std::string display()
  {
  char buf[64]; memset(buf, 0, sizeof(buf));
  hp->GetDisplay(buf);
  return trim_right(buf);
  }

// DIS_ANNUN_REG is 12 bits, BAT in the most significant bit, ALPHA in the least
static std::string annunciators()
  {
  static const char *names[12] = {"BAT","USER","G","RAD","SHIFT","0","1","2","3","4","PRGM","ALPHA"};
  std::string r;
  for (int i = 0; i < 12; i++)
    if (hp->DIS_ANNUN_REG & (0x800 >> i)) { if (!r.empty()) r += ' '; r += names[i]; }
  return r;
  }

// Emulated time. Each loop step, executed or sleeping, stands for one
// instruction cycle of a halfnut HP-41CX (173 us, DEFAULT_INST_SPEED in
// HP41.h). The Time module is stepped every 10 ms of emulated time, as
// V41's Windows multimedia timer would. Results do not depend on host speed.
static uint64_t g_emu_us = 0;
static DWORD emu_ms() { return (DWORD)(g_emu_us / 1000); }

static void tick()
  {
  uint64_t before = g_emu_us / 10000;
  g_emu_us += DEFAULT_INST_SPEED;
  if (g_emu_us / 10000 != before && hp->fTimer)
    HP41::TimerProc(0, 0, (DWORD_PTR)hp, 0, 0);
  }

// run until the CPU sleeps (idle waiting for a key) or the limit is reached
static uint64_t run(uint64_t max_instr, bool stop_on_sleep)
  {
  uint64_t n = 0;
  hp->fTimerEnable = TRUE;
  while (n < max_instr)
    {
    if (hp->IsSleeping())
      {
      if (stop_on_sleep) break;
      tick(); n++; continue;
      }
    hp->Execute();
    tick();
    n++;
    }
  g_instr += n;
  return n;
  }

static const std::map<std::string, int> &keymap()
  {
  static const std::map<std::string, int> m = {
    {"ON",0x18},{"USER",0xc6},{"PRGM",0xc5},{"ALPHA",0xc4},
    {"SIGMA+",0x10},{"1/X",0x30},{"SQRT",0x70},{"LOG",0x80},{"LN",0xc0},
    {"X<>Y",0x11},{"RDN",0x31},{"SIN",0x71},{"COS",0x81},{"TAN",0xc1},
    {"SHIFT",0x12},{"XEQ",0x32},{"STO",0x72},{"RCL",0x82},{"SST",0xc2},
    {"ENTER",0x13},{"CHS",0x73},{"EEX",0x83},{"BACK",0xc3},
    {"-",0x14},{"7",0x34},{"8",0x74},{"9",0x84},
    {"+",0x15},{"4",0x35},{"5",0x75},{"6",0x85},
    {"*",0x16},{"1",0x36},{"2",0x76},{"3",0x86},
    {"/",0x17},{"0",0x37},{".",0x77},{"R/S",0x87},
    // alpha-mode letters share the keys above
    {"A",0x10},{"B",0x30},{"C",0x70},{"D",0x80},{"E",0xc0},
    {"F",0x11},{"G",0x31},{"H",0x71},{"I",0x81},{"J",0xc1},
    {"K",0x32},{"L",0x72},{"M",0x82},
    {"N",0x13},{"O",0x73},{"P",0x83},
    {"Q",0x14},{"R",0x34},{"S",0x74},{"T",0x84},
    {"U",0x15},{"V",0x35},{"W",0x75},{"X",0x85},
    {"Y",0x16},{"Z",0x36},
  };
  return m;
  }

static bool press(const std::string &name)
  {
  if (name.compare(0, 5, "WAIT:") == 0)   // let emulated time pass, CPU awake or asleep
    {
    double sec = atof(name.c_str() + 5);
    run((uint64_t)(sec * 1e6 / DEFAULT_INST_SPEED), false);
    printf("wait %.1f s -> [%-24s] ann[%s] (emulated time %.2f s)\n", sec, display().c_str(), annunciators().c_str(), g_emu_us / 1e6);
    return true;
    }
  auto it = keymap().find(name);
  if (it == keymap().end()) { fprintf(stderr, "unknown key: %s\n", name.c_str()); return false; }
  byte code = (byte)it->second;
  hp->SetKeyDown(code);
  run(3000, false);            // hold the key
  hp->SetKeyUp(code);
  uint64_t n = run(20000000, true);
  printf("key %-6s -> [%-24s] ann[%s] (%llu instr)\n", name.c_str(), display().c_str(), annunciators().c_str(), (unsigned long long)n + 3000);
  return true;
  }

int main(int argc, char **argv)
  {
  std::vector<std::string> mods; std::string keys, trace, wait;
  for (int i = 1; i < argc; i++)
    {
    std::string a = argv[i];
    if (a == "--mod" && i + 1 < argc) mods.push_back(argv[++i]);
    else if (a == "--keys" && i + 1 < argc) keys = argv[++i];
    else if (a == "--trace" && i + 1 < argc) trace = argv[++i];
    else if (a == "--modinfo" && i + 1 < argc) return output_mod_info(stdout, argv[++i], 1, 1, NULL);
    else { fprintf(stderr, "usage: %s [--trace FILE] --mod FILE.MOD ... [--keys \"ON 2 ENTER 3 +\"]\n", argv[0]); return 2; }
    }
  if (mods.empty()) { fprintf(stderr, "no --mod given\n"); return 2; }

  g_tick_source = emu_ms;
  realpath(argv[0], theApp.szAppFullPath);
  theApp.szSessionName[0] = 0;
  static CMainWindow wnd; theApp.pMainWnd = theApp.m_pMainWnd = &wnd;

  hp = new HP41();
  for (auto &m : mods)
    {
    ModuleHeader *pm = NULL;
    int rc = hp->LoadMOD(pm, m.c_str());
    if (rc != 0 || !pm) { fprintf(stderr, "LoadMOD failed (%d): %s\n", rc, m.c_str()); return 1; }
    printf("loaded %-28s title=\"%s\" pages=%d\n", m.substr(m.rfind('/') + 1).c_str(), pm->szTitle, pm->NumPages);
    }
  hp->SetKeyboard(HP41::eKeyboardSmall, 0, 0, 0);
  printf("config: MemModules=%d XMemModules=%d timer=%d hpil=%d printer=%d\n", hp->MemModules, hp->XMemModules, hp->fTimer, hp->fHPIL, hp->fPrinter);
  if (!trace.empty())
    {
    strncpy(hp->szTraceFilename, trace.c_str(), sizeof(hp->szTraceFilename) - 1);
    hp->StartTrace();
    }

  std::istringstream ss(keys); std::string k; int rc = 0;
  while (ss >> k) if (!press(k)) { rc = 1; break; }

  if (hp->GetTrace()) hp->StopTrace();
  printf("final display: [%s]\n", display().c_str());
  printf("instructions executed: %llu\n", (unsigned long long)g_instr);
  printf("emulated time: %.2f s\n", g_emu_us / 1e6);
  return rc;
  }
