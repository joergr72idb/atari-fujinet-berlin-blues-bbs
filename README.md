# Berlin Blues BBS

Single-line BBS for a real **Atari XL + FujiNet**, callers dial in via **telnet on port 6502**.
A revival of the 1987 Berlin-Blues-Box (HICoF + ACC, Celle) as the communication hub for our
retro-computing meetup group. Template for look and menus: the 1987 session transcript in
`acc-pokey/articles/pokey_nr5/ONLINE.txt`.

- C with cc65 (`-t atari`) + fujinet-lib, toolchain from `../atari-dev` (see its README).
- Data on the FujiNet SD card under `/bbs/` (here: `sd/bbs/`, served by our own FujiNet-PC in `opt/fujinet-pc`).
- Callers: PC/Mac/phone telnet (ASCII), Atari 8-bit (ATASCII), C64 (PETSCII), Schneider CPC (ASCII).

## Phases

| # | Phase | Status |
|---|-------|--------|
| 0 | Spike: TCP server on 6502, IAC filter, N:SD: write/append/seek | ✔ 2026-11-01 |
| 1 | Terminal layer (ASCII/ATASCII/PETSCII, line editor, paging), ring sound on incoming call | ✔ 2026-11-01 |
| 2 | Users + login, call counters ("Anrufer Nr.", per-user calls, System Status), **new users need sysop approval before they can write** | |
| 3 | Boards (read/write, new messages) | |
| 4 | Personal messages | |
| 5 | Sysop console + chat, **sound signal when a caller pages the sysop** | |
| 6 | 24/7 hardening, FRITZ!Box port forward + DynDNS, go-live | |

## Make targets

| Command | Does |
|---------|------|
| `make` | build `build/bbs.xex` (`PROG=spike` for the phase 0 spike) |
| `make run` | stop the Altirra NetSIO hub if running, start FujiNet-PC (SD = `sd/`), atari800 `-netsio -run` |
| `make stop` | stop FujiNet-PC |
| `tools/emu.sh build/bbs.xex [sec]` | fresh FujiNet-PC + headless atari800, waits until 6502 listens (`VIDEO=1` / `AUDIO=1` record to `build/rec%d.*`) |
| `cd tools && python3 test_phase1.py` | automated test calls: PC (ASCII/telnet), Atari (ATASCII), C64 (PETSCII) |
| `tools/call.py '<text' '>keys\r' …` | ad-hoc scripted telnet call (expect/send steps) |

## Findings (Phase 0)

- **Listen:** `network_open("N1:TCP://:6502/", OPEN_MODE_RW, 0)`, poll `network_status()` until
  `connected == 1`, `network_accept()`; caller gone ⇒ `connected == 0`; `network_close_client()`
  keeps the listener for the next call. Two calls in a row tested.
- **Second caller:** the firmware refuses a 2nd accept while one is connected; the 2nd caller's
  TCP connect succeeds but waits until the line is free – no "busy" message possible.
- **SD path:** use `N2:SD:///bbs/file` (empty host). `SD://bbs/file` treats `bbs` as host and
  writes to the SD root; `SD:/bbs/file` is not parsed as a URL.
- **Modes:** write (`wb`), append (`OPEN_MODE_APPEND`, `ab`), read + `network_seek` work.
- **Line ends:** cc65 (atari) turns `\n` in strings into ATASCII EOL `$9B`. For telnet use octal
  escapes `"\015\012"` (not `\x0d\x0a…` – a following hex letter is eaten into the escape).
- **Port 9997:** the Altirra NetSIO hub from `atari-dev` also binds UDP 9997; while it runs,
  atari800 can't talk to FujiNet-PC (every call returns error 1). `make run` stops it first.
- **Background emulator:** start atari800 with `< /dev/null` (else it stops on terminal input).
- Open: in one of five runs the listener didn't appear within 80 s – watch for this in phase 6.

## Terminal layer (Phase 1)

- Caller connects → ring on the Atari (POKEY double ring) → "BITTE <RETURN> DRUECKEN" (upper case only,
  readable everywhere). `$9B` = Atari (ATASCII, no question); CR/LF → "1=PC/MAC 2=SCHNEIDER CPC 3=C64".
- Inside the BBS text is ASCII with `'\n'` (= `$9B`) as line end; `term.c` converts per caller:
  PC/CPC CR LF, C64 CR + PETSCII case swap (`$0E` lower-case charset first), Atari unchanged.
- Input: Enter = CR/LF/`$9B`/CR(C64); backspace = BS/DEL, `$7E` (Atari), `$14` (C64).
- Telnet `IAC` filtering and `WILL ECHO`/`WILL SGA` only for PC/Mac callers (retro terminals would show garbage).
- Paging `--Mehr-- (RETURN/Q)` at 23 (C64: 24) lines; width 80 (PC) or 40; idle timeout 5 min.
- Sysop screen mirrors the session via the OS screen editor `E:` (stdio) – cc65 `conio` doesn't scroll on the Atari.

## Findings (Phase 1)

- **FujiNet-PC crashes** (`free(): chunks in smallbin corrupted`, exit 134) when it shuts down or reboots while
  our TCP listener is open (`fnTcpServer::stop`). The emulator's cold start triggers such a reboot, so
  `tools/emu.sh` always starts a fresh FujiNet-PC and waits until the old one is really gone. This also explains
  the phase 0 "listener didn't appear" run. Real FujiNet (ESP32) reboots by hardware reset – recheck on hardware.
- **SIO noise:** with `SOUNDR` ($41) ≠ 0 every SIO access beeps – with FujiNet polled constantly, a real XL would
  buzz 24/7. `main()` sets it to 0 (verified in the audio recording: ring audible, noise gone).
