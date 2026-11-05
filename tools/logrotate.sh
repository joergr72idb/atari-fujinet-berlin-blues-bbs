#!/bin/sh
# Rotate the FujiNet-PC logs (build/fujinet.log ~15 MB/h of SIO debug lines, build/fujinet-test.log).
# Uses the system logrotate with our own config + state in build/ - no root needed.
# copytruncate: FujiNet-PC keeps its file open (opened with >> = O_APPEND by make run), so the live
# BBS never has to be restarted. Lines written between copy and truncate may be lost - fine for a debug log.
# Hourly via cron:  5 * * * * /home/joerg/claude/blues-bbs/tools/logrotate.sh
# Settings: SIZE (default 100M), KEEP (default 10 compressed old logs per file).
cd "$(dirname "$0")/.." || exit 1
SIZE=${SIZE:-100M}
KEEP=${KEEP:-10}
dir=$(pwd)/build
conf=$dir/logrotate.conf
cat > "$conf" <<EOF
$dir/fujinet.log $dir/fujinet-test.log {
    size $SIZE
    rotate $KEEP
    copytruncate
    compress
    missingok
    notifempty
    dateext
    dateformat -%Y-%m-%d_%H%M
}
EOF
exec /usr/sbin/logrotate -s "$dir/logrotate.state" "$conf" "$@"
