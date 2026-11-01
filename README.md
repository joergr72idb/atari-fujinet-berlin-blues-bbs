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
| 2 | Users + login, call counters ("Anrufer Nr.", per-user calls, System Status), **new users need sysop approval before they can write** | ✔ 2026-11-01 |
| 3 | Menu tree from `menus.txt`, boards (read/write/delete, new messages) | ✔ 2026-11-01 |
| 4 | Personal messages (PMsg), "Msg an Sysop" also for not yet approved users | ✔ 2026-11-01 |
| 5 | Sysop console (status line, keys C/K/A/L/Q) + chat, **paging signal when a caller pages the sysop** | ✔ 2026-11-01 |
| 6 | 24/7 hardening, ER605 port forward + DynDNS, go-live | port forward + online test ✔ 2026-11-01, hardening open |

## Try it locally

```sh
make reset PW=meinpasswort   # fresh BBS data, sysop password for the first start
make run                     # FujiNet-PC + atari800 window with the BBS (sysop console)
telnet localhost 6502        # in a second terminal: call the BBS
```

In the emulator window you are the sysop (keys A/C/K/L/Q, see Phase 5). Callers: RETURN, then `1` for PC.
Log in as `SYSOP` with your password, or register a new user and approve it as SYSOP (menu 8 → 9).
FujiNet-PC's web UI: http://localhost:8000. Stop everything: close the window, `make stop`.

## Online (emulator on the dev box)

Running from `make run` on the dev box (LAN `<LAN-IP>`, `eno1` MAC `<MAC>`), reachable from
the internet via the existing DynDNS name, port 6502. Tested from a phone on mobile data 2026-11-01.

- **ER605:** DHCP address reservation MAC → `<LAN-IP>`; NAT virtual server WAN TCP 6502 → `<LAN-IP>:6502`.
  DrayTek Vigor in front as modem (bridge), Telekom native IPv4.
- **Box:** `sudo ufw allow 6502/tcp`.
- **Never forward or open 8000** – FujiNet-PC's web UI listens on all interfaces without login.
- Test from outside the LAN (mobile data); from inside, the DynDNS name depends on NAT loopback.
- Before handing out the address: change the sysop password (menu 3). Until phase 6 hardening is done, watch for
  hangs after dropped calls / a missing listener – restart with `make run`.

## Make targets

| Command | Does |
|---------|------|
| `make` | build `build/bbs.xex` (`PROG=spike` for the phase 0 spike) |
| `make test` | also `build/bbs-test.xex`: test build that reads sysop keys from `bbs/testkeys.txt` – never deploy it |
| `make run` | stop the Altirra hub and old instances, fresh FujiNet-PC (SD = `sd/`), atari800 window with the BBS |
| `make reset PW=…` | delete all BBS data in `sd/bbs/` and set the sysop password for the first start |
| `make stop` | stop FujiNet-PC |
| `tools/emu.sh build/bbs.xex [sec]` | fresh FujiNet-PC on the test SD folder `build/testsd` + headless atari800, waits until 6502 listens (`VIDEO=1` / `AUDIO=1` record to `build/rec%d.*`) |
| `tools/test_all.sh` | **all tests**, each on fresh data in `build/testsd/` (your `sd/` is not touched) – about 20 min |
| `cd tools && python3 test_phase5.py` | sysop console: present/away, paging + chat, kick, local login, quit (`bbs-test.xex`) |
| `cd tools && python3 test_phase4.py` | PMs: new user → sysop, reply, read marker, unknown recipient, delete, guests |
| `cd tools && python3 test_phase1.py` | automated test calls: PC (ASCII/telnet), Atari (ATASCII), C64 (PETSCII) |
| `cd tools && python3 test_phase3.py` | menus, chains, boards: write, new, read, approval rule, delete (needs fresh data) |
| `cd tools && python3 test_phase2.py` | register, login, approval, guest, password change, counters (needs fresh `users.dat`/`stats.dat` and `sysop.txt` = `geheim1`) |
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

## Users + login (Phase 2)

**First start:** put the initial sysop password into `bbs/sysop.txt` on the SD card (one line). On first start
the BBS creates `bbs/users.dat` with user `SYSOP`, then **deletes `sysop.txt`**. Without the file the
password is `blues` (warning on the sysop screen) – change it right away (menu 3).

- Login like 1987: "Gaeste bitte <RETURN> druecken!" / "Username ==>" / "Passwort ==>" (3 tries, then hang-up).
  Names are case-insensitive, passwords case-sensitive.
- Unknown name → "Neu eintragen (J/N)?" → password twice (min. 4) with a hint not to reuse passwords
  (telnet is unencrypted) → level 1 **NEU**: can read, can't write until the sysop approves (menu 9 → level 2).
