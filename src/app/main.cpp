// hp41-mac: SDL2 front end for the V41 emulation core.
// GPL v2 or later.
//
//   hp41 [--skin NAME] [--scale N] [--keys "ON 2 ENTER 3 +"] [--screenshot FILE.bmp] [--fresh]
//
// State lives in ~/Library/Application Support/hp41-mac/HP-41CX.lod and is
// saved on quit. ROM images (MOD files) are looked up in the MOD folder there.
#include "StdAfx.h"
#include "V41.h"
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <map>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <mach-o/dyld.h>

struct Rect { float x, y, w, h; };
struct Key { int code; Rect r; };
struct Skin
  {
  int faceW = 0, faceH = 0;
  Rect lcd{}, cells{}, annrow{};
  SDL_Color bg{0xb9, 0xc0, 0xa8, 255}, fg{0x1d, 0x24, 0x1c, 255};
  std::vector<Key> keys;
  SDL_Texture *tex = NULL;
  };

static HP41 *hp;
static Skin skin;
static SDL_Renderer *ren;

static bool exists(const std::string &p) { struct stat st; return stat(p.c_str(), &st) == 0; }

static SDL_Color hexcolor(const std::string &h)
  {
  unsigned v = (unsigned)strtoul(h.c_str(), NULL, 16);
  return SDL_Color{(Uint8)(v >> 16), (Uint8)(v >> 8), (Uint8)v, 255};
  }

