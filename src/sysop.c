/* Sysop console on the Atari.
   Screen: own display list = 1 status line + 23 lines of the OS screen editor (E:, row 0
   hidden), so the session mirror keeps scrolling below a fixed status line.
   Keys: C chat, K kick, A sysop present/away, L local login (idle), Q quit. */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ticks.h"
#include <peekpoke.h>
#include "net.h"
#include "term.h"
#include "lang.h"
#include "store.h"
#include "user.h"
#include "sound.h"
#include "pm.h"
#include "sysop.h"

#define SDLSTL  0x230
#define SAVMSC  0x58
#define LMARGN  0x52
#define KEYDEF  0x79                        /* XL OS: key code -> ATASCII table */
#define SHFLOK  0x2BE
#define ESC     0x1B
#define PAGE_SECONDS 30

void kbd_init(void);
void kbd_exit(void);
int  kbd_get(void);

uint8_t sysop_present;
uint8_t sysop_quit;

static uint8_t dlbuf[72];
static uint8_t *dl;
static uint16_t old_dl;
static char status[41];
static uint8_t statline[40];        /* ANTIC internal codes */
static clock_t last_status;

/* ---- status line ----------------------------------------------------- */

void sysop_init(void)
{
    uint16_t scr = PEEKW(SAVMSC), a;
    uint8_t i, *p;

    POKE(LMARGN, 0);                        /* 40 columns on the sysop screen */
    putchar(0x7D);
    /* display list must not cross a 1K boundary */
    a = (uint16_t) dlbuf;
    if ((a & 0x3FF) > 0x3FF - 34)
        a = (a + 0x3FF) & ~0x3FF;
    dl = p = (uint8_t *) a;
    *p++ = 0x70; *p++ = 0x70; *p++ = 0x70;  /* 24 blank scan lines */
    *p++ = 0x42; *p++ = (uint16_t) statline & 0xFF; *p++ = (uint16_t) statline >> 8;
    *p++ = 0x42; *p++ = (scr + 40) & 0xFF; *p++ = (scr + 40) >> 8;
    for (i = 0; i < 22; ++i)
        *p++ = 0x02;
    *p++ = 0x41; *p++ = (uint16_t) dl & 0xFF; *p = (uint16_t) dl >> 8;
    old_dl = PEEKW(SDLSTL);
    POKEW(SDLSTL, (uint16_t) dl);
    kbd_init();
    putchar('\n');                          /* E: row 0 is hidden */
    sysop_status();
}

void sysop_exit(void)
{
    kbd_exit();
    POKEW(SDLSTL, old_dl);
}

static void show_status(void)
{
    uint8_t i, c, len = strlen(status);

    for (i = 0; i < 40; ++i) {
        c = i < len ? status[i] : ' ';
        c = c < 0x20 ? c + 0x40 : c < 0x60 ? c - 0x20 : c;   /* ATASCII -> ANTIC code */
        statline[i] = c | 0x80;             /* inverse video */
    }
}

void sysop_status(void)
{
    char t[12];

    if (ELAPSED(last_status) < CLOCKS_PER_SEC / 2)
        return;
    last_status = clock();
    if (net_online() || term_local) {
        user_online(t);
        snprintf(status, sizeof status, L_SYS_STATUS_CALL,
                 user.name[0] ? user.name : term_local ? L_SYS_LOCAL : L_SYS_IN_LOGIN, user.level, t,
                 sysop_present ? L_SYS_ST_PRESENT : L_SYS_ST_ABSENT);
    } else {
        snprintf(status, sizeof status, L_SYS_STATUS_IDLE,
                 stats.total, sysop_present ? L_SYS_IDLE_PRESENT : L_SYS_IDLE_ABSENT);
    }
    show_status();
}

/* ---- keys ------------------------------------------------------------ */

#ifdef TESTKEYS
/* Test build: keys can also come from bbs/testkeys.txt (written by the test scripts). */
#define TESTKEYS_FILE BBS_DIR "testkeys.txt"
static char tk[32];
static uint8_t tk_len, tk_pos;
static clock_t tk_last;

static int test_key(void)
{
    uint16_t n;

    if (tk_pos < tk_len)
        return (uint8_t) tk[tk_pos++];
    if (ELAPSED(tk_last) < CLOCKS_PER_SEC)
        return -1;
    tk_last = clock();
    n = store_size(TESTKEYS_FILE);
    if (!n)
        return -1;
    if (n > sizeof tk)
        n = sizeof tk;
    tk_len = store_get(TESTKEYS_FILE, 0, tk, n) ? (uint8_t) n : 0;
    tk_pos = 0;
    store_delete(TESTKEYS_FILE);
    return tk_pos < tk_len ? (uint8_t) tk[tk_pos++] : -1;
}
#endif

/* Key from the VBI keyboard driver (kbd.s), converted with the OS table; CAPS toggles
   upper case like the OS does. Inverse, HELP, F-keys and the like are ignored. */
