#!/bin/sh
# Start the TEST instance: own FujiNet-PC (build/fujinet-test, SD = build/testsd, web UI 127.0.0.1:8001,
# NetSIO 9998) + headless atari800 with an XEX, wait until the BBS listens on 6503.
# The live BBS (`make run`: 6502/8000/9997, SD = sd/) keeps running untouched.
#   tools/emu.sh build/test/bbs.xex [seconds]      (emulator ends after [seconds], default 300)
# Test XEXs must be built with BBS_PORT=6503 (`make test` -> build/test/*.xex).
#   ALTIRRA=1 tools/emu.sh build/test/bbs.xex      Altirra (Wine, window) instead of atari800: netsiohub
#                                                  TCP 9996 <-> UDP 9998, so it works next to the live BBS.
#                                                  Stop: close Altirra, then tools/inst.sh test stop (also ends the hub).
cd "$(dirname "$0")/.."
SDDIR=build/testsd
FN=build/fujinet-test
mkdir -p $SDDIR/bbs $FN
cp sd/bbs/menus.txt $SDDIR/bbs/ && cp -r sd/bbs/texts $SDDIR/bbs/
# own FujiNet-PC copy (data/ and binary from opt/fujinet-pc), config with NetSIO (BOIP) port 9998
cp -u opt/fujinet-pc/fujinet tools/run-fujinet $FN/   # tools/run-fujinet restarts it on reboot (75) and crash (134)
[ -d $FN/data ] || cp -r opt/fujinet-pc/data $FN/
[ -f $FN/fnconfig.ini ] || cp opt/fujinet-pc/fnconfig.ini $FN/
sed -i '/^\[BOIP\]/,/^\[/ s/^port=.*/port=9998/' $FN/fnconfig.ini
ln -sfn ../testsd $FN/SD
# Always a fresh FujiNet-PC: it crashes (heap corruption in fnTcpServer::stop) when it
# shuts down or reboots with our TCP listener still open.
tools/inst.sh test stop
rm -f build/rec*.avi build/rec*.wav     # atari800 won't overwrite ("All video files exist!")
(cd $FN && nohup ./run-fujinet -u http://127.0.0.1:8001 > ../fujinet-test.log 2>&1 < /dev/null &)
sleep 2
if [ -n "$ALTIRRA" ]; then
  # Altirra's netsio.atdevice talks TCP 9996 to netsiohub, the hub UDP 9998 to the test FujiNet-PC
  if ss -ltn | grep -q ":9996 "; then echo "TCP 9996 is in use (another netsiohub?) - stop it first"; exit 1; fi
  (cd ../atari-dev/src/fujinet-emulator-bridge/fujinet-bridge && \
   nohup python3 -m netsiohub --netsio-port 9998 > "$OLDPWD/build/hub-test.log" 2>&1 < /dev/null &)
  sleep 2
  case "$1" in /*) x=$1 ;; *) x=$PWD/$1 ;; esac
  nohup ../atari-dev/tools/altirra.sh "$x" > build/emu.log 2>&1 < /dev/null &
else
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy nohup timeout "${2:-300}" \
  ../atari-dev/opt/atari800/bin/atari800 -config atari800.cfg -xl -netsio 9998 -run "$1" ${VIDEO:+-record-video -vname build/rec%d.avi} ${AUDIO:+-record-audio -aname build/rec%d.wav} \
  > build/emu.log 2>&1 < /dev/null &
fi
for i in $(seq 1 60); do
  ss -ltn | grep -q ":6503 " && { echo "BBS listening on 6503 (${i}x2s)"; exit 0; }
  sleep 2
done
echo "BBS not listening on 6503 after 120 s"; exit 1
