#!/usr/bin/env python3
"""Phase 4: personal messages - new user writes to sysop, reply, read marker, unknown
recipient, delete, guests. Needs fresh data (see CLAUDE.md), sd/bbs/sysop.txt = 'geheim1'."""
import time, pathlib
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

def text(c, lines, subject=None):
    if subject:
        c.expect('Betreff ==>'); c.send(subject + '\r\n')
    for l in lines:
        c.expect(':'); c.send(l + '\r\n')
    c.expect(':'); c.send('.\r\n')
    c.expect('Speichern (J/N)?'); c.send('j')

def bye(c):
    cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

# 1: new user, not approved: may only write to the sysop
c = dial('1 Joerg neu -> Sysop', 'Joerg', 'test1234', new=True)
c.expect('Es ist keine Post fuer Dich da')
cmd(c, '81'); c.expect('Noch nicht freigeschaltet')
cmd(c, '*9'); c.expect('Brief an den Sysop')
text(c, ['Hallo, ich bin Joerg vom ACC.'], 'Freischaltung bitte')
c.expect('Brief an SYSOP abgelegt.')
bye(c)

# 2: sysop reads, replies, approves
c = dial('2 Sysop liest + antwortet', 'SYSOP', 'geheim1')
c.expect('Du hast 1 neue PMsg! (*82)')
cmd(c, '82'); c.expect('* 1 Joerg'); c.expect('Freischaltung bitt')   # list: subject cut to 18 for 40 columns
c.expect('RETURN=Ende ==>'); c.send('1\r\n')
c.expect('Msg 1 von Joerg'); c.expect('Hallo, ich bin Joerg vom ACC.')
c.expect('Antworten (J/N)?'); c.send('j')
c.expect('Betreff: Re: Freischaltung b')     # subject field holds 19 chars
text(c, ['Bist freigeschaltet - viel Spass!'])
c.expect('Brief an Joerg abgelegt.')
c.expect('RETURN=Ende ==>'); c.send('\r\n')
cmd(c, '*89'); c.expect('freischalten'); c.send('2\r\n'); c.expect('Joerg ist freigeschaltet.')
c.expect('freischalten'); c.send('\r\n')
bye(c)

# 3: Joerg reads the reply, unknown recipient, writes, deletes
c = dial('3 Joerg liest Antwort', 'Joerg', 'test1234')
c.expect('Du hast 1 neue PMsg!')
cmd(c, '82'); c.expect('* 1 SYSOP')
c.expect('RETURN=Ende ==>'); c.send('1\r\n')
c.expect('Bist freigeschaltet - viel Spass!')
c.expect('Antworten (J/N)?'); c.send('n')
c.expect('RETURN=Ende ==>'); c.send('\r\n')
cmd(c, '*81'); c.expect('An ==>'); c.send('Anna\r\n'); c.expect('Unbekannter Benutzer.')
cmd(c, '1'); c.expect('An ==>'); c.send('sysop\r\n')    # still in menu 8, name case-insensitive
text(c, ['Danke!'], 'Danke')
c.expect('Brief an SYSOP abgelegt.')
cmd(c, '2'); c.expect('  1 SYSOP')                     # read now: no '*'
c.expect('RETURN=Ende ==>'); c.send('d1\r\n'); c.expect('Geloescht.')
c.expect('RETURN=Ende ==>'); c.send('1\r\n'); c.expect('(geloescht)')
c.expect('RETURN=Ende ==>'); c.send('\r\n')
bye(c)

# 4: guest has no mailbox
c = dial('4 Gast', '')
cmd(c, '9'); c.expect('Gaeste haben keinen Briefkasten')
bye(c)

# 5: sysop sees exactly one new PMsg (no. 2)
c = dial('5 Sysop', 'SYSOP', 'geheim1')
c.expect('Du hast 1 neue PMsg!')
cmd(c, '82'); c.expect('  1 Joerg'); c.expect('* 2 Joerg')
c.expect('RETURN=Ende ==>'); c.send('\r\n')
bye(c)

sizes = {f: (SD / f).stat().st_size for f in ('p001.idx', 'p002.idx')}
print('== mailboxes:', sizes)
assert sizes == {'p001.idx': 96, 'p002.idx': 48}
print('ALL OK')
