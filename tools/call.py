#!/usr/bin/env python3
"""Scripted BBS call: connect, then alternate expect/send steps.

usage: call.py [host:port] STEP...   STEP = "<text" (wait for text) or ">keys" (send);
       both take escapes like \\r, \\x9b
"""
import socket, sys, time

def main():
    args = sys.argv[1:]
    host, port = 'localhost', 6502
    if args and not args[0][0] in '<>':
        host, port = args[0].split(':'); port = int(port); args = args[1:]
    s = socket.create_connection((host, port), timeout=30)
    got = b''
    for step in args:
        if step[0] == '<':
            want = step[1:].encode().decode('unicode_escape').encode('latin-1')
            end = time.time() + 30
            while want not in got and time.time() < end:
                try:
                    d = s.recv(1024)
                except socket.timeout:
                    break
                if not d:
                    break
                got += d
            ok = want in got
            print(('OK   ' if ok else 'FAIL ') + 'expect ' + step[1:])
            if not ok:
                print(got.decode('latin-1')); sys.exit(1)
        else:
            s.sendall(step[1:].encode().decode('unicode_escape').encode('latin-1'))
    s.close()
    clean = bytes(b for b in got if b < 0xf0).decode('latin-1')
    print('--- session ---'); print(clean.replace('\r', ''))

main()
