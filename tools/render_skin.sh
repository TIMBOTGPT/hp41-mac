#!/bin/zsh
# Render skin/hp41.svg to skin/hp41.png at 2x using headless Chrome.
cd "$(dirname "$0")/.." || exit 1
read W H <<< "$(python3 tools/make_skin.py)" || exit 1
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless --disable-gpu --hide-scrollbars \
  --force-device-scale-factor=2 --window-size=$W,$((H+100)) --screenshot=/tmp/hp41_skin_raw.png \
  "file://$PWD/skin/hp41.svg" >/dev/null 2>&1
python3 - "$W" "$H" <<'PY'
import sys
from PIL import Image
w, h = int(sys.argv[1]), int(sys.argv[2])
Image.open('/tmp/hp41_skin_raw.png').convert('RGB').crop((0, 0, w * 2, h * 2)).save('skin/hp41.png')
print('wrote skin/hp41.png', w * 2, h * 2)
PY
