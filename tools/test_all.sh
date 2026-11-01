#!/bin/sh
# Run all phase tests, each data-dependent one on fresh BBS data (sd/bbs/*.dat etc. are deleted!).
cd "$(dirname "$0")/.."
make -s || exit 1
fresh() {
  rm -f sd/bbs/users.dat sd/bbs/stats.dat sd/bbs/lastread.dat sd/bbs/b*.idx sd/bbs/b*.dat sd/bbs/p*.idx sd/bbs/p*.dat
  printf 'geheim1\n' > sd/bbs/sysop.txt
  tools/emu.sh build/bbs.xex 900 >/dev/null || exit 1
}
rc=0
for t in 4 3 2; do
  fresh
  if (cd tools && python3 test_phase$t.py > ../build/test_phase$t.log 2>&1); then echo "phase $t: OK"; else echo "phase $t: FAIL (build/test_phase$t.log)"; rc=1; fi
done
sleep 3
if (cd tools && python3 test_phase1.py > ../build/test_phase1.log 2>&1); then echo "phase 1: OK"; else echo "phase 1: FAIL (build/test_phase1.log)"; rc=1; fi
pkill -x atari800
exit $rc
