#!/usr/bin/env python3
"""Phase 1: terminal detection, line editor, paging, password - for PC, Atari, C64."""
import time
from bbstest import Call, petscii

# PC: RETURN = CR LF, telnet IAC negotiation from the client must be ignored
c = Call('PC/Mac (ASCII, telnet)')
c.send(b'\xff\xfd\x01\xff\xfd\x03')                 # DO ECHO, DO SGA - like a real telnet client
c.expect('BITTE <RETURN> DRUECKEN'); c.send('\r\n')
c.expect('1=PC/MAC'); c.send('1')
c.expect(b'\xff\xfb\x01')                          # we answer WILL ECHO
c.expect('Terminal: PC/Mac, 80 Zeichen\r\n')
c.expect('Anrufer Nr. 1')
c.expect('=>'); c.send('1')
c.expect('Text: '); c.send('hallo\x08\x08lo welt\r\n')
c.expect('Du schriebst: hallo welt\r\n')
c.expect('=>'); c.send('2')
c.page_until('Ende des Textes.')
c.expect('=>'); c.send('2')
c.expect('--Mehr--'); c.send('q')
c.expect('=>'); c.send('3')
c.expect('Passwort ==>'); c.send('geheim\r')
c.expect('******'); c.expect('6 Zeichen')
c.expect('=>'); c.send('0')
c.expect('Bis zum naechsten Mal')
c.close()
time.sleep(3)

# Atari: RETURN = $9B selects ATASCII without a question, BS = $7E
c = Call('Atari (ATASCII)')
c.expect('BITTE <RETURN> DRUECKEN'); c.send(b'\x9b')
c.expect(b'Terminal: Atari, 40 Zeichen\x9b')
c.expect('Anrufer Nr. 2')
c.expect('=>'); c.send('1')
c.expect('Text: '); c.send(b'ab\x7ec\x9b')
c.expect(b'Du schriebst: ac\x9b')
c.expect('=>'); c.send('0')
c.expect('Bis zum naechsten Mal')
c.close()
time.sleep(3)

# C64: RETURN = CR, then "3"; text arrives in PETSCII, keys are sent in PETSCII
c = Call('C64 (PETSCII)')
c.expect('BITTE <RETURN> DRUECKEN'); c.send(b'\r')
c.expect('3=C64'); c.send('3')
c.expect(b'\x0e')                                  # lower case charset
c.expect(petscii('Terminal: C64, 40 Zeichen\n'))
c.expect(petscii('Anrufer Nr. 3'))
c.expect('=>'); c.send('1')
c.expect(petscii('Text: ')); c.send(petscii('Hallo C64').replace(b'\r', b'') + b'\x14\x14' + b'64\r')
c.expect(petscii('Du schriebst: Hallo C64\n'))
c.expect('=>'); c.send('0')
c.close()
print('ALL OK')
