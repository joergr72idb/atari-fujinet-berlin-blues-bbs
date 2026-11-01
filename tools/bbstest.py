#!/usr/bin/env python3
"""Scripted test calls against the BBS on localhost:6502."""
import socket, sys, time

def petscii(text):
    """ASCII text as the BBS sends it to a C64 (upper/lower case charset)."""
    out = bytearray()
    for ch in text.encode('latin-1'):
        if 0x61 <= ch <= 0x7a: ch -= 0x20
        elif 0x41 <= ch <= 0x5a: ch += 0x80
        elif ch == 0x0a: ch = 0x0d
        out.append(ch)
    return bytes(out)

class Call:
    def __init__(self, name, host='localhost', port=6502):
        self.name, self.buf, self.ok = name, b'', True
        self.s = socket.create_connection((host, port), timeout=60)
        print('== ' + name)

    def expect(self, want, timeout=60):
        if isinstance(want, str):
            want = want.encode('latin-1')
        end = time.time() + timeout
        while want not in self.buf and time.time() < end:
            try:
                d = self.s.recv(1024)
            except socket.timeout:
                break
            if not d:
                break
            self.buf += d
        hit = want in self.buf
        print(('  OK   ' if hit else '  FAIL ') + repr(want))
        if not hit:
            print('  got: ' + repr(self.buf[-300:]))
            self.ok = False
            raise SystemExit(1)
        self.buf = self.buf[self.buf.index(want) + len(want):]

    def page_until(self, want, key='\r', timeout=60):
        """Answer every --Mehr-- prompt with key until want shows up."""
        if isinstance(want, str):
            want = want.encode('latin-1')
        more, end, pages = b'--Mehr--', time.time() + timeout, 0
        while time.time() < end:
            if want in self.buf:
                break
            if more in self.buf[self.buf.find(more) if more in self.buf else 0:] and self.buf.endswith(b'(RETURN/Q)'):
                self.buf = b''
                pages += 1
                self.send(key)
                continue
            try:
                d = self.s.recv(1024)
            except socket.timeout:
                break
            if not d:
                break
            self.buf += d
        print('  ... %d x --Mehr--' % pages)
        self.expect(want, 1)

    def send(self, data):
        if isinstance(data, str):
            data = data.encode('latin-1')
        self.s.sendall(data)

    def close(self):
        self.s.close()
