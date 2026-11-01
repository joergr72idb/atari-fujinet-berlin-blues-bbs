#!/usr/bin/env python3
"""Phase 5: sysop console - present/away, paging + chat, kick, local login, quit.
Runs against build/bbs-test.xex (sysop keys via sd/bbs/testkeys.txt). Needs fresh data."""
import os, time, socket, struct, subprocess, pathlib
from bbstest import Call

SD = pathlib.Path(__file__).resolve().parent.parent / 'build' / 'testsd' / 'bbs'

def keys(data):
    """'Press' keys on the Atari (ATASCII; RETURN = \\x9b, ESC = \\x1b)."""
    if isinstance(data, str):
        data = data.encode('latin-1')
    tmp = SD / 'testkeys.tmp'
    tmp.write_bytes(data)
    os.rename(tmp, SD / 'testkeys.txt')
    end = time.time() + 15                      # the BBS polls the file about once a second
    while (SD / 'testkeys.txt').exists() and time.time() < end:
        time.sleep(0.2)
    assert not (SD / 'testkeys.txt').exists(), 'BBS did not pick up the keys'
    print('  keys ' + repr(data))

def guest(name):
    c = Call(name)
    c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.send('1')
    c.expect('Username ==>'); c.send('\r\n')
    return c

def cmd(c, k):
    c.expect(' =>'); c.send(k + '\r\n')

def calls_of_sysop():
    return struct.unpack('<H', (SD / 'users.dat').read_bytes()[20:22])[0]

# 1: sysop present
keys('A')
c = guest('1 Sysop anwesend, Chat')
c.expect('Sysop ist: anwesend')
cmd(c, '9'); c.expect('Sysop zum Chat rufen (J/N)?'); c.send('j')
c.expect('Ich rufe den Sysop'); c.expect('.')
keys('C')
c.expect('*** Der Sysop ist da - Chat! ***')
keys('Hallo Gast\x9b')
c.expect('Hallo Gast\r\n')
c.send('Hi Sysop\r\n'); c.expect('Hi Sysop\r\n')
keys('\x1b')
c.expect('*** Chat beendet ***')
cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 2: kick
c = guest('2 Kick')
c.expect(' =>')
keys('K')
c.expect('Der Sysop hat die Verbindung beendet.')
c.s.settimeout(10)
assert c.s.recv(100) in (b'', b'\r\n'), 'connection should be closed'
c.close(); print('  OK   connection closed'); time.sleep(3)

# 3: sysop away -> letter (guests have no mailbox)
keys('A')
c = guest('3 Sysop weg')
c.expect('Sysop ist: abwesend')
cmd(c, '9'); c.expect('Der Sysop ist gerade nicht da.'); c.expect('Gaeste haben keinen Briefkasten')
cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 4: paging without answer (30 s)
keys('A')
c = guest('4 Keine Antwort')
cmd(c, '9'); c.expect('(J/N)?'); c.send('j')
c.expect('Keine Antwort.', timeout=90)
cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 5: local login as SYSOP on the Atari keyboard
before_calls, before_total = calls_of_sysop(), struct.unpack('<I', (SD / 'stats.dat').read_bytes()[:4])[0]
keys('L')
keys('SYSOP\x9bgeheim1\x9b')
time.sleep(8)
keys('0\x9b')
time.sleep(5)
after_calls, after_total = calls_of_sysop(), struct.unpack('<I', (SD / 'stats.dat').read_bytes()[:4])[0]
print('  SYSOP calls %d -> %d, total calls %d -> %d' % (before_calls, after_calls, before_total, after_total))
assert after_calls == before_calls + 1 and after_total == before_total, 'local login: user counted, system not'

# 6: a call still works after the local session
c = guest('6 Anruf nach lokaler Sitzung')
cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 7: quit closes the listener cleanly
keys('Q')
time.sleep(5)
listening = ':6502 ' in subprocess.run(['ss', '-ltn'], capture_output=True, text=True).stdout
fujinet = subprocess.run(['pgrep', '-x', 'fujinet'], capture_output=True).returncode == 0
print('  after Q: listening=%s fujinet running=%s' % (listening, fujinet))
assert not listening and fujinet
print('ALL OK')
