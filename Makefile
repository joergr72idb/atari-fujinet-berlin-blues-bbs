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

.PHONY: all test run reset boot fujinet nohub stop sd clean

all: $(XEX)

BBS_SRC  := src/bbs.c src/net.c src/term.c src/sound.c src/store.c src/user.c src/menu.c src/msg.c src/board.c src/pm.c src/sysop.c src/kbd.s

build/bbs.xex: $(BBS_SRC) src/*.h | build
	$(CL65) $(CFLAGS) -m build/bbs.map -Ln build/bbs.lbl -Wl --dbgfile,build/bbs.dbg \
	  -o $@ $(BBS_SRC) $(FNLIBA)

# test builds listen on 6503 (test instance, tools/emu.sh) so tests never collide with the live BBS on 6502;
# bbs-test.xex also reads sysop keys from bbs/testkeys.txt (tools/test_phase5.py)
TEST_PORT := 6503
# short pre-login limits (seconds) so the timeout tests run fast; clock wraps ~90 s after boot
TEST_DEFS := -DBBS_PORT=$(TEST_PORT) -DLOGIN_IDLE=20 -DLOGIN_TOTAL=60 -DTESTWRAP

build/test/bbs.xex: $(BBS_SRC) src/*.h | build
	@mkdir -p build/test
	$(CL65) $(CFLAGS) $(TEST_DEFS) -o $@ $(BBS_SRC) $(FNLIBA)

build/test/bbs-test.xex: $(BBS_SRC) src/*.h | build
	@mkdir -p build/test
	$(CL65) $(CFLAGS) $(TEST_DEFS) -DTESTKEYS -o $@ $(BBS_SRC) $(FNLIBA)

test: build/bbs.xex build/test/bbs.xex build/test/bbs-test.xex

build/spike.xex: src/spike.c | build
	$(CL65) $(CFLAGS) -o $@ $^ $(FNLIBA)

build:
	mkdir -p build

# live FujiNet-PC instance: SD folder = ./sd, NetSIO UDP 9997, web UI 8000 (the test instance is tools/emu.sh)
fujinet:
	@test -n "$$(tools/inst.sh live fujinet)" || ( cd $(FUJINET) && \
	  nohup ./run-fujinet > $(CURDIR)/build/fujinet.log 2>&1 & sleep 2 )

# the Altirra NetSIO hub (atari-dev) also binds UDP 9997, atari800 needs it
nohub:
	@pkill -f "python3 -m [n]etsiohub" && sleep 1 || true

stop:
	@tools/inst.sh live stop

# always a fresh FujiNet-PC: it crashes when restarted with the BBS listener open
run: $(XEX) nohub
	@tools/inst.sh live stop                     # only the live instance; tests keep running
	@ln -sfn ../../sd $(FUJINET)/SD             # your data (tests use build/testsd)
	@$(MAKE) -s fujinet
	$(ATARI800) -config atari800.cfg -no-kbdjoy0 -no-kbdjoy1 -xl -netsio -run $(XEX)   # W/A/S/D, keypad, Ctrl stay keys

# fresh BBS data (users, stats, boards, mailboxes); PW = sysop password for the first start
reset:
	@test -n "$(PW)" || { echo "usage: make reset PW=<sysop password>"; exit 1; }
	rm -f sd/bbs/users.dat sd/bbs/stats.dat sd/bbs/lastread.dat sd/bbs/b*.idx sd/bbs/b*.dat \
	  sd/bbs/p*.idx sd/bbs/p*.dat sd/bbs/testkeys.* sd/bbs/spike.txt
	@printf '%s\n' "$(PW)" > sd/bbs/sysop.txt
	@echo "BBS data reset - SYSOP password for the first start is set"


sd: $(XEX)
	cp $(XEX) sd/$(PROG).xex

clean:
	rm -rf build
