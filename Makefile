# Berlin Blues BBS - uses the toolchain from ../atari-dev (cc65, fujinet-lib, atari800, Altirra)

DEV      ?= $(abspath ../atari-dev)
CL65     := $(DEV)/opt/cc65/bin/cl65
FNLIB    := $(DEV)/opt/fujinet-lib
FNLIBA   := $(wildcard $(FNLIB)/fujinet-atari-*.lib)
ATARI800 := $(DEV)/opt/atari800/bin/atari800
FUJINET  := $(CURDIR)/opt/fujinet-pc
export CC65_HOME := $(DEV)/opt/cc65/share/cc65

PROG     ?= bbs
XEX      := build/$(PROG).xex
CFLAGS   := -t atari -O -I $(FNLIB) -g

.PHONY: all test run boot fujinet nohub stop sd clean

all: $(XEX)

BBS_SRC  := src/bbs.c src/net.c src/term.c src/sound.c src/store.c src/user.c src/menu.c src/msg.c src/board.c src/pm.c src/sysop.c

build/bbs.xex: $(BBS_SRC) src/*.h | build
	$(CL65) $(CFLAGS) -m build/bbs.map -Ln build/bbs.lbl -Wl --dbgfile,build/bbs.dbg \
	  -o $@ $(BBS_SRC) $(FNLIBA)

# test build: sysop keys can also come from bbs/testkeys.txt (tools/test_phase5.py)
build/bbs-test.xex: $(BBS_SRC) src/*.h | build
	$(CL65) $(CFLAGS) -DTESTKEYS -o $@ $(BBS_SRC) $(FNLIBA)

test: build/bbs.xex build/bbs-test.xex

build/spike.xex: src/spike.c | build
	$(CL65) $(CFLAGS) -o $@ $^ $(FNLIBA)

build:
	mkdir -p build

# own FujiNet-PC instance: SD folder = ./sd, NetSIO UDP 9997
fujinet:
	@pgrep -x fujinet >/dev/null || ( cd $(FUJINET) && \
	  nohup ./run-fujinet > $(CURDIR)/build/fujinet.log 2>&1 & sleep 2 )

# the Altirra NetSIO hub (atari-dev) also binds UDP 9997, atari800 needs it
nohub:
	@pkill -f "python3 -m [n]etsiohub" && sleep 1 || true

stop:
	-pkill -x fujinet

run: $(XEX) nohub fujinet
	$(ATARI800) -config atari800.cfg -xl -netsio -run $(XEX)

sd: $(XEX)
	cp $(XEX) sd/$(PROG).xex

clean:
	rm -rf build
