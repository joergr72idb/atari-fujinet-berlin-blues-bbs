# Berlin Blues BBS – technical notes

How the BBS is built and why: phases, FujiNet findings, data formats, the 24/7 hardening, tests and make
targets. Setup and customizing: [README](../README.md).

- C with cc65 (`-t atari`) + fujinet-lib, toolchain from `../atari-dev`.
- Data on the FujiNet SD card under `/bbs/` (here: `sd/bbs/`, served by our own FujiNet-PC in `opt/fujinet-pc`).
- Callers: PC/Mac/phone telnet (ASCII), Atari 8-bit (ATASCII), C64 (PETSCII), Schneider CPC (ASCII).
- Template for look and menus: the 1987 session transcript in `acc-pokey/articles/pokey_nr5/ONLINE.txt`.

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

## Languages and texts

All program texts live in **`lang/en.txt`** (English, the master list with every key) and their
translations, e.g. **`lang/de.txt`**. The build compiles the chosen language into the XEX (no extra
memory or SD access on the Atari):

```sh
make BBS_LANG=de            # one build in German
echo 'BBS_LANG = de' > config.mk   # or for good: config.mk is local, not in git (default: en)
```

New language: copy `lang/en.txt` to `lang/xx.txt`, translate the text in quotes, `make BBS_LANG=xx`.
`tools/mklang.py` checks it and stops the build on errors:

- keys that aren't in `en.txt`; missing keys only warn and use English
- placeholders (`%u`, `%lu`, `%s`, …) must match the English text in number and order
- plain ASCII only (umlauts as ae/oe/ue/ss – the callers' machines don't agree on special characters)
- `KEY_*` = one lower-case answer letter (YES, QUIT at --More--, DELETE in the mailbox)
- `RAW_*` = upper case only (shown before the caller picked PC/Atari/C64)
- warning for lines longer than 40 characters (Atari/C64 screen)

Not in the language file, because the sysop edits them on the SD card: the banner
(`bbs/texts/banner.txt`), help texts (`bbs/texts/*.txt`) and menu names (`bbs/menus.txt`).
The tests check the German texts, so test builds are always German (`build/lang/de`).

## Settings (`bbs/bbs.cfg`)

`sd/bbs/bbs.cfg` holds site settings as `key = value` lines (`#` = comment), read once at boot – restart the
BBS (`make run`) after a change. The sysop screen shows the port in use ("Port 6502"); bad lines are reported
there too. Missing file or invalid value → compiled-in default.

| key | default | |
|---|---|---|
| `port` | 6502 (test builds 6503) | TCP port for callers. Router forward and ufw must match. `8000` is refused (FujiNet-PC web UI). |

More settings (idle limits, time zone – `TZ` is still compiled in) can go here later.

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

**Two instances side by side** – tests never touch the live BBS:

| | live (`make run`) | test (`tools/emu.sh`, `tools/test_all.sh`) |
|---|---|---|
| BBS port | 6502 (forwarded) | 6503 (not forwarded) |
| FujiNet-PC | `opt/fujinet-pc`, web UI `0.0.0.0:8000` | `build/fujinet-test`, web UI `127.0.0.1:8001` |
| NetSIO (UDP) | 9997 | 9998 |
| SD folder | `sd/` | `build/testsd/` |
| XEX | `build/bbs.xex` | `build/test/bbs.xex`, `build/test/bbs-test.xex` (built with `BBS_PORT=6503`) |

`tools/inst.sh live|test fujinet|atari800|stop` finds or stops exactly one of them.

## Make targets

