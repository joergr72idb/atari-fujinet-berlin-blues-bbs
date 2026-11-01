#!/bin/sh
# Run all phase tests, each data-dependent one on fresh BBS data (sd/bbs/*.dat etc. are deleted!).
cd "$(dirname "$0")/.."
make -s test || exit 1
fresh() {
  rm -f sd/bbs/users.dat sd/bbs/stats.dat sd/bbs/lastread.dat sd/bbs/b*.idx sd/bbs/b*.dat sd/bbs/p*.idx sd/bbs/p*.dat sd/bbs/testkeys.*
  printf 'geheim1\n' > sd/bbs/sysop.txt
  tools/emu.sh "$1" 900 >/dev/null || exit 1
}
rc=0
fresh build/bbs-test.xex                  # phase 5 presses sysop keys: test build
if (cd tools && python3 test_phase5.py > ../build/test_phase5.log 2>&1); then echo "phase 5: OK"; else echo "phase 5: FAIL (build/test_phase5.log)"; rc=1; fi
for t in 4 3 2; do
  fresh build/bbs.xex
  if (cd tools && python3 test_phase$t.py > ../build/test_phase$t.log 2>&1); then echo "phase $t: OK"; else echo "phase $t: FAIL (build/test_phase$t.log)"; rc=1; fi
done
sleep 3
if (cd tools && python3 test_phase1.py > ../build/test_phase1.log 2>&1); then echo "phase 1: OK"; else echo "phase 1: FAIL (build/test_phase1.log)"; rc=1; fi
pkill -x atari800
exit $rc
