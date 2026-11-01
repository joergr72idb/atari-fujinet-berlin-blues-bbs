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
| 1 | Terminal layer (ASCII/ATASCII/PETSCII, line editor, paging) | |
| 2 | Users + login, call counters ("Anrufer Nr.", per-user calls, System Status) | |
| 3 | Boards (read/write, new messages) | |
| 4 | Personal messages | |
| 5 | Sysop console + chat | |
| 6 | 24/7 hardening, FRITZ!Box port forward + DynDNS, go-live | |

## Make targets

| Command | Does |
|---------|------|
| `make` | build `build/$(PROG).xex` (default `PROG=spike`) |
| `make run` | stop the Altirra NetSIO hub if running, start FujiNet-PC (SD = `sd/`), atari800 `-netsio -run` |
| `make stop` | stop FujiNet-PC |
| `tools/call.py '<text' '>keys\r' …` | scripted telnet call to localhost:6502 (expect/send steps) |

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