| Command | Does |
|---------|------|
| `make` | build `build/bbs.xex` in `BBS_LANG` (config.mk, default `en`; `PROG=spike` for the phase 0 spike) |
| `make test` | also the test builds on port 6503: `build/test/bbs.xex` and `build/test/bbs-test.xex` (reads sysop keys from `bbs/testkeys.txt`) – never deploy them |
| `make run` | stop the Altirra hub and the old live instance, fresh FujiNet-PC (SD = `sd/`), atari800 window with the BBS (test instance keeps running) |
| `make reset PW=…` | delete all BBS data in `sd/bbs/` and set the sysop password for the first start |
| `make stop` | stop the live instance |
| `tools/emu.sh build/test/bbs.xex [sec]` | fresh test instance (FujiNet-PC on `build/testsd`) + headless atari800, waits until 6503 listens (`VIDEO=1` / `AUDIO=1` record to `build/rec%d.*`) |
| `tools/c64call.sh [host:port]` | call the BBS from an emulated C64: VICE + ccgmsterm over the userport RS232 (default live `127.0.0.1:6502`) |
| `tools/soak.sh [hours]` | soak test on the test instance (phase 6 item 4), log in `build/soak.log` |
| `tools/test_all.sh` | **all tests** on the test instance, each on fresh data in `build/testsd/` (live BBS and `sd/` untouched) – about 20 min |
| `cd tools && python3 test_phase6.py` | 24/7: idle across the clock wrap, silent + drip bots, hang-ups at every stage, FujiNet restart (right after `emu.sh`) |
| `cd tools && python3 test_phase5.py` | sysop console: present/away, paging + chat, kick, local login, quit (`build/test/bbs-test.xex`) |
| `cd tools && python3 test_phase4.py` | PMs: new user → sysop, reply, read marker, unknown recipient, delete, guests |
| `cd tools && python3 test_phase1.py` | automated test calls: PC (ASCII/telnet), Atari (ATASCII), C64 (PETSCII) |
| `cd tools && python3 test_phase3.py` | menus, chains, boards: write, new, read, approval rule, delete (needs fresh data) |
| `cd tools && python3 test_phase2.py` | register, login, approval, guest, password change, counters (needs fresh `users.dat`/`stats.dat` and `sysop.txt` = `geheim1`) |
| `tools/call.py '<text' '>keys\r' …` | ad-hoc scripted telnet call (expect/send steps), test instance 6503 by default |

## Findings (Phase 0)

- **Listen:** `network_open("N1:TCP://:6502/", OPEN_MODE_RW, 0)`, poll `network_status()` until
  `connected == 1`, `network_accept()`; caller gone ⇒ `connected == 0`; `network_close_client()`
  keeps the listener for the next call. Two calls in a row tested.
- **Second caller:** the firmware refuses a 2nd accept while one is connected; the 2nd caller's
  TCP connect succeeds but waits until the line is free – no "busy" message possible.
- **SD path:** use `N2:SD:///bbs/file` (empty host). `SD://bbs/file` treats `bbs` as host and
  writes to the SD root; `SD:/bbs/file` is not parsed as a URL.
- **Modes:** write (`wb`, truncates!), append (`OPEN_MODE_APPEND`, `ab`), read + `network_seek` work.
- **Line ends:** cc65 (atari) turns `\n` in strings into ATASCII EOL `$9B`. For telnet use octal
  escapes `"\015\012"` (not `\x0d\x0a…` – a following hex letter is eaten into the escape).
- **Port 9997:** the Altirra NetSIO hub from `atari-dev` also binds UDP 9997; while it runs,
  atari800 can't talk to FujiNet-PC (every call returns error 1). `make run` stops it first.
- **Background emulator:** start atari800 with `< /dev/null` (else it stops on terminal input).
- In one of five runs the listener didn't appear within 80 s – cause: FujiNet-PC crash on reboot (see phase 6 item 2).

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

## Two clocks: RTCLOK and the FujiNet clock

The BBS uses two clocks, each for what it does best:

| | FujiNet clock (APETIME) | Atari RTCLOK |
|---|---|---|
| Answers | "What time is it?" | "How much time has passed?" |
| Used for | dates on messages and PMs, a user's last call (`clock_get_time(…, APETIME_BINARY)` in `user.c`, stored in `msg.date`) | pre-login limits (60 s idle / 5 min total), idle limit after login, time online, sound timing |
| Cost | one SIO call (milliseconds), needs the FujiNet to answer | one memory read |
| Weak spots | jumps (DST switch, NTP correction), midnight | wraps every 3.9 days |

**RTCLOK** is a 3-byte counter at `$12–$14` that the OS increments every frame – 50 times a second on PAL.
After 2^24 = 16,777,216 ticks (335,544 s = **3.9 days**) it rolls over from `FFFFFF` to `000000`. A plain
`now - start` across that wrap goes negative and, as unsigned math, becomes ~4 billion ticks: an idle caller
looks idle for years and is kicked, or a wait loop never ends. So all time math uses

```c
#define ELAPSED(t)  ((clock() - (t)) & 0xFFFFFFUL)     /* src/ticks.h */
```

– the result is kept to 24 bits like the counter itself, so `200 - 16,777,000` comes out as 416 ticks. Correct
for any span under 3.9 days; the longest the BBS measures is minutes.

**Why not the FujiNet clock for timeouts too:** the BBS checks its timeouts constantly while it waits for
input – an SIO call each time would cost speed and keep N2 busy; a wall clock can jump (winter time: 03:00 →
02:00, NTP corrections) and wraps at midnight; and the limits must work even when the FujiNet answers slowly,
so a bot can't hold the line because N2 hangs. RTCLOK counts steadily, never jumps and costs nothing – its
only quirk, the wrap, is handled by `ELAPSED`.

