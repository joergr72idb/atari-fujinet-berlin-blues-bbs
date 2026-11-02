#!/bin/sh
# Find or stop one emulator instance without touching the other:
#   live = `make run` (FujiNet-PC in opt/fujinet-pc, atari800 on NetSIO 9997, BBS port 6502)
#   test = tools/emu.sh (FujiNet-PC in build/fujinet-test, atari800 on NetSIO 9998, BBS port 6503)
# usage: tools/inst.sh live|test fujinet|atari800|stop   (stop also ends the run-fujinet loop)
cd "$(dirname "$0")/.."
case "$1" in
  live) dir=$PWD/opt/fujinet-pc ;;
  test) dir=$PWD/build/fujinet-test ;;
  *) echo "usage: $0 live|test fujinet|atari800|stop" >&2; exit 2 ;;
esac
fn() { for p in $(pgrep -x fujinet); do [ "$(readlink /proc/$p/cwd)" = "$dir" ] && echo $p; done; }
# the tools/run-fujinet loop around it (stopped first, else it would restart FujiNet-PC)
loop() { for p in $(pgrep -f '[r]un-fujinet'); do [ "$(readlink /proc/$p/cwd)" = "$dir" ] && echo $p; done; }
a8() {
  for p in $(pgrep -x atari800); do
    if tr '\0' ' ' < /proc/$p/cmdline | grep -q -- '-netsio 9998'; then t=test; else t=live; fi
    [ $t = "$1" ] && echo $p
  done
}
case "$2" in
  fujinet) fn ;;
  atari800) a8 "$1" ;;
  stop)
    l=$(loop); [ -n "$l" ] && kill $l 2>/dev/null
    pids="$(fn) $(a8 "$1")"
    [ -n "${pids# }" ] || exit 0
    kill $pids 2>/dev/null
    for i in $(seq 1 30); do
      alive=; for p in $pids; do kill -0 $p 2>/dev/null && alive=1; done
      [ -z "$alive" ] && exit 0; sleep 0.5
    done
    kill -9 $pids 2>/dev/null; exit 0 ;;
  *) echo "usage: $0 live|test fujinet|atari800|stop" >&2; exit 2 ;;
esac
