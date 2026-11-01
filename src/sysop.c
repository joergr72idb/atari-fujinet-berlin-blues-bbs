/* Sysop console on the Atari.
   Screen: own display list = 1 status line + 23 lines of the OS screen editor (E:, row 0
   hidden), so the session mirror keeps scrolling below a fixed status line.
   Keys: C chat, K kick, A sysop present/away, L local login (idle), Q quit. */
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <peekpoke.h>
#include "net.h"
#include "term.h"
#include "store.h"
#include "user.h"
#include "sound.h"
#include "pm.h"
#include "sysop.h"

#define SDLSTL  0x230
#define SAVMSC  0x58
#define LMARGN  0x52
#define CH      0x2FC
#define ESC     0x1B
#define PAGE_SECONDS 30

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
    putchar('\n');                          /* E: row 0 is hidden */
    sysop_status();
}

void sysop_exit(void)
{
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

    if (clock() - last_status < CLOCKS_PER_SEC / 2)
        return;
    last_status = clock();
    if (net_online() || term_local) {
        user_online(t);
        snprintf(status, sizeof status, " BLUES %-10.10s L%u %s %s",
                 user.name[0] ? user.name : term_local ? "(lokal)" : "(login)", user.level, t,
                 sysop_present ? "Sysop da" : "Sysop weg");
    } else {
        snprintf(status, sizeof status, " BLUES warte  Anrufe %lu  Sysop %s  L A Q",
                 stats.total, sysop_present ? "da" : "weg");
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
    if (clock() - tk_last < CLOCKS_PER_SEC)
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

int sysop_key(void)
{
    if (PEEK(CH) != 0xFF)
        return cgetc();
#ifdef TESTKEYS
    return test_key();
#else
    return -1;
#endif
}

static void toggle_present(void)
{
    sysop_present = !sysop_present;
    printf("\n[Sysop %s]\n", sysop_present ? "anwesend" : "abwesend");
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

/* Chat until the sysop presses ESC; 0 = caller gone. */
static uint8_t chat(void)
{
    int k;

    term_paging = 0;
    term_puts("\n*** Der Sysop ist da - Chat! ***\n");
    puts("[Chat - Ende mit ESC]");
    for (;;) {
        sysop_status();
        k = sysop_key();
        if (k == ESC)
            break;
        if (k == 0x9B)
            term_putc('\n');
        else if (k == 0x7E)
            term_putc(KEY_BS);
        else if (k >= 0x20 && k < 0x7D)
            term_putc((char) k);
        if (k >= 0)
            net_flush();
        k = term_poll();
        if (k == KEY_GONE) {
            term_paging = 1;
            return 0;
        }
        if (k >= 0)
            term_putc((char) k);
    }
    term_puts("\n*** Chat beendet ***\n");
    net_flush();
    term_paging = 1;
    return 1;
}

static void kick(void)
{
    term_paging = 0;
    term_puts("\n*** Der Sysop hat die Verbindung beendet. ***\n");
    net_hangup();
    puts("[gekickt]");
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
        term_puts("Der Sysop ist gerade nicht da.\n");
        pm_to_sysop();
        return;
    }
    term_puts("Sysop zum Chat rufen (J/N)? ");
    if ((k = term_getc()) == KEY_GONE)
        return;
    term_putc('\n');
    if (k != 'j' && k != 'J') {
        pm_to_sysop();
        return;
    }
    term_puts("Ich rufe den Sysop (Taste = Abbruch)");
    puts("\n[Sysop gerufen - C = Chat]");
    start = clock();
    while (clock() - start < PAGE_SECONDS * CLOCKS_PER_SEC) {
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
    term_puts("\nKeine Antwort.\n");
    pm_to_sysop();
}