Tested: test builds (`-DTESTWRAP`) wrap ~90 s after boot, phase 6 test 1 keeps a caller online across it.
The first natural wrap of the live BBS (started 2026-11-03 05:59) is due Fri 2026-11-07 ~03:30 (manual test 7).

## 24/7 hardening (Phase 6)

**Item 1 – network robustness** (done 2026-11-01):

- **Clock wrap:** cc65's `clock()` reads the 24-bit RTCLOK, which wraps every 2^24 ticks (**3.9 days** PAL).
  All time math uses `ELAPSED(t)` (`src/ticks.h`, masked to 24 bits). Before, the wrap kicked a connected
  caller as "idle", showed a wrong online time and could hang `sound.c`'s wait loop for 93 hours.
  Test builds (`-DTESTWRAP`) set RTCLOK to wrap ~90 s after boot, so every test run crosses a wrap.
- **Bots on the internet:** before login a caller gets 60 s without a key (`LOGIN_IDLE`) and 5 min in total
  (`LOGIN_TOTAL`), then "Zeit abgelaufen" and hang-up – a port scanner can't hold the single line. After login
  the 5 min idle limit stays, no total limit. Test builds use 20 s / 60 s.
- **Hang-ups:** a caller vanishing at any stage (terminal choice, password, menu, editor, during output) ends the
  session and the BBS answers the next call; a failed `network_write` marks the caller gone at once.
- **Lost listener:** if FujiNet restarts, the status call still succeeds but reports `conn=0, err=207`
  (NOT_CONNECTED, firmware `NDevice::current_status`); after 4 such polls (~1 s) the BBS reopens the listener
  ("[Listener neu geoeffnet (n)]" on the sysop screen). A failed listen at startup retries every 10 s instead
  of exiting.
- A connection that dies without a TCP close (phone loses signal) can't be detected; it ends via the idle limit (5 min after login, 60 s before).

**Item 2 – listener at boot** (done 2026-11-02):

- Measured: a fresh start (`tools/emu.sh`) listens after ~10 s, 10 of 10 runs. The old "no listener within 80 s"
  was FujiNet-PC crashing on reboot (phase 1 finding), not a slow start.
- The real gap: a **cold start of the Atari while FujiNet-PC runs** (emulator restart, Shift-F5, later a power
  cycle) – FujiNet-PC reboots with the listener open, crashes with exit 134, and upstream `run-fujinet` only
  restarts on exit 75. The BBS then retried its listen forever against nothing.
- `tools/run-fujinet` replaces it (`make run` and `tools/emu.sh` use it): restarts FujiNet-PC on every exit
  except a deliberate stop (SIGTERM/SIGINT/SIGKILL), crash-loop guard (>10 restarts/min → 30 s pause),
  timestamped lines in `build/fujinet.log`. `tools/inst.sh … stop` ends the loop first.
  Listener back ~5 s after a cold start when FujiNet-PC restarts (phase 6 test 5).
- **Emulator limit (found in item 3):** restarting the atari800 *process* while FujiNet-PC keeps running
  leaves the NetSIO link dead (atari800 says "netsio connected", FujiNet-PC never gets a SIO frame) –
  unless FujiNet-PC happens to crash and restart, as it did in the measurements above. `make run` always
  restarts both, so only restart atari800 via `make run`. A fresh FujiNet-PC does link to a running
  atari800 (test 6). Not an issue on real hardware (no NetSIO).

**Item 3 – SD write safety** (done 2026-11-02):

- **No false "missing file":** `store_put` creates a missing file with `OPEN_MODE_WRITE`, which *truncates*.
  A failed open used to count as "missing" right away – one SIO hiccup could empty `users.dat`. Now the
  file is only created if three read-write and three read opens fail. `store_size_sure()` (retried) is used
  wherever "missing/empty" leads to a write: new message number, new user number, first-start setup.
- **Boot waits for the SD:** after a cold start FujiNet may still be coming up; `user_init()` would take an
  unreadable `users.dat` for a missing one and set up a new SYSOP with the default password. The BBS now
  loads `menus.txt` first and waits ("SD (N2:) nicht lesbar, warte 5 s") until it is readable.
- **Torn writes heal:** messages and new users are written at computed positions instead of appended. A
  save that broke off halfway (text without header, half a header, half a user record) is overwritten by
  the next save instead of shifting every later record. The message header is written last (= commit).
- **Read-back check** for message headers, deletions and user records (`store_put_check`): the firmware
  buffers writes and doesn't report `fclose` errors, so a write "OK" alone proves little.
