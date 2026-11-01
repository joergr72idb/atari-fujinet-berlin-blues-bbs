# CLAUDE.md

Berlin Blues BBS for Atari XL + FujiNet (telnet port 6502). Plan, phases and FujiNet findings: README.md.
Toolchain/dev loop: ../atari-dev (README + CLAUDE.md there).

- Caller output goes through `term_puts()` with `'\n'` (= $9B) as line end; term.c converts. Raw `net_*` writes
  need CR/LF as `"\015\012"` (cc65 atari maps `\n` to $9B).
- Sysop screen output via stdio (`putchar`/`puts`, E: scrolls), not conio (no scrolling on Atari).
- SD files: `N2:SD:///bbs/...` (three slashes). N1 = listener, N2 = files. All file access goes through
  `store.c`; a sequential read (`store_open/next/close`) blocks N2 until closed.
- Never use `"\n"`/`"\r\n"` to match bytes from files or the network (strcspn etc.) – in cc65 atari `"\n"` is $9B;
  use `"\015\012"`.
- Headless test: `tools/emu.sh build/bbs.xex` (always fresh FujiNet-PC – it crashes on reboot with an open
  listener), then `cd tools && python3 test_phase1.py` / `test_phase2.py` (phase 2 needs fresh data, see README) (module `bbstest.py`: Call.expect/send/page_until, petscii()).
  `VIDEO=1`/`AUDIO=1` record; last frame via ffmpeg `-sseof -0.2`.
- Don't `pgrep -f`/`pkill -f` with a pattern that appears in your own command line (use `[x]yz` trick).
