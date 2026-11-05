#!/bin/sh
# Build docs/guide/setup-guide.pdf from setup-guide.html with headless Chrome/Chromium.
cd "$(dirname "$0")" || exit 1
for b in google-chrome chromium chromium-browser; do command -v $b >/dev/null && break; done
$b --headless --disable-gpu --no-pdf-header-footer --print-to-pdf="$PWD/setup-guide.pdf" \
   "file://$PWD/setup-guide.html" 2>/dev/null
ls -l setup-guide.pdf
