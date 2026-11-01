#!/bin/sh
# Run all phase tests on the test instance (port 6503, live BBS untouched), each data-dependent one on fresh test data in build/testsd (sd/ is not touched).
cd "$(dirname "$0")/.."
make -s test || exit 1
fresh() {
  T=build/testsd/bbs; mkdir -p $T
  rm -f $T/users.dat $T/stats.dat $T/lastread.dat $T/b*.idx $T/b*.dat $T/p*.idx $T/p*.dat $T/testkeys.*
  printf 'geheim1\n' > $T/sysop.txt
  tools/emu.sh "$1" 900 >/dev/null || exit 1
}
rc=0
fresh build/test/bbs-test.xex             # phase 5 presses sysop keys: test build
if (cd tools && python3 test_phase5.py > ../build/test_phase5.log 2>&1); then echo "phase 5: OK"; else echo "phase 5: FAIL (build/test_phase5.log)"; rc=1; fi
for t in 4 3 2; do
  fresh build/test/bbs.xex
  if (cd tools && python3 test_phase$t.py > ../build/test_phase$t.log 2>&1); then echo "phase $t: OK"; else echo "phase $t: FAIL (build/test_phase$t.log)"; rc=1; fi
done
sleep 3
if (cd tools && python3 test_phase1.py > ../build/test_phase1.log 2>&1); then echo "phase 1: OK"; else echo "phase 1: FAIL (build/test_phase1.log)"; rc=1; fi
tools/inst.sh test stop
exit $rc
