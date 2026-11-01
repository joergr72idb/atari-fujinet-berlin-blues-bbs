#!/bin/sh
# Start FujiNet-PC + headless atari800 with an XEX, wait until the BBS listens on 6502.
#   tools/emu.sh build/bbs.xex [seconds]      (emulator ends after [seconds], default 300)
cd "$(dirname "$0")/.."
make -s nohub
# Always a fresh FujiNet-PC: it crashes (heap corruption in fnTcpServer::stop) when it
# shuts down or reboots with our TCP listener still open.
pkill -x atari800 2>/dev/null; pkill -x fujinet 2>/dev/null
rm -f build/rec*.avi build/rec*.wav     # atari800 won't overwrite ("All video files exist!")
while pgrep -x atari800 >/dev/null || pgrep -x fujinet >/dev/null; do sleep 1; done
make -s fujinet
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy nohup timeout "${2:-300}" \
  ../atari-dev/opt/atari800/bin/atari800 -config atari800.cfg -xl -netsio -run "$1" ${VIDEO:+-record-video -vname build/rec%d.avi} ${AUDIO:+-record-audio -aname build/rec%d.wav} \
  > build/emu.log 2>&1 < /dev/null &
for i in $(seq 1 60); do
  ss -ltn | grep -q ":6502 " && { echo "BBS listening (${i}x2s)"; exit 0; }
  sleep 2
done
echo "BBS not listening after 120 s"; exit 1
