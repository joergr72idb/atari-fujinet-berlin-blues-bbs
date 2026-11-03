#!/usr/bin/env python3
"""Phase 6 item 4: soak test - hours of mixed calls against the TEST instance (port 6503), watching that
nothing degrades: the BBS keeps answering, answer times stay flat, data files stay consistent, FujiNet-PC
and atari800 don't grow or restart.

usage: tools/soak.sh [hours]        (fresh test data + test instance, then this script)
       cd tools && python3 soak.py HOURS     (instance already running with fresh data, sysop.txt = geheim1)

Writes a health line every 10 min and a summary to stdout (soak.sh: build/soak.log)."""
import os, random, socket, struct, subprocess, sys, time, pathlib, statistics, contextlib, io
from bbstest import Call, petscii

ROOT = pathlib.Path(__file__).resolve().parent.parent
SD = ROOT / 'build' / 'testsd' / 'bbs'
HOURS = float(sys.argv[1]) if len(sys.argv) > 1 else 4
CHECK_EVERY = 600
OUTAGE = 600                        # no answer for 10 min = abort
USERS = ['Ute', 'Kai', 'Lena', 'Otto']
PW = 'soak1234'
BOARDS = {1: '*2', 10: '*6'}        # board no. -> chain into it
rnd = random.Random(6502)

calls = ok = 0
fails = []                          # (time, scenario, error)
answer_times = []                   # seconds until "DRUECKEN", this window
counted = 0                         # calls that got past the terminal choice (= stats.dat total)
posts = {1: 0, 10: 0}
pms = 0
log = print

def quiet():
    """bbstest prints every expect - keep the soak log readable."""
    return contextlib.redirect_stdout(io.StringIO())

def inst(what):
    return subprocess.run([str(ROOT / 'tools/inst.sh'), 'test', what], capture_output=True, text=True).stdout.split()

def rss(pid):
    try:
        for l in open('/proc/%s/status' % pid):
            if l.startswith('VmRSS'):
                return int(l.split()[1]) // 1024
    except OSError:
        return -1

def restarts():
    try:
        return sum('ended with exit code' in l for l in open(ROOT / 'build/fujinet-test.log', errors='replace')
                   if 'run-fujinet:' in l)
    except OSError:
        return -1

# --- call building blocks -------------------------------------------------------------------------

def dial(name):
    global counted
    t0 = time.time()
    c = Call(name)
    c.expect('DRUECKEN', 30)
    answer_times.append(time.time() - t0)
    return c

def pc(c):
    global counted
    c.send('\r\n'); c.expect('1=PC'); c.send('1'); counted += 1
    c.expect('Username ==>')

def login(c, user, pw=PW):
    pc(c); c.send(user + '\r\n')
    if user:
        c.expect('Passwort ==>'); c.send(pw + '\r\n')

def cmd(c, keys):
    c.expect(' =>'); c.send(keys + '\r\n')

def bye(c):
    cmd(c, '0'); c.expect('im System.'); c.close()

def write(c, subject, lines):
    c.expect('Betreff ==>'); c.send(subject + '\r\n')
    for l in lines:
        c.expect(':'); c.send(l + '\r\n')
    c.expect(':'); c.send('.\r\n'); c.expect('Speichern (J/N)?'); c.send('j')

# --- scenarios -------------------------------------------------------------------------------------

def s_guest():
    c = dial('guest'); login(c, '')
    cmd(c, rnd.choice(['*2', '*6', '*11', '*12', '*83'])); bye(c)

def s_atari():
    global counted
    c = dial('atari'); c.send(b'\x9b'); counted += 1
    c.expect('Username ==>'); c.send(b'\x9b'); c.expect(' =>'); c.send(b'0\x9b'); c.expect('im System.'); c.close()

def s_c64():
    global counted
    c = dial('c64'); c.send(b'\r'); c.expect('3=C64'); c.send('3'); counted += 1
    c.expect(petscii('Username ==>')); c.send(b'\r'); c.expect(petscii(' =>')); c.send(b'0\r')
    c.expect(petscii('im System.')); c.close()

def s_post():
    b = rnd.choice(list(BOARDS)); u = rnd.choice(USERS)
    c = dial('post'); login(c, u)
    cmd(c, BOARDS[b]); cmd(c, '4')
    n = posts[b] + 1
    write(c, 'Soak %d' % n, ['Nachricht %d von %s' % (n, u), 'zweite Zeile ' + 'x' * rnd.randint(0, 30)])
    c.expect('Msg Nr. %d gespeichert.' % n)
    posts[b] = n
    cmd(c, '3'); c.expect('Nr. ==>'); c.send('%d\r\n' % n); c.expect('Nachricht %d von %s' % (n, u))
    bye(c)

def s_read():
    c = dial('read'); login(c, rnd.choice(USERS))
    cmd(c, '7')
    c.page_until(' =>', rnd.choice('\rq'), 180)    # all new messages: page through or quit
    c.send('0\r\n'); c.expect('im System.'); c.close()

def s_pm():
    global pms
    a, b = rnd.sample(USERS, 2)
    c = dial('pm'); login(c, a)
    cmd(c, '*81'); c.expect('An ==>'); c.send(b + '\r\n')
    write(c, 'Hallo %s' % b, ['PM von %s' % a]); c.expect('Brief an %s abgelegt.' % b)
    pms += 1
    bye(c)
    c = dial('pm read'); login(c, b)
    cmd(c, '*82'); c.page_until('RETURN=Ende ==>', '\r', 120); c.send('\r\n'); bye(c)

