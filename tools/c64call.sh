#!/bin/sh
# Call the BBS from an emulated C64: VICE (x64sc) with the userport RS232 "modem" wired straight to a TCP
# address, terminal program ccgmsterm v0.2 (https://github.com/mist64/ccgmsterm, build/c64/dl/ccgms.prg).
#   tools/c64call.sh [host:port]      default 127.0.0.1:6502 = live BBS (test instance: 127.0.0.1:6503)
# In ccgmsterm: F7 (config) -> modem type "User Port", baud 2400, then back to the terminal; at the BBS:
# RETURN, then 3 (C64). VICE dials as soon as ccgmsterm starts: the BBS's 60 s pre-login limit runs from
# then on - press RETURN right away. VICE doesn't redial after a hang-up: restart the script.
# Own VICE config in build/c64/vicerc - your ~/.config/vice stays untouched.
cd "$(dirname "$0")/.."
TARGET=${1:-127.0.0.1:6502}
PRG=build/c64/dl/ccgms.prg
[ -f $PRG ] || { mkdir -p build/c64/dl && gh release download v0.2 -R mist64/ccgmsterm -p ccgms.prg -D build/c64/dl; } || exit 1
mkdir -p build/c64
exec x64sc -config build/c64/vicerc \
  -userportdevice 2 -rsuserdev 0 -rsuserbaud 2400 \
  -rsdev1 "$TARGET" +rsdev1ip232 -rsdev1baud 2400 \
  -autostart $PRG
