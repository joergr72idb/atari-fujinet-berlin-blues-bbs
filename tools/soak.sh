#!/bin/sh
# Soak test (phase 6 item 4) on the TEST instance: fresh test data, test instance, tools/soak.py.
#   tools/soak.sh [hours]   (default 4)   -> build/soak.log; live BBS and sd/ untouched
cd "$(dirname "$0")/.."
H=${1:-4}
make -s test || exit 1
T=build/testsd/bbs; mkdir -p $T
rm -f $T/users.dat $T/stats.dat $T/lastread.dat $T/b*.idx $T/b*.dat $T/p*.idx $T/p*.dat $T/testkeys.* $T/bbs.cfg
printf 'geheim1\n' > $T/sysop.txt
secs=$(python3 -c "print(int($H * 3600 + 1800))")
tools/emu.sh build/test/bbs.xex "$secs" >/dev/null || exit 1
(cd tools && python3 -u soak.py "$H") 2>&1 | tee build/soak.log
grep -q ", 0 failed, 0 data problems" build/soak.log; rc=$?
tools/inst.sh test stop
exit $rc
