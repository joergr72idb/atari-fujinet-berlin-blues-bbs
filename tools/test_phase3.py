#!/usr/bin/env python3
"""Phase 3: menus from menus.txt, chains (*42), boards: write, new, read, approval rule, delete.
Needs fresh data: no users.dat/stats.dat/lastread.dat/b*.idx/b*.dat, sd/bbs/sysop.txt = 'geheim1'."""
import time, struct, pathlib
from bbstest import Call

SD = pathlib.Path(__file__).resolve().parent.parent / 'build' / 'testsd' / 'bbs'

def dial(name, user, pw=None, new=False):
    c = Call(name)
    c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.send('1')
    c.expect('Username ==>'); c.send(user + '\r\n')
    if new:
        c.expect('(J/N)?'); c.send('j')
        c.expect('Neues Passwort'); c.send(pw + '\r\n')
        c.expect('Nochmal'); c.send(pw + '\r\n')
    elif pw:
        c.expect('Passwort ==>'); c.send(pw + '\r\n')
    return c

def cmd(c, keys):
    c.expect(' =>'); c.send(keys + '\r\n')

def write(c, subject, lines):
    c.expect('Betreff ==>'); c.send(subject + '\r\n')
    for l in lines:
        c.expect(':'); c.send(l + '\r\n')
    c.expect(':'); c.send('.\r\n')
    c.expect('Speichern (J/N)?'); c.send('j')

def bye(c):
    cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 1: Anna registers, may not write yet
c = dial('1 Anna neu', 'Anna', 'anna2026', new=True)
c.expect('Neue Msg: keine')
c.expect('Hauptmenue'); c.expect('4 Computer Box'); c.expect('9 Msg an Sysop/Chat')
cmd(c, '2'); c.expect('Termine & Treffen'); c.expect('0 Msg, 0 neu')
cmd(c, '4'); c.expect('noch nicht freigeschaltet')
bye(c)

# 2: sysop approves Anna and writes two messages
c = dial('2 Sysop', 'SYSOP', 'geheim1')
cmd(c, '89'); c.expect('  2 Anna')
c.expect('freischalten'); c.send('2\r\n'); c.expect('Anna ist freigeschaltet.')
c.expect('freischalten'); c.send('\r\n')
cmd(c, '*2'); c.expect('Termine & Treffen')
cmd(c, '4'); write(c, 'Treffen 14.11.', ['Naechstes Treffen am Freitag,', '14.11. um 19 Uhr im Club.'])
c.expect('Msg Nr. 1 gespeichert.')
cmd(c, '*42'); c.expect('Commodore 64')                     # chain: Hauptmenue -> Computer Box -> C64
cmd(c, '4'); write(c, 'Meatloaf-Frage', ['Wer hat schon Meatloaf 1.0?'])
c.expect('Msg Nr. 1 gespeichert.')
bye(c)

# 3: Anna sees and reads the new messages, then writes
c = dial('3 Anna liest', 'Anna', 'anna2026')
c.expect('Neue Msg: Termine & Treffen (1), Commodore 64 (1)')
cmd(c, '7')
c.expect('=== Termine & Treffen ==='); c.expect('Msg 1 von SYSOP'); c.expect('Betreff: Treffen 14.11.')
c.expect('14.11. um 19 Uhr im Club.')
c.expect('=== Commodore 64 ==='); c.expect('Wer hat schon Meatloaf 1.0?')
cmd(c, '7'); c.expect('Keine neuen Msg.')
cmd(c, '*6'); c.expect('Plauderecke')
cmd(c, '4'); write(c, 'Hallo', ['Moin zusammen!'])
c.expect('Msg Nr. 1 gespeichert.')
cmd(c, '*24'); c.expect('Betreff ==>')                       # chain into a board action, then cancel
c.send('\r\n')
bye(c)

# 4: guest reads, may not write; help text
c = dial('4 Gast', '')
c.expect('Neue Msg: Termine & Treffen (1), Commodore 64 (1), Plauderecke (1)')
cmd(c, '11'); c.expect('Kurz-Anleitung')
cmd(c, '*6'); cmd(c, '1'); c.expect('Msg 1 von Anna'); c.expect('Moin zusammen!')
cmd(c, '4'); c.expect('Gaeste koennen nur lesen.')
bye(c)

# 5: sysop deletes Anna's message
c = dial('5 Sysop loescht', 'SYSOP', 'geheim1')
c.expect('Neue Msg: Plauderecke (1)')
cmd(c, '*6'); c.expect('5 Msg loeschen')
cmd(c, '5'); c.expect('Nr. ==>'); c.send('1\r\n'); c.expect('Geloescht.')
cmd(c, '3'); c.expect('Nr. ==>'); c.send('1\r\n'); c.expect('(geloescht)')
bye(c)

# files on the SD card
idx, dat = (SD / 'b01.idx').read_bytes(), (SD / 'b01.dat').read_bytes()
print('== b01.idx %d bytes, b01.dat %r' % (len(idx), dat))
assert len(idx) == 48 and dat == b'Naechstes Treffen am Freitag,\n14.11. um 19 Uhr im Club.\n'
off, ln, frm = struct.unpack('<IH16s', (SD / 'b10.idx').read_bytes()[:22])
print('== b10.idx: offset %d len %d from %s flags %d' % (off, ln, frm.rstrip(b'\0').decode(), (SD / 'b10.idx').read_bytes()[27]))
assert (SD / 'b10.idx').read_bytes()[27] & 1
print('ALL OK')
