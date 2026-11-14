#!/usr/bin/env python3
"""Generate the hp41-mac skin: skin/hp41.svg and skin/hp41.layout.

Original artwork for this project. It follows the HP-41 key arrangement and
legends so the emulator can be operated, and carries no HP branding.
Render the PNG with tools/render_skin.sh.

All coordinates in the layout file are in 1x sheet pixels.
"""
import os, html
here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, '..', 'skin')
W, H = 360, 730
SHEET_H = H
FONT = "Helvetica Neue, Helvetica, Arial, sans-serif"
e = html.escape

GOLD, BLUE, WHITE = '#e3a53c', '#7fb2e6', '#f2f2ee'
LCD_BG, LCD_FG = '#b9c0a8', '#1d241c'

# ---- LCD ----
LCD = (36, 46, 288, 78)                 # window
CELLS = (48, 56, 264, 44)               # 12 character cells inside the window
ANN_Y, ANN_H = 106, 12
ANN = [('BAT', 48), ('USER', 76), ('G', 112), ('RAD', 121), ('SHIFT', 150),
       ('0', 190), ('1', 199), ('2', 208), ('3', 217), ('4', 226), ('PRGM', 244), ('ALPHA', 282)]
ANN_W = {'BAT': 22, 'USER': 30, 'G': 8, 'RAD': 24, 'SHIFT': 34, '0': 8, '1': 8, '2': 8, '3': 8, '4': 8, 'PRGM': 32, 'ALPHA': 36}

# ---- keys: (code, x, y, w, h, main, shifted, alpha, style) ----
keys = []
TOP_Y = 150
for code, x, lab in [(0x18, 30, 'ON'), (0xc6, 92, 'USER'), (0xc5, 218, 'PRGM'), (0xc4, 280, 'ALPHA')]:
    keys.append((code, x, TOP_Y, 50, 22, lab, '', '', 'top'))

COLX = [30 + i * 62.5 for i in range(5)]
ROWY = [222 + i * 60 for i in range(8)]
KW, KH = 50, 32
rows5 = [
    [(0x10, 'Σ+', 'Σ−', 'A'), (0x30, '1/x', 'yˣ', 'B'), (0x70, '√x', 'x²', 'C'), (0x80, 'LOG', '10ˣ', 'D'), (0xc0, 'LN', 'eˣ', 'E')],
    [(0x11, 'x≷y', 'CLΣ', 'F'), (0x31, 'R↓', '%', 'G'), (0x71, 'SIN', 'SIN⁻¹', 'H'), (0x81, 'COS', 'COS⁻¹', 'I'), (0xc1, 'TAN', 'TAN⁻¹', 'J')],
    [(0x12, '', '', ''), (0x32, 'XEQ', 'ASN', 'K'), (0x72, 'STO', 'LBL', 'L'), (0x82, 'RCL', 'GTO', 'M'), (0xc2, 'SST', 'BST', '')],
]
for r, row in enumerate(rows5):
    for c, (code, main, sh, al) in enumerate(row):
        keys.append((code, COLX[c], ROWY[r], KW, KH, main, sh, al, 'shift' if code == 0x12 else 'fn'))
# ENTER row
keys.append((0x13, COLX[0], ROWY[3], COLX[1] + KW - COLX[0], KH, 'ENTER ↑', 'CATALOG', 'N', 'fn'))
for c, (code, main, sh, al) in zip((2, 3, 4), [(0x73, 'CHS', 'ISG', 'O'), (0x83, 'EEX', 'RTN', 'P'), (0xc3, '←', 'CLX/A', '')]):
    keys.append((code, COLX[c], ROWY[3], KW, KH, main, sh, al, 'fn'))
# number pad: operator column then three wide digit keys
OPW, DW = 44, 64
DX = [106, 186, 266]
pad = [
    [(0x14, '−', 'x=y?', 'Q'), (0x34, '7', 'SF', 'R'), (0x74, '8', 'CF', 'S'), (0x84, '9', 'FS?', 'T')],
    [(0x15, '+', 'x≤y?', 'U'), (0x35, '4', 'BEEP', 'V'), (0x75, '5', 'P→R', 'W'), (0x85, '6', 'R→P', 'X')],
    [(0x16, '×', 'x>y?', 'Y'), (0x36, '1', 'FIX', 'Z'), (0x76, '2', 'SCI', '='), (0x86, '3', 'ENG', '?')],
    [(0x17, '÷', 'x=0?', ':'), (0x37, '0', 'π', 'SPACE'), (0x77, '.', 'LASTx', ','), (0x87, 'R/S', 'VIEW', '')],
]
for r, row in enumerate(pad):
    y = ROWY[4 + r]
    for i, (code, main, sh, al) in enumerate(row):
        if i == 0:
            keys.append((code, COLX[0], y, OPW, KH, main, sh, al, 'op'))
        else:
            keys.append((code, DX[i - 1], y, DW, KH, main, sh, al, 'num'))

