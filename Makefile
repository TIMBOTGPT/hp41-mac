# hp41-mac headless build.
# The V41 emulation core is compiled unmodified from an upstream checkout
# (not stored in this repository). See docs/emulator-selection.md.
UPSTREAM ?= $(HOME)/Projects/hp41-upstream/v41r9o/src/V41R9
V41 := $(UPSTREAM)/V41
GEN := build/gen
CXX := clang++
CXXFLAGS := -std=c++17 -O2 -arch arm64 -fms-extensions -w -include src/shim/StdAfx.h -I$(GEN) -Isrc/shim
CORE := HP41Kernel HP41Display HP41Timer HP41Hpil HP41File HP41Trace HP41Printer HP41Infrared WFile
GENSRC := HP41GraphicsText MODFile
HDRS := HP41.h HP41Hpil.h HP41HpilIf.h WFile.h Tcpip.h Udp.h
OBJS := $(CORE:%=build/%.o) $(GENSRC:%=build/%.o) build/shim.o build/stubs.o build/tcpip.o build/main.o

all: build/hp41-headless build/hp41 build/hp41-ildev build/hp41-gfx

$(GEN)/.stamp:
	@test -d "$(V41)" || { echo "Upstream V41 source not found at $(V41)"; exit 1; }
	mkdir -p $(GEN)
	for f in $(CORE); do cp "$(V41)/$$f.cpp" $(GEN)/; done
	for h in $(HDRS); do cp "$(V41)/$$h" $(GEN)/; done
	cp "$(V41)/Tcpip.h" $(GEN)/TcpIp.h 2>/dev/null || true
	sed -n '1,122p;225,$$p' "$(V41)/HP41Graphics.cpp" > $(GEN)/HP41GraphicsText.cpp
	cp "$(UPSTREAM)/MODFile/MODFile.c" $(GEN)/MODFile.cpp
	sed -E '/^typedef unsigned (char byte|short word|long dword);/d' "$(UPSTREAM)/MODFile/MODFile.h" > $(GEN)/MODFile.h
	touch $@

build/%.o: $(GEN)/.stamp
	$(CXX) $(CXXFLAGS) -c $(GEN)/$*.cpp -o $@
build/shim.o: src/shim/shim.cpp src/shim/StdAfx.h
	mkdir -p build && $(CXX) $(CXXFLAGS) -c $< -o $@
build/stubs.o: src/headless/stubs.cpp $(GEN)/.stamp
	$(CXX) $(CXXFLAGS) -c $< -o $@
build/tcpip.o: src/hpil/tcpip.cpp src/hpil/hpil_log.h $(GEN)/.stamp
	$(CXX) $(CXXFLAGS) -Isrc/hpil -c $< -o $@
build/main.o: src/headless/main.cpp $(GEN)/.stamp
	$(CXX) $(CXXFLAGS) -Isrc/hpil -c $< -o $@
build/hp41-headless: $(OBJS)
	$(CXX) -arch arm64 $(OBJS) -o $@ -lpthread

COREOBJS := $(CORE:%=build/%.o) $(GENSRC:%=build/%.o) build/shim.o build/stubs.o build/tcpip.o
build/app.o: src/app/main.cpp $(GEN)/.stamp
	$(CXX) $(CXXFLAGS) $(shell sdl2-config --cflags) -c $< -o $@
build/hp41: $(COREOBJS) build/app.o
	$(CXX) -arch arm64 $(COREOBJS) build/app.o -o $@ $(shell sdl2-config --libs) -lpthread

# portable HP-IL test device: the core must build as strict C99 with warnings on
build/hp41-ildev: src/ildev/ildev.c src/ildev/ildev.h src/ildev/main.c
	mkdir -p build
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -Wall -Wextra -pedantic -Werror src/ildev/ildev.c src/ildev/main.c -o $@

# graphics peripheral: portable cores as strict C99, Mac wrapper with zlib for PNG
# (window.c uses SDL2 and is built without -pedantic; the cores stay strict)
build/hp41-gfx: src/gfx/gfx.c src/gfx/gfx.h src/gfx/session.c src/gfx/session.h src/gfx/main.c src/gfx/window.c src/gfx/window.h src/gfx/assets.c src/gfx/assets.h src/ildev/ildev.c src/ildev/ildev.h
	mkdir -p build
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -Wall -Wextra -pedantic -Werror -Isrc/ildev -c src/gfx/gfx.c -o build/gfx.o
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -Wall -Wextra -pedantic -Werror -c src/ildev/ildev.c -o build/ildev.o
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -Wall -Wextra -pedantic -Werror -c src/gfx/session.c -o build/gfxsession.o
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -Wall -Wextra -pedantic -Werror -Isrc/ildev -c src/gfx/main.c -o build/gfxmain.o
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -Wall -Wextra -Werror $(shell sdl2-config --cflags) -c src/gfx/window.c -o build/gfxwin.o
	clang -std=c99 -D_DARWIN_C_SOURCE -O2 -arch arm64 -w -c src/gfx/assets.c -o build/gfxassets.o
	clang -arch arm64 build/gfx.o build/gfxsession.o build/ildev.o build/gfxmain.o build/gfxwin.o build/gfxassets.o -o $@ -lz $(shell sdl2-config --libs)

# user-code XROM (needs the focalcod/asmnut toolchain, docs/xrom.md)
xrom: xrom/build/HP41GFX.MOD
xrom/build/HP41GFX.MOD: xrom/src/hp41gfx.focal tools/build_xrom.py
	mkdir -p xrom/build
	python3 tools/build_xrom.py xrom/src/hp41gfx.focal $@ --xrom 8 --header "-HP41GFX 0A" --romid GF-0A \
	  --title "HP41GFX graphics v0" --listing xrom/build/hp41gfx.lst

clean:
	rm -rf build
.PHONY: all clean xrom
