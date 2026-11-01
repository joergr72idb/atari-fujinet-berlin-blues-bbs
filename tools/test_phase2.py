#!/usr/bin/env python3
"""Phase 2: register, login, approval, guest, password change, call counters.
Needs a fresh start: sd/bbs/users.dat + stats.dat removed, sd/bbs/sysop.txt = 'geheim1'."""
import time, struct, datetime, pathlib
from bbstest import Call

today = datetime.datetime.now().strftime('%d.%m.%y')
SD = pathlib.Path(__file__).resolve().parent.parent / 'sd' / 'bbs'

def dial(name, atari=False):
    c = Call(name)
    c.expect('DRUECKEN')
    if atari:
        c.send(b'\x9b')
    else:
        c.send('\r\n'); c.expect('1=PC'); c.send('1')
    return c

def enter(c, text, atari=False):
    c.send(text + ('\x9b' if atari else '\r\n'))

# 1: new user registers
c = dial('1 Joerg registriert sich')
c.expect('Du bist Anrufer Nr. 1')
c.expect('Username ==>'); enter(c, 'Joerg')
c.expect('Neu eintragen (J/N)?'); c.send('j')
c.expect('Neues Passwort'); enter(c, 'test1234')
c.expect('Nochmal'); enter(c, 'test1234')
c.expect('Willkommen!')
c.expect('Dein Status NEU (wartet auf Freigabe)')
c.expect('Anrufe       : 001')
c.expect('Letzter Anruf: -')
c.expect(' =>'); enter(c, '83')
c.expect('SYSOP'); c.expect('Joerg')
c.expect(' =>'); enter(c, '0')
c.expect('im System.'); c.close(); time.sleep(3)

# 2: login from an Atari, first a wrong password
c = dial('2 Joerg vom Atari, falsches Passwort', atari=True)
c.expect('Anrufer Nr. 2')
c.expect('Username ==>'); enter(c, 'joerg', True)        # name is case insensitive
c.expect('Passwort ==>'); enter(c, 'falsch', True)
c.expect('Falsch.')
c.expect('Passwort ==>'); enter(c, 'test1234', True)
c.expect('Anrufe       : 002')
c.expect('Letzter Anruf: ' + today)
c.expect(' =>'); enter(c, '0', True); c.expect('im System.'); c.close(); time.sleep(3)

# 3: sysop approves Joerg
c = dial('3 Sysop schaltet frei')
c.expect('Username ==>'); enter(c, 'sysop')
c.expect('Passwort ==>'); enter(c, 'geheim1')
c.expect('Dein Status SYSOP')
c.expect(' =>'); enter(c, '8'); c.expect('9 Freischalten'); c.expect(' =>'); enter(c, '9')
c.expect('  2 Joerg')
c.expect('freischalten'); enter(c, '2')
c.expect('Joerg ist freigeschaltet.')
c.expect('freischalten'); enter(c, '')
c.expect(' =>'); enter(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 4: guest
c = dial('4 Gast')
c.expect('Username ==>'); enter(c, '')
c.expect('Dein Status GAST')
c.expect('Anrufer  : 4 (heute 4)')
c.expect('Zuletzt  : SYSOP')
c.expect(' =>'); enter(c, '89')                          # sysop item: not for guests
c.expect('PMsg/Userverwaltung'); c.expect(' =>'); enter(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 5: Joerg is now a member and changes the password
c = dial('5 Joerg ist Mitglied, aendert Passwort')
c.expect('Username ==>'); enter(c, 'Joerg')
c.expect('Passwort ==>'); enter(c, 'test1234')
c.expect('Dein Status MITGLIED')
c.expect(' =>'); enter(c, '84')
c.expect('Altes Passwort'); enter(c, 'test1234')
c.expect('Neues Passwort'); enter(c, 'blues87')
c.expect('Nochmal'); enter(c, 'blues87')
c.expect('Passwort geaendert.')
c.expect(' =>'); enter(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 5b: line-mode telnet client (Linux telnet): "1" arrives together with CR LF
c = Call('5b Zeilenmodus-Telnet')
c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.send('1\r\n')
c.expect('Username ==>'); enter(c, 'Joerg')
c.expect('Passwort ==>'); enter(c, 'blues87')
c.expect('Dein Status MITGLIED')
c.expect(' =>'); enter(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 6: new password works, old one not
c = dial('6 neues Passwort')
c.expect('Username ==>'); enter(c, 'Joerg')
c.expect('Passwort ==>'); enter(c, 'test1234'); c.expect('Falsch.')
c.expect('Passwort ==>'); enter(c, 'blues87')
c.expect('Anrufe       : 005')
c.expect(' =>'); enter(c, '0'); c.expect('im System.'); c.close()

# data on the SD card
users = (SD / 'users.dat').read_bytes()
print('== users.dat: %d bytes' % len(users))
assert len(users) == 64, 'expected 2 records'
for i in range(2):
    name, pw, calls, level, flags = struct.unpack('<16sIHBB', users[i*32:i*32+24])
    print('  %d %-6s level %d calls %d pw %08x' % (i + 1, name.rstrip(b'\0').decode(), level, calls, pw))
assert users[32+22] == 2, 'Joerg should be level 2'
total, today_calls = struct.unpack('<IH', (SD / 'stats.dat').read_bytes()[:6])
print('== stats.dat: total %d, today %d' % (total, today_calls))
assert total == 7
assert not (SD / 'sysop.txt').exists(), 'sysop.txt should be deleted after first start'
print('== sysop.txt deleted after first start')
print('ALL OK')
