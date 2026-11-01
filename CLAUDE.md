# CLAUDE.md

Berlin Blues BBS for Atari XL + FujiNet (telnet port 6502). Plan, phases and FujiNet findings: README.md.
Toolchain/dev loop: ../atari-dev (README + CLAUDE.md there).

- Strings sent to callers: CR/LF as `"\015\012"`, never `\n` (cc65 atari maps it to $9B).
- SD files: `N2:SD:///bbs/...` (three slashes). N1 = listener, N2 = files.
- Headless test: start FujiNet-PC (`make nohub fujinet`), atari800 with SDL dummy drivers, `< /dev/null`, in the
  background; wait for `ss -ltn | grep :6502`; then `tools/call.py`.
- Don't `pgrep -f`/`pkill -f` with a pattern that appears in your own command line (use `[x]yz` trick).