static bool load_skin(const std::string &base)
  {
  FILE *f = fopen((base + ".layout").c_str(), "r");
  if (!f) return false;
  char line[256];
  while (fgets(line, sizeof(line), f))
    {
    std::istringstream ss(line); std::string t; ss >> t;
    if (t == "face") ss >> skin.faceW >> skin.faceH;
    else if (t == "lcd") ss >> skin.lcd.x >> skin.lcd.y >> skin.lcd.w >> skin.lcd.h;
    else if (t == "cells") ss >> skin.cells.x >> skin.cells.y >> skin.cells.w >> skin.cells.h;
    else if (t == "annrow") ss >> skin.annrow.x >> skin.annrow.y >> skin.annrow.w >> skin.annrow.h;
    else if (t == "lcdcolors") { std::string a, b; ss >> a >> b; skin.bg = hexcolor(a); skin.fg = hexcolor(b); }
    else if (t == "key") { Key k; ss >> std::hex >> k.code >> std::dec >> k.r.x >> k.r.y >> k.r.w >> k.r.h; skin.keys.push_back(k); }
    }
  fclose(f);
  int w, h, n;
  unsigned char *px = stbi_load((base + ".png").c_str(), &w, &h, &n, 4);
  if (!px) return false;
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(px, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  skin.tex = SDL_CreateTextureFromSurface(ren, s);
  SDL_FreeSurface(s); stbi_image_free(px);
  return skin.tex != NULL && skin.faceW > 0;
  }

// ---------- 14-segment LCD ----------
// segments: a top, b upper right, c lower right, d bottom, e lower left, f upper left,
// g middle left, G middle right, h/i/j upper diagonals and centre bar, k/l/m lower ones
static const char *glyph(unsigned short u)
  {
  static const std::map<unsigned short, const char *> m = {
    {'0',"abcdef"},{'1',"bc"},{'2',"abgGed"},{'3',"abGcd"},{'4',"fgGbc"},{'5',"afgGcd"},{'6',"afgGcde"},{'7',"abc"},{'8',"abcdefgG"},{'9',"abcdfgG"},
    {'A',"abcefgG"},{'B',"abcdGil"},{'C',"adef"},{'D',"abcdil"},{'E',"adefgG"},{'F',"aefgG"},{'G',"acdefG"},{'H',"bcefgG"},{'I',"adil"},{'J',"bcde"},
    {'K',"efgjm"},{'L',"def"},{'M',"bcefhj"},{'N',"bcefhm"},{'O',"abcdef"},{'P',"abefgG"},{'Q',"abcdefm"},{'R',"abefgGm"},{'S',"afgGcd"},{'T',"ail"},
    {'U',"bcdef"},{'V',"efjk"},{'W',"bcefkm"},{'X',"hjkm"},{'Y',"hjl"},{'Z',"adjk"},
    {' ',""},{'-',"gG"},{'+',"gGil"},{'*',"gGhjkmil"},{'/',"jk"},{'=',"dgG"},{'<',"jm"},{'>',"hk"},{'_',"d"},{'?',"abGl"},{'!',"il"},
    {'"',"fi"},{'#',"bcdgGil"},{'$',"afgGcdil"},{'%',"cfjk"},{'&',"adehgkm"},{'\'',"i"},{'(',"jm"},{')',"hk"},{'[',"adef"},{']',"abcd"},
    {'\\',"hm"},{'@',"abdefGi"},{';',"jl"},{'{',"jm"},{'}',"hk"},
    {'a',"cdegG"},{'b',"cdefgG"},{'c',"degG"},{'d',"bcdegG"},{'e',"adefgk"},
    {0x03A3,"adhk"},{0x2191,"ilkm"},{0x00B5,"befgk"},{0x2221,"djk"},{0x00AF,"a"},{0x0370,"efg"},
  };
  auto it = m.find(u);
  if (it != m.end()) return it->second;
  if (u >= 'f' && u <= 'z') return glyph((unsigned short)(u - 32));
  return "abcdefgGhijklm";   // starburst for anything unmapped
  }

static void thick_line(float x1, float y1, float x2, float y2, float t, SDL_Color c)
  {
  float dx = x2 - x1, dy = y2 - y1, len = SDL_sqrtf(dx * dx + dy * dy);
  if (len < 0.01f) return;
  float nx = -dy / len * t / 2, ny = dx / len * t / 2;
  SDL_Vertex v[4] = {
    {{x1 + nx, y1 + ny}, c, {0, 0}}, {{x2 + nx, y2 + ny}, c, {0, 0}},
    {{x2 - nx, y2 - ny}, c, {0, 0}}, {{x1 - nx, y1 - ny}, c, {0, 0}}};
  int idx[6] = {0, 1, 2, 0, 2, 3};
  SDL_RenderGeometry(ren, NULL, v, 4, idx, 6);
  }

static void fill(float x, float y, float w, float h, SDL_Color c)
  {
  SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
  SDL_FRect r{x, y, w, h}; SDL_RenderFillRectF(ren, &r);
  }

static void draw_char(float x, float y, float w, float h, const char *segs, SDL_Color c)
  {
  static const struct { char id; float x1, y1, x2, y2; } S[] = {
    {'a',0,0,1,0},{'b',1,0,1,.5f},{'c',1,.5f,1,1},{'d',0,1,1,1},{'e',0,.5f,0,1},{'f',0,0,0,.5f},
    {'g',0,.5f,.5f,.5f},{'G',.5f,.5f,1,.5f},{'h',0,0,.5f,.5f},{'i',.5f,0,.5f,.5f},{'j',1,0,.5f,.5f},
    {'k',.5f,.5f,0,1},{'l',.5f,.5f,.5f,1},{'m',.5f,.5f,1,1}};
  float t = w * 0.13f; if (t < 1.2f) t = 1.2f;
  for (const char *p = segs; *p; p++)
    for (auto &s : S)
      if (s.id == *p)
        {
        float ax = x + s.x1 * w, ay = y + s.y1 * h, bx = x + s.x2 * w, by = y + s.y2 * h;
        float dx = bx - ax, dy = by - ay;          // pull the ends in a little so segments read separately
        bool diag = dx != 0 && dy != 0;
        float in = diag ? 0.16f : 0.07f;
        thick_line(ax + dx * in, ay + dy * in, bx - dx * in, by - dy * in, diag ? t * 0.85f : t, c);
        }
  }

// 5x7 font for the annunciator row
static const unsigned char *font5(char ch)
  {
  static const std::map<char, std::vector<unsigned char>> F = {
    {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},{'T',{31,4,4,4,4,4,4}},{'U',{17,17,17,17,17,17,14}},
    {'S',{15,16,16,14,1,1,30}},{'E',{31,16,16,30,16,16,31}},{'R',{30,17,17,30,20,18,17}},{'G',{14,17,16,23,17,17,15}},
    {'D',{30,17,17,17,17,17,30}},{'H',{17,17,17,31,17,17,17}},{'I',{14,4,4,4,4,4,14}},{'F',{31,16,16,30,16,16,16}},
    {'P',{30,17,17,30,16,16,16}},{'M',{17,27,21,21,17,17,17}},{'L',{16,16,16,16,16,16,31}},
    {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},{'2',{14,17,1,2,4,8,31}},{'3',{30,1,1,14,1,1,30}},{'4',{2,6,10,18,31,2,2}}};
  auto it = F.find(ch); return it == F.end() ? NULL : it->second.data();
  }

static void draw_text5(float x, float y, float px, const char *s, SDL_Color c)
  {
  for (; *s; s++, x += 6 * px)
    {
    const unsigned char *g = font5(*s); if (!g) continue;
    for (int r = 0; r < 7; r++)
      for (int b = 0; b < 5; b++)
        if (g[r] & (16 >> b)) fill(x + b * px, y + r * px, px, px, c);
    }
  }

static void draw_lcd()
  {
  fill(skin.lcd.x, skin.lcd.y, skin.lcd.w, skin.lcd.h, skin.bg);
  if (!hp->DisplayOn) return;
  float cw = skin.cells.w / 12.0f, gw = cw * 0.60f, gh = skin.cells.h * 0.92f;
  float gy = skin.cells.y + (skin.cells.h - gh) / 2;
  for (int i = 0; i < 12; i++)
    {
    word lcdchar = (hp->DIS_C_REG[11 - i] << 8) | (hp->DIS_B_REG[11 - i] << 4) | hp->DIS_A_REG[11 - i];
    char ch, punct; unsigned short wch;
    decode_lcdchar(lcdchar, &ch, &wch, &punct);
    float gx = skin.cells.x + i * cw + cw * 0.08f;
    draw_char(gx, gy, gw, gh, glyph(wch), skin.fg);
    float d = cw * 0.11f, px = gx + gw + cw * 0.10f;
    if (punct == '.' || punct == ',' || punct == ':') fill(px, gy + gh - d, d, d, skin.fg);
    if (punct == ':') fill(px, gy + gh * 0.55f, d, d, skin.fg);
    if (punct == ',') thick_line(px + d * 0.5f, gy + gh, px - d * 0.3f, gy + gh + d * 1.4f, d * 0.7f, skin.fg);
    }
  static const struct { const char *name; float at; } A[12] = {
    {"BAT",0},{"USER",.10f},{"G",.235f},{"RAD",.27f},{"SHIFT",.38f},{"0",.53f},{"1",.565f},{"2",.60f},{"3",.635f},{"4",.67f},{"PRGM",.74f},{"ALPHA",.88f}};
  float px = SDL_min(skin.annrow.h / 7.0f, skin.annrow.w / 300.0f);
  for (int i = 0; i < 12; i++)
    if (hp->DIS_ANNUN_REG & (0x800 >> i))
      draw_text5(skin.annrow.x + A[i].at * skin.annrow.w, skin.annrow.y + (skin.annrow.h - 7 * px) / 2, px, A[i].name, skin.fg);
  }

static int pressed_mouse = 0;
static std::map<int, int> down_count;      // HP key code -> number of sources holding it

static void key_down(int code) { if (code && down_count[code]++ == 0) hp->SetKeyDown((byte)code); }
static void key_up(int code) { if (code && down_count[code] > 0 && --down_count[code] == 0) hp->SetKeyUp((byte)code); }

static void draw()
  {
  SDL_SetRenderDrawColor(ren, 24, 24, 26, 255); SDL_RenderClear(ren);
  SDL_Rect src{0, 0, 0, 0}; SDL_QueryTexture(skin.tex, NULL, NULL, &src.w, &src.h);
  SDL_FRect dst{0, 0, (float)skin.faceW, (float)skin.faceH};
  SDL_RenderCopyF(ren, skin.tex, &src, &dst);
  draw_lcd();
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
  for (auto &k : skin.keys)
    if (down_count[k.code] > 0) fill(k.r.x, k.r.y, k.r.w, k.r.h, SDL_Color{0, 0, 0, 90});
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
  SDL_RenderPresent(ren);
  }

static int key_at(float x, float y)
  {
  for (auto &k : skin.keys)
    if (x >= k.r.x && x < k.r.x + k.r.w && y >= k.r.y && y < k.r.y + k.r.h) return k.code;
  return 0;
  }

// Mac keyboard to HP-41 key. Letters map to the key carrying that letter in ALPHA mode.
static int map_key(SDL_Keycode k)
  {
  static const int alpha[26] = {0x10,0x30,0x70,0x80,0xc0, 0x11,0x31,0x71,0x81,0xc1, 0x32,0x72,0x82, 0x13,0x73,0x83, 0x14,0x34,0x74,0x84, 0x15,0x35,0x75,0x85, 0x16,0x36};
  static const int digit[10] = {0x37,0x36,0x76,0x86,0x35,0x75,0x85,0x34,0x74,0x84};
  if (k >= SDLK_a && k <= SDLK_z) return alpha[k - SDLK_a];
  if (k >= SDLK_0 && k <= SDLK_9) return digit[k - SDLK_0];
  if (k >= SDLK_KP_1 && k <= SDLK_KP_9) return digit[k - SDLK_KP_1 + 1];
  switch (k)
    {
    case SDLK_KP_0: return 0x37;
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x13;
    case SDLK_BACKSPACE: case SDLK_ESCAPE: case SDLK_DELETE: return 0xc3;
    case SDLK_MINUS: case SDLK_KP_MINUS: return 0x14;
    case SDLK_EQUALS: case SDLK_KP_PLUS: return 0x15;
    case SDLK_QUOTE: case SDLK_KP_MULTIPLY: return 0x16;
    case SDLK_SLASH: case SDLK_KP_DIVIDE: return 0x17;
    case SDLK_PERIOD: case SDLK_COMMA: case SDLK_KP_PERIOD: return 0x77;
    case SDLK_SPACE: return 0x37;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x12;
    case SDLK_F2: case SDLK_BACKQUOTE: return 0x18;
    case SDLK_F3: return 0xc6;
    case SDLK_F4: return 0xc5;
    case SDLK_F5: case SDLK_TAB: return 0xc4;
    case SDLK_F6: return 0xc2;
    case SDLK_F7: return 0x87;
    }
  return 0;
  }

static uint64_t run(uint64_t max_instr, bool stop_on_sleep)
  {
  uint64_t n = 0;
  hp->fTimerEnable = TRUE;
  for (; n < max_instr; n++)
    {
    if (hp->IsSleeping()) { if (stop_on_sleep) break; continue; }
    hp->Execute();
    }
  return n;
  }

static const std::map<std::string, int> &names()
  {
  static const std::map<std::string, int> m = {
    {"ON",0x18},{"USER",0xc6},{"PRGM",0xc5},{"ALPHA",0xc4},{"SIGMA+",0x10},{"1/X",0x30},{"SQRT",0x70},{"LOG",0x80},{"LN",0xc0},
    {"X<>Y",0x11},{"RDN",0x31},{"SIN",0x71},{"COS",0x81},{"TAN",0xc1},{"SHIFT",0x12},{"XEQ",0x32},{"STO",0x72},{"RCL",0x82},{"SST",0xc2},
    {"ENTER",0x13},{"CHS",0x73},{"EEX",0x83},{"BACK",0xc3},{"-",0x14},{"7",0x34},{"8",0x74},{"9",0x84},{"+",0x15},{"4",0x35},{"5",0x75},{"6",0x85},
    {"*",0x16},{"1",0x36},{"2",0x76},{"3",0x86},{"/",0x17},{"0",0x37},{".",0x77},{"R/S",0x87}};
  return m;
  }

static std::string data_dir()
  {
  std::string d = std::string(getenv("HOME")) + "/Library/Application Support/hp41-mac";
  mkdir((std::string(getenv("HOME")) + "/Library/Application Support").c_str(), 0755);
  mkdir(d.c_str(), 0755);
  return d;
  }

int main(int argc, char **argv)
  {
  std::string skinname, keys, shot; float scale = 0; bool fresh = false;
  for (int i = 1; i < argc; i++)
    {
    std::string a = argv[i];
    if (a == "--skin" && i + 1 < argc) skinname = argv[++i];
    else if (a == "--scale" && i + 1 < argc) scale = (float)atof(argv[++i]);
    else if (a == "--keys" && i + 1 < argc) keys = argv[++i];
    else if (a == "--screenshot" && i + 1 < argc) shot = argv[++i];
    else if (a == "--fresh") fresh = true;
    else if (a.rfind("-psn", 0) == 0) continue;   // passed by Finder on old systems
    else { fprintf(stderr, "usage: hp41 [--skin NAME] [--scale N] [--keys \"...\"] [--screenshot FILE.bmp] [--fresh]\n"); return 2; }
    }

  // locate resources: Contents/Resources in a bundle, else the repository root
  char exe[2048]; uint32_t n = sizeof(exe); _NSGetExecutablePath(exe, &n);
  char real[2048]; realpath(exe, real);
  std::string dir = real; dir = dir.substr(0, dir.rfind('/'));
  std::string res = exists(dir + "/../Resources/skin") ? dir + "/../Resources" : dir + "/..";

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }

  std::string base;
  if (!skinname.empty()) base = exists(res + "/skin/local/" + skinname + ".layout") ? res + "/skin/local/" + skinname : res + "/skin/" + skinname;
  else base = exists(res + "/skin/local/v41.layout") ? res + "/skin/local/v41" : res + "/skin/hp41";

  // read the face size before creating the window
  { FILE *f = fopen((base + ".layout").c_str(), "r"); char line[256];
    if (!f) { fprintf(stderr, "skin not found: %s\n", base.c_str()); return 1; }
    while (fgets(line, sizeof(line), f)) if (sscanf(line, "face %d %d", &skin.faceW, &skin.faceH) == 2) break;
    fclose(f); }
  if (scale <= 0)
    {
    SDL_Rect ub; scale = 1;
    if (SDL_GetDisplayUsableBounds(0, &ub) == 0 && skin.faceH > ub.h - 40) scale = (float)(ub.h - 40) / skin.faceH;
    }
  SDL_Window *win = SDL_CreateWindow("HP-41CX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     (int)(skin.faceW * scale), (int)(skin.faceH * scale),
                                     SDL_WINDOW_ALLOW_HIGHDPI | (shot.empty() ? 0 : SDL_WINDOW_HIDDEN));
  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  skin = Skin();
  if (!load_skin(base)) { fprintf(stderr, "cannot load skin %s\n", base.c_str()); return 1; }
  SDL_RenderSetLogicalSize(ren, skin.faceW, skin.faceH);

  // emulator
  std::string data = data_dir(), lod = data + "/HP-41CX.lod";
  snprintf(theApp.szAppFullPath, sizeof(theApp.szAppFullPath), "%s/hp41", data.c_str());
  theApp.szSessionName[0] = 0;
  static CMainWindow wnd; theApp.pMainWnd = theApp.m_pMainWnd = &wnd;
  if (fresh) unlink(lod.c_str());
  if (!exists(lod)) { FILE *f = fopen(lod.c_str(), "w"); if (f) { fputs(";hp41-mac session\r\n$MODULE NUT-CX\r\n", f); fclose(f); } }

  hp = new HP41();
  hp->SetKeyboard(HP41::eKeyboardSmall, 0, 0, 0);
  bool dm41 = false;
  int rc = hp->LoadConfig(lod.c_str(), dm41);
  if (rc != 0 || hp->PageMatrix[0][0] == NULL)
    {
    std::string msg = "No HP-41 ROM image was loaded.\n\nPut NUT-CX.MOD (and any other MOD files) in:\n" + data + "/MOD\n\nROM images are not distributed with hp41-mac.";
    fprintf(stderr, "%s\n", msg.c_str());
    if (shot.empty()) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "HP-41CX", msg.c_str(), win);
    return 1;
    }

  // scripted keys (for tests and screenshots)
  if (!keys.empty())
    {
    std::istringstream ss(keys); std::string k;
    while (ss >> k)
      {
      auto it = names().find(k); if (it == names().end()) { fprintf(stderr, "unknown key %s\n", k.c_str()); return 2; }
      hp->SetKeyDown((byte)it->second); run(3000, false); hp->SetKeyUp((byte)it->second); run(20000000, true);
      }
    }
  if (!shot.empty())
    {
    draw();
    int w, h; SDL_GetRendererOutputSize(ren, &w, &h);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    draw(); SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch);
    SDL_SaveBMP(s, shot.c_str());
    char d[64] = ""; hp->GetDisplay(d); printf("display: [%s]\n", d);
    hp->SaveConfig(lod.c_str(), TRUE);
    return 0;
    }

  // main loop: authentic speed is 1e6/173 instructions per second
  const double IPS = 1e6 / DEFAULT_INST_SPEED;
  bool turbo = false, quit = false;
  Uint64 freq = SDL_GetPerformanceFrequency(), last = SDL_GetPerformanceCounter();
  double owed = 0, tick_owed = 0;
  while (!quit)
    {
    SDL_Event e;
    while (SDL_PollEvent(&e))
      {
      if (e.type == SDL_QUIT) quit = true;
      else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        { pressed_mouse = key_at((float)e.button.x, (float)e.button.y); key_down(pressed_mouse); }
      else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
        { key_up(pressed_mouse); pressed_mouse = 0; }
      else if (e.type == SDL_KEYDOWN && !e.key.repeat)
        {
        if (e.key.keysym.sym == SDLK_F9) { turbo = !turbo; SDL_SetWindowTitle(win, turbo ? "HP-41CX (turbo)" : "HP-41CX"); }
        else if (!(e.key.keysym.mod & KMOD_GUI)) key_down(map_key(e.key.keysym.sym));
        }
      else if (e.type == SDL_KEYUP) key_up(map_key(e.key.keysym.sym));
      }
    Uint64 now = SDL_GetPerformanceCounter();
    double dt = (double)(now - last) / freq; last = now;
    if (dt > 0.25) dt = 0.25;
    // timer module: V41 advances its clocks every 10 ms
    for (tick_owed += dt; tick_owed >= 0.01; tick_owed -= 0.01)
      HP41::TimerProc(0, 0, (DWORD_PTR)hp, 0, 0);
    owed += dt * IPS * (turbo ? 50 : 1);
    uint64_t todo = (uint64_t)owed; owed -= (double)todo;
    run(todo, false);
    draw();
    }
  hp->SaveConfig(lod.c_str(), TRUE);
  SDL_Quit();
  return 0;
  }
