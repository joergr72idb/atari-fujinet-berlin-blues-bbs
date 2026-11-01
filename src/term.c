/* Terminal layer: ASCII (PC, CPC), PETSCII (C64), ATASCII (Atari). */
#include <stdio.h>
#include <time.h>
#include "ticks.h"
#include "fujinet-network.h"
#include "net.h"
#include "term.h"
#include "sound.h"
#include "sysop.h"

uint8_t term_mode;
uint8_t term_width = 40;
uint8_t term_abort;
uint8_t term_paging = 1;
uint8_t term_local;

static uint8_t rows = 24;
static uint8_t col, lines;
static uint8_t last_cr;         /* ASCII: swallow LF/NUL after CR */
static char fbuf[128];

/* ---- output ---------------------------------------------------------- */

static void out(uint8_t c)
{
    if (term_local)
        return;                     /* local session: only the sysop screen */
    switch (term_mode) {
    case TERM_ATARI:
        break;
    case TERM_C64:
        if (c == '\n')
            c = 0x0D;
        else if (c >= 'a' && c <= 'z')
            c -= 0x20;              /* PETSCII lower case: $41-$5A */
        else if (c >= 'A' && c <= 'Z')
            c += 0x80;              /* PETSCII upper case: $C1-$DA */
        break;
    default:                        /* PC, CPC */
        if (c == '\n') {
            net_putc(0x0D);
            c = 0x0A;
        }
        break;
    }
    net_putc(c);
}

static void more(void)
{
    int k;
    uint8_t i;

    term_paging = 0;
    term_puts("--Mehr-- (RETURN/Q)");
    term_paging = 1;
    k = term_getc();
    for (i = 0; i < 19; ++i)
        term_putc(KEY_BS);
    if (k == 'q' || k == 'Q' || k == KEY_GONE)
        term_abort = 1;
    lines = 0;
}

void term_putc(char c)
{
    if (term_abort)
        return;
    if (c == KEY_BS) {
        if (term_mode == TERM_ATARI)
            net_putc(0x7E);
        else if (term_mode == TERM_C64)
            net_putc(0x14);
        else
            net_write("\010 \010", 3);
        putchar(0x7E);              /* ATASCII backspace on the sysop screen */
        if (col)
            --col;
        return;
    }
    out(c);
    putchar(c);                     /* sysop mirror on the local screen (E:, scrolls) */
    if (c == '\n' || ++col == term_width) {
        col = 0;
        if (term_paging && ++lines >= rows - 1)
            more();
    }
}

void term_puts(const char *s)
{
    while (*s && !term_abort)
        term_putc(*s++);
}

void term_page_reset(void)
{
    lines = 0;
    term_abort = 0;
}

void term_cls(void)
{
    if (term_mode == TERM_ATARI)
        net_putc(0x7D);
    else if (term_mode == TERM_C64)
        net_putc(0x93);
    else
        net_write("\033[2J\033[H", 7);
    putchar(0x7D);                  /* ATASCII clear screen */
    col = 0;
    term_page_reset();
}

/* ---- input ----------------------------------------------------------- */

static uint16_t idle_secs = IDLE_SECONDS, total_secs;
static clock_t since;

void term_limits(uint16_t idle, uint16_t total)
{
    idle_secs = idle;
    total_secs = total;
    since = clock();
}

static int raw_getc(void)
{
    clock_t idle = clock();
    int c;

    if (term_local) {
        while ((c = sysop_key()) < 0)
            sysop_status();
        return c;
    }
    for (;;) {
        if (sysop_poll())
            return KEY_GONE;        /* kicked by the sysop */
        c = net_getc();
        if (c == NET_GONE)
            return KEY_GONE;
        if (c != NET_NONE)
            return c;
        if (ELAPSED(idle) > idle_secs * CLOCKS_PER_SEC
            || (total_secs && ELAPSED(since) > total_secs * CLOCKS_PER_SEC)) {
            term_puts("\nZeit abgelaufen - bis zum naechsten Mal!\n");
            net_hangup();
            return KEY_GONE;
        }
    }
}

/* Map a byte from the caller to the internal code; -2 = ignore. */
static int map_in(uint8_t c)
{
    switch (term_mode) {
    case TERM_ATARI:
        if (c == 0x9B) return KEY_ENTER;
        if (c == 0x7E) return KEY_BS;
        break;
    case TERM_C64:
        if (c == 0x0D) return KEY_ENTER;
        if (c == 0x14) return KEY_BS;
        if (c >= 0x41 && c <= 0x5A) return c + 0x20;
        if (c >= 0xC1 && c <= 0xDA) return c - 0x80;
        if (c >= 0x61 && c <= 0x7A) return c - 0x20;
        break;
    default:
        if (last_cr && (c == 0x0A || c == 0x00)) {
            last_cr = 0;
            return -2;
        }
        last_cr = (c == 0x0D);
        if (c == 0x0D || c == 0x0A) return KEY_ENTER;
        if (c == 0x08 || c == 0x7F) return KEY_BS;
        break;
    }
    return (c >= 0x20 && c < 0x7F) ? c : -2;
}

