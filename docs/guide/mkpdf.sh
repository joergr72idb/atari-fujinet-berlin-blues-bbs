#!/bin/sh
# Build the PDFs in docs/guide/ from their HTML sources with headless Chrome/Chromium.
# Usage: mkpdf.sh [name ...]   e.g. mkpdf.sh calling-guide   (no name = all *.html)
cd "$(dirname "$0")" || exit 1
for b in google-chrome chromium chromium-browser; do command -v $b >/dev/null && break; done
[ $# -eq 0 ] && set -- $(ls *.html | sed 's/\.html$//')
for n in "$@"; do
    $b --headless --disable-gpu --no-pdf-header-footer --print-to-pdf="$PWD/$n.pdf" \
       "file://$PWD/$n.html" 2>/dev/null
    ls -l "$n.pdf"
done