- **Host backup:** `tools/backup.sh` puts a snapshot of `sd/bbs` into `backup/bbs-<date>.tar.gz` (skips when
  nothing changed, keeps 30; `backup/` is git-ignored – it holds password hashes). Hourly via cron:
  `0 * * * * /home/joerg/claude/blues-bbs/tools/backup.sh` (installed 2026-11-03, output in `build/backup.log`).
  The snapshots are on the same disk – copy `backup/` elsewhere now and then. Restore: `make stop`, `tar -xzf … -C sd`.
- Limits: `store_size` is capped at 65534 bytes by the firmware ("bytes waiting") – ~1365 messages per board
  index, 2047 users. A power cut on a real FujiNet can still damage the FAT – that's what the backup is for.
- Tests: phase 6 test 7 (torn message/user writes), test 8 (boot without readable SD, users unchanged).

**Item 4 – soak test** (2026-11-03):

*Automated* – `tools/soak.sh [hours]` (default 4, log in `build/soak.log`): fresh test data and test instance,
4 members registered + approved, then mixed calls until the time is up – guests, members posting and reading
back, reading all new messages (paging through or quitting), PMs between members, hang-ups at 5 stages,
Atari (ATASCII) and C64 (PETSCII) callers, silent bots. Every 10 min a health line: answer time (p50/p95/max),
`*.idx` without half records and with the posted message counts, `users.dat` size, `stats.dat` total = calls
past the terminal choice, FujiNet-PC + atari800 memory, FujiNet-PC restarts. Aborts if no call succeeds for
10 min. About 160 calls per hour.

Run 2026-11-03, 8 h (06:48–14:49): **1436 calls, 1434 ok, 2 failed, 0 data problems.**

- Answer time (connect → "DRUECKEN") flat all day: median 0.21 s, max per 10 min 0.26–0.38 s, once 16.5 s
  (11:40–11:50, one call).
- 156 + 169 posts, 137 PMs; every health check consistent (`*.idx` whole records and = posted count,
  `users.dat` 5 records, `stats.dat` total = counted calls).
- FujiNet-PC 12–13 MB, atari800 18–19 MB from start to end – no leak; no FujiNet-PC restart after boot.
- The 2 failures were both posts (08:54, 13:35, of ~325): the test didn't get what it expected, the data
  stayed consistent. Cause unknown – that run didn't log the transcript; `soak.py` now logs the last
  expects + received text on a failure, and `SOAK_ONLY=s_post tools/soak.sh 1` runs posts only.
  Follow-up run, posts only, 1 h (14:49–15:49): 147 posts, 0 failed, 0 data problems – not reproduced
  (~1 in 160 posts before). Watch the next soak run's transcript.

*Manual, on the live BBS* (2026-11-03, phone on mobile data via DynDNS):

| # | Test | Result |
|---|------|--------|
| 1 | Logged in, then airplane mode (connection vanishes without TCP close) | ✔ hung up after the 5 min idle limit (07:44 call, 07:50:02 hang-up); next call 07:52 answered at once while the dead connection was still in `FIN-WAIT-1` |
| 2 | Second caller while the line is busy | ✔ TCP connect succeeds, then silence (no busy message possible); gets "BITTE <RETURN> DRUECKEN" right after the first caller logs off |
| 3 | Sysop present (A), caller pages via Hauptmenue 9, sysop answers (C), chat both ways, typing at the same time | ✔ "SYSOP:" / name prefixes keep the lines apart |
| 4 | Idle limit after login | ✔ (= test 1) |
| 5 | FujiNet-PC web UI `:8000` from outside | ✔ timeout – not forwarded |
| 6a | C64 with a real terminal program: VICE 3.10 (x64sc), userport RS232 wired to `localhost:6502`, ccgmsterm v0.2 at 2400 baud (`tools/c64call.sh`) | ✔ PETSCII, case, menus fine. VICE connects as soon as ccgmsterm starts, so the 60 s pre-login limit runs from then – press RETURN right away |
| 6b | Real Atari XL (FujiNet) and real C64 (Meatloaf) as callers | open |
| 7 | First real RTCLOK wrap (3.9 days after the 2026-11-03 05:59 start) | open – Fri 2026-11-07 ~03:30 |

Note for callers: a busy line looks like a hanging connection – worth saying in the announcement.

**Item 5 – go-live / unattended operation:** parked 2026-11-03 – depends on whether the BBS runs on real
hardware or in the emulator. Untracked draft for the emulator case in `drafts/` (supervisor/watchdog
`live.sh`, systemd user service), not tested. Backup cron is installed (item 3).
