#!/bin/sh
# Snapshot of the live BBS data (sd/bbs) into backup/bbs-YYYY-MM-DD_HHMM.tar.gz, keeps the newest KEEP (30).
# Read-only on sd/ - safe while the BBS runs. Restore: stop the BBS (make stop), then
#   tar -xzf backup/bbs-<date>.tar.gz -C sd
# Hourly via cron:  0 * * * * /home/joerg/claude/blues-bbs/tools/backup.sh
# backup/ holds password hashes: git-ignored, never commit or upload it.
cd "$(dirname "$0")/.." || exit 1
KEEP=${KEEP:-30}
mkdir -p backup
name=backup/bbs-$(date +%F_%H%M).tar.gz
# skip if nothing changed since the last snapshot
last=$(ls -1 backup/bbs-*.tar.gz 2>/dev/null | tail -1)
if [ -n "$last" ] && [ -z "$(find sd/bbs -type f -newer "$last" | head -1)" ]; then
  exit 0
fi
tar -czf "$name.tmp" -C sd bbs && mv "$name.tmp" "$name" || { rm -f "$name.tmp"; echo "backup failed" >&2; exit 1; }
ls -1 backup/bbs-*.tar.gz | head -n -"$KEEP" | xargs -r rm -f