int sysop_key(void)
{
    int k = kbd_get();
    uint8_t c;

    if (k >= 0 && k < 0xC0) {               /* SHIFT+CTRL+key: no ATASCII */
        c = PEEK(PEEKW(KEYDEF) + k);
        if (c == 0x82) {                    /* CAPS */
            POKE(SHFLOK, PEEK(SHFLOK) ? 0 : 0x40);
            return -1;
        }
        if (c == 0x83) {                    /* SHIFT+CAPS */
            POKE(SHFLOK, 0x40);
            return -1;
        }
        if (c >= 0x80 && c < 0x90)
            return -1;
        if (PEEK(SHFLOK) == 0x40 && c >= 'a' && c <= 'z')
            c -= 0x20;
        return c;
    }
#ifdef TESTKEYS
    return test_key();
#else
    return -1;
#endif
}

static void toggle_present(void)
{
    sysop_present = !sysop_present;
    fputs(sysop_present ? L_SYS_PRESENT : L_SYS_ABSENT, stdout);
    last_status = 0;
}

char sysop_idle(void)
{
    int k;

    sysop_status();
    k = sysop_key();
    if (k == 'a' || k == 'A')
        toggle_present();
    else if (k == 'l' || k == 'L')
        return 'L';
    else if (k == 'q' || k == 'Q')
        return 'Q';
    return 0;
}

/* ---- chat ------------------------------------------------------------ */

/* Every chat line starts with who wrote it ("SYSOP: " / "<Name>: "); if the other side
   starts typing in the middle of a line, that line ends and theirs begins. */
static uint8_t speaker, bol, len;   /* writer of the current line, at line start, chars on it */

static void chat_char(uint8_t caller, char c)
{
    if (c == '\n') {
        if (!bol) {
            term_putc('\n');
            bol = 1;
        }
        return;
    }
    if (c == KEY_BS) {
        if (!bol && speaker == caller && len) {
            term_putc(KEY_BS);          /* never into the prefix or the other's text */
            --len;
        }
        return;
    }
    if (!bol && speaker != caller)
        term_putc('\n');
    if (bol || speaker != caller) {
        term_puts(caller ? user.name : "SYSOP");
        term_puts(": ");
        speaker = caller;
        bol = 0;
        len = 0;
    }
    term_putc(c);
    ++len;
}

/* Chat until the sysop presses ESC; 0 = caller gone.
   The Atari keyboard holds only one key, and every SIO call to FujiNet takes a while:
   while the sysop types, only the keyboard is read; the text goes out on RETURN or
   after a pause, and the caller is polled at most 4 times a second. */
static uint8_t chat(void)
{
    clock_t last_key = 0, last_net = 0;
    int k;

    term_paging = 0;
    term_puts(L_CHAT_START);
    bol = 1;
    net_flush();
    puts(L_SYS_CHAT);
    for (;;) {
        k = sysop_key();
        if (k == ESC)
            break;
        if (k >= 0) {
            if (k == 0x9B)
                chat_char(0, '\n');
            else if (k == 0x7E)
                chat_char(0, KEY_BS);
            else if (k >= 0x20 && k < 0x7D)
                chat_char(0, (char) k);
            last_key = clock();
            if (k == 0x9B)
                net_flush();
            continue;
        }
        if (ELAPSED(last_key) < CLOCKS_PER_SEC)
            continue;                   /* sysop is typing: keyboard only */
        net_flush();
        if (ELAPSED(last_net) < CLOCKS_PER_SEC / 4)
            continue;
        last_net = clock();
        sysop_status();
        /* show everything the caller typed (one SIO poll, then from the buffer) */
        while ((k = term_poll()) != -2) {
            if (k == KEY_GONE) {
                term_paging = 1;
                return 0;
            }
            chat_char(1, (char) k);
        }
        net_flush();
    }
    term_puts(L_CHAT_END);
    net_flush();
    term_paging = 1;
    return 1;
}

static void kick(void)
{
    term_paging = 0;
    term_puts(L_KICKED);
    net_hangup();
    puts(L_SYS_KICKED);
}

uint8_t sysop_poll(void)
{
    int k;

    sysop_status();
    k = sysop_key();
    switch (k) {
    case 'a': case 'A':
        toggle_present();
        break;
    case 'c': case 'C':
        if (!chat())
            return 1;
        break;
    case 'q': case 'Q':
        sysop_quit = 1;
        /* fall through: end the current call */
    case 'k': case 'K':
        kick();
        return 1;
    }
    return 0;
}

/* ---- paging (Hauptmenue 9) -------------------------------------------- */

void sysop_page(void)
{
    clock_t start;
    int k;

    if (!sysop_present) {
        term_puts(L_SYSOP_NOT_HERE);
        pm_to_sysop();
        return;
    }
    term_puts(L_PAGE_ASK);
    if ((k = term_getc()) == KEY_GONE)
        return;
    term_putc('\n');
    if (!term_key(k, L_KEY_YES)) {
        pm_to_sysop();
        return;
    }
    term_puts(L_PAGING);
    puts(L_SYS_PAGED);
    start = clock();
    while (ELAPSED(start) < PAGE_SECONDS * CLOCKS_PER_SEC) {
        sound_page();
        term_putc('.');
        net_flush();
        k = sysop_key();
        if (k == 'c' || k == 'C') {
            chat();
            return;
        }
        if (k == 'k' || k == 'K') {
            kick();
            return;
        }
        if ((k = term_poll()) == KEY_GONE)
            return;
        if (k >= 0)
            break;
    }
    term_puts(L_NO_ANSWER);
    pm_to_sysop();
}