s = []
a = s.append
a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{SHEET_H}" viewBox="0 0 {W} {SHEET_H}">')
a('''<defs>
<linearGradient id="body" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#3a3a3d"/><stop offset="1" stop-color="#232325"/></linearGradient>
<linearGradient id="kfn" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#55565a"/><stop offset="0.55" stop-color="#3b3c40"/><stop offset="1" stop-color="#2c2d30"/></linearGradient>
<linearGradient id="knum" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#5c5d61"/><stop offset="0.55" stop-color="#424347"/><stop offset="1" stop-color="#313236"/></linearGradient>
<linearGradient id="kshift" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#f0b955"/><stop offset="0.55" stop-color="#dc9a2e"/><stop offset="1" stop-color="#b87a1c"/></linearGradient>
<linearGradient id="ktop" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#4a4b4f"/><stop offset="1" stop-color="#2e2f32"/></linearGradient>
<linearGradient id="bezel" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#101011"/><stop offset="1" stop-color="#2a2a2c"/></linearGradient>
</defs>''')
a(f'<rect x="0" y="0" width="{W}" height="{SHEET_H}" fill="#18181a"/>')
a(f'<rect x="3" y="3" width="{W-6}" height="{H-6}" rx="16" fill="url(#body)" stroke="#0c0c0d" stroke-width="2"/>')
# display surround
a(f'<rect x="18" y="26" width="{W-36}" height="114" rx="8" fill="#1a1a1c" stroke="#4a4a4d" stroke-width="1"/>')
a(f'<rect x="{LCD[0]-5}" y="{LCD[1]-5}" width="{LCD[2]+10}" height="{LCD[3]+10}" rx="5" fill="url(#bezel)"/>')
a(f'<rect x="{LCD[0]}" y="{LCD[1]}" width="{LCD[2]}" height="{LCD[3]}" rx="2" fill="{LCD_BG}"/>')
a(f'<text x="{W-24}" y="38" text-anchor="end" font-family="{FONT}" font-size="7.5" fill="#8a8a8e" letter-spacing="1.2">41CX · hp41-mac</text>')
# keyboard panel
a(f'<rect x="14" y="196" width="{W-28}" height="{H-212}" rx="10" fill="#2a2a2d" stroke="#19191b" stroke-width="1"/>')
a(f'<line x1="150" y1="{TOP_Y+11}" x2="210" y2="{TOP_Y+11}" stroke="#4a4a4d" stroke-width="1"/>')

def key(code, x, y, w, h, main, sh, al, style):
    fill = {'fn': 'url(#kfn)', 'num': 'url(#knum)', 'op': 'url(#kfn)', 'shift': 'url(#kshift)', 'top': 'url(#ktop)'}[style]
    a(f'<rect x="{x}" y="{y+2}" width="{w}" height="{h}" rx="5" fill="#0b0b0c"/>')
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="5" fill="{fill}" stroke="#111" stroke-width="0.8"/>')
    a(f'<rect x="{x+2}" y="{y+1.5}" width="{w-4}" height="{h*0.42}" rx="4" fill="#ffffff" opacity="0.07"/>')
    cx = x + w / 2
    if main:
        fs = 15 if style in ('num', 'op') and len(main) <= 2 else (9.5 if style == 'top' else 11)
        if main == 'R/S': fs = 11
        a(f'<text x="{cx}" y="{y+h/2+fs*0.36}" text-anchor="middle" font-family="{FONT}" font-size="{fs}" font-weight="600" fill="{WHITE}">{e(main)}</text>')
    if sh:
        a(f'<text x="{cx}" y="{y-5}" text-anchor="middle" font-family="{FONT}" font-size="9" font-weight="600" fill="{GOLD}">{e(sh)}</text>')
    if al:
        fs = 7 if len(al) > 1 else 9
        a(f'<text x="{x+w-3}" y="{y+h+10.5}" text-anchor="end" font-family="{FONT}" font-size="{fs}" font-weight="700" fill="{BLUE}">{e(al)}</text>')
for k in keys:
    key(*k)
a('</svg>')
os.makedirs(out, exist_ok=True)
open(os.path.join(out, 'hp41.svg'), 'w').write('\n'.join(s))

with open(os.path.join(out, 'hp41.layout'), 'w') as f:
    f.write('# generated by tools/make_skin.py - 1x sheet pixels\n')
    f.write(f'face {W} {H}\n')
    f.write('lcd %d %d %d %d\n' % LCD)
    f.write('cells %d %d %d %d\n' % CELLS)
    f.write(f'lcdcolors {LCD_BG[1:]} {LCD_FG[1:]}\n')
    f.write(f'annrow {CELLS[0]} {ANN_Y} {CELLS[2]} {ANN_H}\n')
    for (code, x, y, w, h, main, sh, al, style) in keys:
        f.write('key %02x %g %g %g %g\n' % (code, x, y, w, h))
print(W, SHEET_H)
