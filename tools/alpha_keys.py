#!/usr/bin/env python3
"""Turn text into hp41-headless key tokens for typing it in ALPHA mode.
Covers letters, digits, space and the punctuation the graphics protocol
uses. usage: alpha_keys.py "TEXT"   (prints the tokens; caller adds ALPHA)"""
import sys
MAP = {' ': '0', '?': '3', ':': '/', ',': '.', '=': '2', '.': 'SHIFT .', '-': 'SHIFT -',
       '+': 'SHIFT +', '*': 'SHIFT *', '/': 'SHIFT /'}
out = []
for ch in sys.argv[1]:
    if 'A' <= ch <= 'Z': out.append(ch)
    elif ch.isdigit(): out.append('SHIFT ' + ch)
    elif ch in MAP: out.append(MAP[ch])
    else: sys.exit("no key for %r" % ch)
print(' '.join(out))
