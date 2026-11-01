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
- Message storage (boards `bNN`, mailboxes `pNNN`) lives in `msg.c`; board.c/pm.c only do UI + read markers.
- Menu digits are relative to the current menu (like 1987); tests must use `*…` chains for absolute paths.
- Sysop keys in tests: build `bbs-test.xex` (`make test`, `-DTESTKEYS`), write keys to `sd/bbs/testkeys.txt`
  (see `keys()` in test_phase5.py). Never deploy the test build.
- Status line: sysop.c display list; any `GRAPHICS`/screen reopen would drop it.
- Tests use their own SD folder `build/testsd` (tools/emu.sh re-points `opt/fujinet-pc/SD`); `make run` points it
  back to `sd/`, which holds the user's own data – never delete or reset `sd/bbs/*` yourself.
- All tests: `tools/test_all.sh` (fresh data each). Single: `tools/emu.sh build/bbs.xex` (always fresh FujiNet-PC – it crashes on reboot with an open
  listener), then `cd tools && python3 test_phase{1,2,3}.py` (2-5 need fresh data in `build/testsd/bbs`: delete `{users,stats,lastread}.dat`,
  `b*`, `p*`, write `sysop.txt` = `geheim1` – test_all.sh does this) (module `bbstest.py`: Call.expect/send/page_until, petscii()).
  `VIDEO=1`/`AUDIO=1` record; last frame via ffmpeg `-sseof -0.2`.
- Don't `pgrep -f`/`pkill -f` with a pattern that appears in your own command line (use `[x]yz` trick).
- atari800 keyboard joysticks (W/A/S/D + left Ctrl, keypad) swallow those keys: `make run` passes `-no-kbdjoy0 -no-kbdjoy1`.