def s_hangup():
    c = dial('hangup')
    stage = rnd.randint(0, 4)
    if stage >= 1: pc(c)
    if stage >= 2: c.send(rnd.choice(USERS) + '\r\n'); c.expect('Passwort ==>')
    if stage >= 3: c.send(PW + '\r\n'); cmd(c, BOARDS[1])
    if stage >= 4: cmd(c, '4'); c.expect('Betreff ==>'); c.send('weg\r\n'); c.expect(':'); c.send('halb')
    c.close()

def s_bot():
    c = dial('bot')                          # silent: hung up after LOGIN_IDLE (20 s in test builds)
    c.s.settimeout(60)
    while True:
        try:
            if not c.s.recv(1024):
                break
        except (socket.timeout, ConnectionResetError):
            break
    c.close()

SCENARIOS = [(s_guest, 20), (s_post, 20), (s_read, 15), (s_pm, 10), (s_hangup, 15),
             (s_atari, 7), (s_c64, 7), (s_bot, 3), ]
if os.environ.get('SOAK_ONLY'):                  # e.g. SOAK_ONLY=s_post tools/soak.sh 0.5
    SCENARIOS = [(f, w) for f, w in SCENARIOS if f.__name__ in os.environ['SOAK_ONLY'].split(',')]

def setup():
    """Register USERS, sysop approves them (user numbers 2..5)."""
    for u in USERS:
        c = dial('register ' + u); pc(c); c.send(u + '\r\n')
        c.expect('(J/N)?'); c.send('j')
        c.expect('Neues Passwort'); c.send(PW + '\r\n'); c.expect('Nochmal'); c.send(PW + '\r\n')
        bye(c); time.sleep(3)
    c = dial('approve'); login(c, 'SYSOP', 'geheim1')
    cmd(c, '*89')
    for n in range(2, 2 + len(USERS)):
        c.expect('freischalten'); c.send('%d\r\n' % n); c.expect('ist freigeschaltet.')
    c.expect('freischalten'); c.send('\r\n'); bye(c); time.sleep(3)

# --- health ----------------------------------------------------------------------------------------

def health(t_start, final=False):
    global answer_times
    problems = []
    for f in sorted(SD.glob('*.idx')):
        if f.stat().st_size % 48:
            problems.append('%s size %d' % (f.name, f.stat().st_size))
    for b, n in posts.items():
        size = (SD / ('b%02d.idx' % b)).stat().st_size if (SD / ('b%02d.idx' % b)).exists() else 0
        if size != n * 48:
            problems.append('b%02d.idx %d msgs, expected %d' % (b, size // 48, n))
    users = (SD / 'users.dat').read_bytes()
    if len(users) != 32 * (1 + len(USERS)):
        problems.append('users.dat %d bytes' % len(users))
    total = struct.unpack('<I', (SD / 'stats.dat').read_bytes()[:4])[0]
    if total != counted:
        problems.append('stats total %d, expected %d' % (total, counted))
    fn, a8 = inst('fujinet'), inst('atari800')
    at = sorted(answer_times) or [0]
    log('%s %5.2fh calls %5d ok %5d fail %3d | answer p50 %.2fs p95 %.2fs max %.2fs | posts %s pms %d | '
        'FujiNet-PC %s MB, atari800 %s MB, restarts %d%s' % (
            time.strftime('%H:%M'), (time.time() - t_start) / 3600, calls, ok, len(fails),
            statistics.median(at), at[int(len(at) * .95) - 1 if len(at) > 1 else 0], at[-1],
            '/'.join(str(posts[b]) for b in posts), pms,
            '+'.join(str(rss(p)) for p in fn) or 'GONE', '+'.join(str(rss(p)) for p in a8) or 'GONE',
            restarts(), ('\n      PROBLEMS: ' + '; '.join(problems)) if problems else ''))
    sys.stdout.flush()
    if not final:
        answer_times = []
    return problems

def main():
    global calls, ok
    t_start = time.time()
    log('soak %.1f h against port 6503, started %s' % (HOURS, time.strftime('%F %T')))
    with quiet():
        setup()
    health(t_start)
    next_check = time.time() + CHECK_EVERY
    names, weights = zip(*SCENARIOS)
    last_ok = time.time()
    while time.time() - t_start < HOURS * 3600:
        s = rnd.choices(names, weights)[0]
        calls += 1
        out = io.StringIO()
        try:
            with contextlib.redirect_stdout(out):
                s()
            ok += 1
            last_ok = time.time()
        except (SystemExit, OSError) as e:
            fails.append((time.strftime('%H:%M:%S'), s.__name__, repr(e)))
            log('  FAIL %s %s: %r' % (time.strftime('%H:%M:%S'), s.__name__, e))
            log('    ' + '\n    '.join(out.getvalue().splitlines()[-6:]))   # last expects + "got:"
            if time.time() - last_ok > OUTAGE:
                log('  OUTAGE: no successful call for %d s - abort' % OUTAGE)
                break
            time.sleep(10)
        time.sleep(3)
        if time.time() >= next_check:
            health(t_start)
            next_check += CHECK_EVERY
    problems = health(t_start, final=True)
    log('\nsummary: %d calls, %d ok, %d failed, %d data problems' % (calls, ok, len(fails), len(problems)))
    for f in fails[:30]:
        log('  %s %s %s' % f)
    sys.exit(1 if fails or problems else 0)

main()