- Levels: 0 Gast, 1 Neu, 2 Mitglied, 5 Sysop.
- **Call counters:** `stats.dat` counts every call that gets past the terminal selection ("Du bist Anrufer
  Nr. N", calls today, last caller); per user: calls + last call date in `users.dat`.
- Status screens like 1987 ("Dein Status", "Anrufe : 003", "Letzter Anruf", "System Status"); logoff shows
  time and time online. Date/time from the FujiNet clock (timezone CET/CEST).

Data formats (little endian):

| File | Content |
|------|---------|
| `users.dat` | 32-byte records, user no. = index + 1: name[16], pw hash u32 (FNV-1a of upper-case name + `:` + password), calls u16, level u8, flags u8, last call D,M,Y,h,m, PMs read u16, 1 spare |
| `stats.dat` | total calls u32, calls today u16, day D,M,Y, last caller[16] |

## Menus + boards (Phase 3)

**Menus** come from `bbs/menus.txt` (read at startup – after editing, restart the BBS). One item per line:
`<menu> <key> <type> <arg> <level> <label>` – type `M` submenu, `B` board (1–16), `T` text file in
`bbs/texts/`, `F` function (`status users passwd approve new chat`); level = minimum user level.
Menu 0 is the Hauptmenue; a submenu's title is the label of the item that leads to it.

Boards (2026-11-01): 1 Termine & Treffen · Schwarzes Brett: 2 Suche, 3 Biete, 4 Allgemeines ·
Computer Box: 5 Atari XL/XE, 6 C64, 7 Schneider CPC, 8 Hardware & Loeten · 9 Kultur Brett · 10 Plauderecke.

Navigation like 1987: prompt shows the time online (`0:01:01 =>`); digits + RETURN; chains like `*42`
(Hauptmenue → Computer Box → C64) or `*24` (Termine → write); `*` = Hauptmenue, RETURN = back, `0` = logoff.
In a board: 1 new, 2 all, 3 by number, 4 write (subject + up to 40 lines, `.` alone ends), 5 delete (sysop).
After login: "Neue Msg: Termine & Treffen (1), Commodore 64 (1)"; Hauptmenue 7 reads all new messages.
Writing needs level 2 (approved member); guests and new users only read.

| File | Content |
|------|---------|
| `pNNN.idx/.dat` | personal mailbox of user NNN, same format as boards |
| `bNN.idx` | 48-byte headers, msg no. = index + 1: offset u32, len u16, from[16], date D,M,Y,h,m, flags u8 (1 = deleted), subject[20] |
| `bNN.dat` | message texts back to back, lines end with LF |
| `lastread.dat` | per user 16 × u16: highest message read per board (offset = (user no. − 1) × 32) |
| `menus.txt`, `texts/*.txt` | menu tree and info texts, plain text |

Limits: 40 lines per message; `store_size()` is 16 bit, so an `.idx` holds up to 1365 messages per board.

## Personal messages (Phase 4)

- Login: "Es ist keine Post fuer Dich da" or "Du hast 2 neue PMsg! (*82)".
- Menu 8 PMsg/Userverwaltung: 1 Pers. Briefe schreiben (`An ==>` name, case-insensitive), 2 Pers. Briefe lesen
  (list with `*` = new, number = read, `D`+number = delete, then "Antworten (J/N)?" with subject "Re: …"),
  3 Liste der Benutzer, 4 Passwort aendern, 9 Freischalten (sysop).
- Hauptmenue 9 "Msg an Sysop": a letter to user 1 (SYSOP) – also for **new, not yet approved users**
  (e.g. "bitte freischalten"); everything else needs level 2. Guests have no mailbox.
- Storage: mailbox `pNNN.idx/.dat` per user (shared code with the boards in `msg.c`); "read up to" marker in
  the user record. Lists cut subjects to 18 characters to fit 40 columns.

## Sysop console (Phase 5)

The Atari screen has a fixed **status line** (own display list: 1 status line + 23 scrolling lines of the OS
screen editor, which mirrors the session):
`BLUES warte  Anrufe 123  Sysop weg  L A Q` while idle, `BLUES Joerg L2 0:03:12 Sysop da` during a call.

| Key | When | Does |
|-----|------|------|
| **A** | always | sysop present/away ("Sysop ist: anwesend" in the status screen) |
| **C** | during a call / when paged | chat with the caller, ESC ends it |
| **K** | during a call | kick: "Der Sysop hat die Verbindung beendet." |
| **L** | idle | local login: the BBS on the Atari's own keyboard and screen (not counted as a call) |
| **Q** | always | shut down cleanly (ends a call, closes the listener) |

Callers: Hauptmenue 9 "Msg an Sysop/Chat" – sysop away → straight to a letter; present →
"Sysop zum Chat rufen (J/N)?": the Atari plays the **paging signal** (rising triple beep, different from the
double ring of an incoming call) for up to 30 s; C answers, otherwise "Keine Antwort." and a letter.

Memory (2026-11-01): code + data up to `$905B`, 2 KB stack, about 9 KB free below the screen.