int term_getc(void)
{
    int c;

    net_flush();
    do {
        c = raw_getc();
        if (c == KEY_GONE)
            return KEY_GONE;
        c = map_in((uint8_t) c);
    } while (c == -2);
    return c;
}

int term_poll(void)
{
    int c;

    if (term_local)
        return -2;
    for (;;) {
        c = net_getc();
        if (c == NET_GONE)
            return KEY_GONE;
        if (c == NET_NONE)
            return -2;
        if ((c = map_in((uint8_t) c)) != -2)
            return c;               /* skip ignored bytes (LF after CR, ...) */
    }
}

void term_set_local(void)
{
    term_local = 1;
    term_mode = TERM_ATARI;
    term_width = 40;
    rows = 23;
    term_page_reset();
}

int term_getline(char *buf, uint8_t max, uint8_t hidden)
{
    uint8_t len = 0;
    int c;

    for (;;) {
        c = term_getc();
        if (c == KEY_GONE)
            return KEY_GONE;
        if (c == KEY_ENTER) {
            buf[len] = 0;
            term_putc('\n');
            lines = 0;
            return len;
        }
        if (c == KEY_BS) {
            if (len) {
                --len;
                term_putc(KEY_BS);
            }
        } else if (len < max) {
            buf[len++] = (char) c;
            term_putc(hidden ? '*' : (char) c);
        }
        net_flush();
    }
}

/* ---- mode detection -------------------------------------------------- */

/* Shown before we know the machine: upper case only, so it reads the same in
   ASCII, ATASCII and the C64's upper case/graphics mode. */
static void raw_line(const char *s)
{
    net_write("\015\012", 2);
    while (*s)
        net_putc(*s++);
}

uint8_t term_detect(void)
{
    clock_t start;
    int c;

    term_mode = TERM_PC;
    raw_line("BERLIN BLUES BOX - BITTE <RETURN> DRUECKEN");
    net_flush();
    sound_ring();
    for (;;) {
        c = raw_getc();
        if (c == KEY_GONE)
            return 0;
        if (c == 0x9B) {
            term_mode = TERM_ATARI;
            break;
        }
        if (c != 0x0D && c != 0x0A)
            continue;
        raw_line("1=PC/MAC  2=SCHNEIDER CPC  3=C64 ? ");
        do {
            c = raw_getc();
            if (c == KEY_GONE)
                return 0;
        } while (c < '1' || c > '3');
        term_mode = c - '1';
        break;
    }
    /* A line-mode telnet client sends "1" + CR LF together: drop the line end,
       or it would be read as an empty user name (= guest). */
    start = clock();
    while (ELAPSED(start) < CLOCKS_PER_SEC / 2) {
        c = net_getc();
        if (c == NET_GONE)
            return 0;
        if (c == NET_NONE)
            continue;
        if (c != 0x0D && c != 0x0A && c != 0x00 && c != 0x9B)
            break;                  /* shouldn't happen: a real key, keep going */
    }
    last_cr = 0;
    net_telnet = (term_mode == TERM_PC);
    term_width = (term_mode == TERM_PC) ? 80 : 40;
    rows = (term_mode == TERM_C64) ? 25 : 24;
    if (term_mode == TERM_PC) {
        static const uint8_t will[] = { 255, 251, 1, 255, 251, 3 };  /* WILL ECHO, WILL SGA */
        net_write(will, sizeof will);
    }
    if (term_mode == TERM_C64)
        net_putc(0x0E);             /* switch to upper/lower case charset */
    term_cls();
    return 1;
}

/* ---- text files ------------------------------------------------------ */

uint8_t term_type_file(const char *spec)
{
    int16_t n, i;
    char c;

    if (network_open(spec, OPEN_MODE_READ, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    while (!term_abort && (n = network_read_nb(spec, fbuf, sizeof fbuf)) > 0) {
        for (i = 0; i < n && !term_abort; ++i) {
            c = fbuf[i];
            if (c == 0x0A)
                term_putc('\n');
            else if (c != 0x0D)
                term_putc(c);
        }
    }
    network_close(spec);
    return 1;
}
