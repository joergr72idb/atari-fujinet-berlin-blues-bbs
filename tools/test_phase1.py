#!/usr/bin/env python3
"""Phase 1: terminal detection, echo, backspace, paging - for PC, Atari, C64 (as guest)."""
import time
from bbstest import Call, petscii

# PC: RETURN = CR LF, telnet IAC negotiation from the client must be ignored
c = Call('PC/Mac (ASCII, telnet)')
c.send(b'\xff\xfd\x01\xff\xfd\x03')                 # DO ECHO, DO SGA - like a real telnet client
c.expect('BITTE <RETURN> DRUECKEN'); c.send('\r\n')
c.expect('1=PC/MAC'); c.send('1')
c.expect(b'\xff\xfb\x01')                          # we answer WILL ECHO
c.expect('Anrufer Nr.')
c.expect('Username ==>'); c.send('xy\x08\x08\r\n')  # typed and erased again -> guest
c.expect('xy\x08 \x08\x08 \x08')                     # echo + BS-space-BS
c.expect('Dein Status GAST')
c.expect('=>'); c.send('4')
c.page_until('Ende des Textes.')
c.expect('=>'); c.send('4')
c.expect('--Mehr--'); c.send('q')
c.expect('=>'); c.send('0')
c.expect('Bis zum naechsten Mal')
c.close()
time.sleep(3)

# Atari: RETURN = $9B selects ATASCII without a question, BS = $7E
c = Call('Atari (ATASCII)')
c.expect('BITTE <RETURN> DRUECKEN'); c.send(b'\x9b')
c.expect(b'Anrufer Nr.')
c.expect('Username ==>'); c.send(b'ab\x7e\x7e\x9b')
c.expect(b'ab\x7e\x7e\x9b')
c.expect(b'Dein Status GAST\x9b')
c.expect('=>'); c.send('0')
c.expect('Bis zum naechsten Mal')
c.close()
time.sleep(3)

# C64: RETURN = CR, then "3"; text arrives in PETSCII, keys are sent in PETSCII
c = Call('C64 (PETSCII)')
c.expect('BITTE <RETURN> DRUECKEN'); c.send(b'\r')
c.expect('3=C64'); c.send('3')
c.expect(b'\x0e')                                  # lower case charset
c.expect(petscii('Du bist Anrufer Nr.'))
c.expect(petscii('Username ==>')); c.send(petscii('Hi') + b'\x14\x14\r')
c.expect(petscii('Hi') + b'\x14\x14\r')
c.expect(petscii('Dein Status GAST\n'))
c.expect('=>'); c.send('0')
c.expect(petscii('Bis zum naechsten Mal'))
c.close()
print('ALL OK')
