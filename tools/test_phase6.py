#!/usr/bin/env python3
"""Phase 6: 24/7 robustness - clock wrap, pre-login limits (bots), abrupt hang-ups, FujiNet restart, Atari cold start.
Runs against build/test/bbs.xex (LOGIN_IDLE=20, LOGIN_TOTAL=60, clock wraps ~90 s after boot).
Must start right after tools/emu.sh (test 1 relies on the wrap). Needs fresh data (sysop.txt = geheim1)."""
import os, signal, socket, subprocess, time, pathlib
from bbstest import Call

ROOT = pathlib.Path(__file__).resolve().parent.parent
LOGIN_IDLE, LOGIN_TOTAL = 20, 60

def guest(name):
    c = Call(name)
    c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.send('1')
    c.expect('Username ==>'); c.send('\r\n')
    return c

def cmd(c, k):
    c.expect(' =>'); c.send(k + '\r\n')

def bye(c):
    cmd(c, '0'); c.expect('im System.'); c.close(); time.sleep(3)

def closed_after(c, limit):
    """Wait until the BBS hangs up; return the seconds it took."""
    t0 = time.time()
    c.s.settimeout(limit)
    while True:
        try:
            d = c.s.recv(1024)
        except socket.timeout:
            raise SystemExit('  FAIL not hung up after %d s' % limit)
        except ConnectionResetError:
            d = b''
        if not d:
            return time.time() - t0
        c.buf += d

def next_call(name):
    """The BBS must be back at 'warte auf Anruf' and answer the next call."""
    time.sleep(3)
    c = Call(name)
    c.expect('DRUECKEN', 30)
    c.close()
    time.sleep(3)

def listening():
    return ':6503 ' in subprocess.run(['ss', '-ltn'], capture_output=True, text=True).stdout

# 1: logged-in caller idle across the clock wrap (~90 s after boot) - must not be kicked
c = guest('1 Gast idle ueber Clock-Wrap')
cmd(c, '')                                  # back at the menu prompt
time.sleep(100)
c.send('0\r\n'); c.expect('im System.'); c.close()
print('  still online after 100 s idle across the wrap')
time.sleep(3)

# 2: silent connection (port scanner) - hung up after LOGIN_IDLE
c = Call('2 Stummer Bot')
c.expect('DRUECKEN')
t = closed_after(c, LOGIN_IDLE + 20)
print('  hung up after %.0f s (LOGIN_IDLE %d)' % (t, LOGIN_IDLE))
assert LOGIN_IDLE - 2 <= t <= LOGIN_IDLE + 8, t
assert b'Zeit abgelaufen' in c.buf
next_call('2b naechster Anruf')

# 3: drip bot - a junk byte every 5 s keeps the idle timer happy, LOGIN_TOTAL ends it
c = Call('3 Tropf-Bot')
c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC')
t0 = time.time()
c.s.settimeout(1)
gone = False
while time.time() - t0 < LOGIN_TOTAL + 30:
    try:
        c.send('x')
    except (BrokenPipeError, ConnectionResetError):
        gone = True
        break
    try:
        if c.s.recv(1024) == b'':
            gone = True
            break
    except socket.timeout:
        pass
    except ConnectionResetError:
        gone = True
        break
    time.sleep(4)
t = time.time() - t0
print('  drip bot hung up after %.0f s (LOGIN_TOTAL %d)' % (t, LOGIN_TOTAL))
assert gone and LOGIN_TOTAL - 15 <= t <= LOGIN_TOTAL + 15, t
next_call('3b naechster Anruf')

# 4: abrupt hang-ups at every stage; the next call must always work
c = Call('4a weg sofort'); c.close()
next_call('4a danach')
c = Call('4b weg bei Rechnerwahl'); c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.close()
next_call('4b danach')
c = Call('4c weg beim Passwort'); c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.send('1')
c.expect('Username ==>'); c.send('SYSOP\r\n'); c.expect('Passwort ==>'); c.close()
next_call('4c danach')
c = guest('4d weg im Menue'); cmd(c, '2'); c.close()
next_call('4d danach')
c = Call('4e weg beim Schreiben'); c.expect('DRUECKEN'); c.send('\r\n'); c.expect('1=PC'); c.send('1')
c.expect('Username ==>'); c.send('SYSOP\r\n'); c.expect('Passwort ==>'); c.send('geheim1\r\n')
cmd(c, '*2'); c.expect('Termine & Treffen'); cmd(c, '4')
c.expect('Betreff ==>'); c.send('Abbruch\r\n'); c.expect(':'); c.send('halbe Zeile'); c.close()
next_call('4e danach')
c = guest('4f weg waehrend Ausgabe'); cmd(c, '*13'); c.close()   # info text, not read
next_call('4f danach')
c = guest('4g normaler Anruf'); bye(c)

# 5: FujiNet-PC dies and comes back while the BBS waits - the listener must reappear
pids = subprocess.run([str(ROOT / 'tools/inst.sh'), 'test', 'fujinet'], capture_output=True, text=True).stdout.split()
assert pids, 'no test FujiNet-PC'
for p in pids:
    os.kill(int(p), signal.SIGKILL)         # SIGKILL: run-fujinet doesn't restart it, no shutdown crash
time.sleep(3)
assert not listening(), 'still listening after kill'
subprocess.Popen('cd build/fujinet-test && exec nohup ./run-fujinet -u http://127.0.0.1:8001 '
                 '>> ../fujinet-test.log 2>&1 < /dev/null', shell=True, cwd=ROOT, start_new_session=True)
t0 = time.time()
while not listening() and time.time() - t0 < 90:
    time.sleep(1)
print('  listener back after %.0f s' % (time.time() - t0))
assert listening(), 'listener not back after 90 s'
c = guest('5 Anruf nach FujiNet-Neustart'); bye(c)

# 6: the Atari cold-starts (emulator restart / power cycle) with the listener open - FujiNet-PC
#    crashes (exit 134) on the reboot; tools/run-fujinet must restart it and the BBS listen again
fn_before = subprocess.run([str(ROOT / 'tools/inst.sh'), 'test', 'fujinet'], capture_output=True, text=True).stdout.split()
for p in subprocess.run([str(ROOT / 'tools/inst.sh'), 'test', 'atari800'], capture_output=True, text=True).stdout.split():
    os.kill(int(p), signal.SIGTERM)
t0 = time.time()
while listening() and time.time() - t0 < 30:
    time.sleep(0.5)
subprocess.Popen('SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy exec nohup timeout 600 ../atari-dev/opt/atari800/bin/atari800 '
                 '-config atari800.cfg -xl -netsio 9998 -run build/test/bbs.xex >> build/emu.log 2>&1 < /dev/null',
                 shell=True, cwd=ROOT, start_new_session=True)
t0 = time.time()
while not listening() and time.time() - t0 < 60:
    time.sleep(0.5)
print('  listener back %.0f s after the cold start' % (time.time() - t0))
assert listening(), 'listener not back 60 s after the cold start'
fn_after = subprocess.run([str(ROOT / 'tools/inst.sh'), 'test', 'fujinet'], capture_output=True, text=True).stdout.split()
print('  FujiNet-PC %s -> %s' % (fn_before, fn_after))
c = guest('6 Anruf nach Kaltstart'); bye(c)
print('ALL OK')
