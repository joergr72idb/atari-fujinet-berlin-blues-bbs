/* Berlin Blues BBS - main loop. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <peekpoke.h>
#include "ticks.h"
#include "net.h"
#include "term.h"
#include "lang.h"
#include "store.h"
#include "user.h"
#include "board.h"
#include "menu.h"
#include "pm.h"
#include "sysop.h"

/* The banner comes from bbs/texts/banner.txt on the SD card (editable by the sysop);
   this one is only shown if the file is missing. */
static const char banner[] = L_BANNER_FALLBACK;

static char line[41];

/* bbs/bbs.cfg on the SD card: "key = value" lines, # comments. Read once at boot.
   port = TCP port for callers (default 6502; never 8000 = FujiNet-PC web UI). */
static void cfg_load(void)
{
    char *v, *e;
    long n;

    if (!store_open(BBS_DIR "bbs.cfg"))
        return;
    while (store_gets(line, sizeof line - 1) >= 0) {
        if (!line[0] || line[0] == '#')
            continue;
        if (!(v = strchr(line, '=')))
            v = line + strlen(line);
        else
            *v++ = 0;
        for (e = line + strlen(line); e > line && (e[-1] == ' ' || e[-1] == '\t'); )
            *--e = 0;                   /* "port  " -> "port" */
        while (*v == ' ' || *v == '\t')
            ++v;
        if (!strcmp(line, "port")) {
            n = atol(v);
            if (n >= 1 && n <= 65535L && n != 8000)
                net_set_port((uint16_t) n);
            else
                printf(L_SYS_CFG_BAD, line, v);
        } else
            printf(L_SYS_CFG_UNKNOWN, line);
    }
    store_close();
}

static void session(void)
{
    memset(&user, 0, sizeof user);
    user_no = 0;
    term_limits(LOGIN_IDLE, LOGIN_TOTAL);
    if (!term_detect())
        return;
    stats_count_call();
    term_putc('\n');
    if (!term_type_file(BBS_DIR "texts/banner.txt"))
        term_puts(banner);
    sprintf(line, L_CALLER_NO, stats.total);
    term_puts(line);
    if (!user_login())
        return;
    term_limits(IDLE_SECONDS, 0);
    printf("[%s, Level %u]\n", user.name, user.level);
    user_status();
    pm_login();
    board_login();
    menu_run();
}

/* Sysop presses L: the BBS on the Atari's own keyboard and screen. */
static void local_session(void)
{
    memset(&user, 0, sizeof user);
    user_no = 0;
    term_set_local();
    user_clock_start();
    puts(L_SYS_LOCAL_LOGIN);
    if (user_login()) {
        user_status();
        pm_login();
        board_login();
        menu_run();
    }
    term_local = 0;
}

int main(void)
{
    char k;
    clock_t start;

    POKE(0x41, 0);              /* SOUNDR: no SIO beeping - FujiNet is polled all the time */
#ifdef TESTWRAP
    /* test builds: RTCLOK ($12-$14, MSB first) wraps to 0 in 4500 ticks (~90 s PAL) */
    POKE(0x14, 0); POKE(0x13, 0xEE); POKE(0x12, 0xFF); POKE(0x14, 0x6C);
#endif
    sysop_init();
    puts(L_SYS_TITLE);
    /* SD first: after a cold start FujiNet may still be coming up. user_init() would take an
       unreadable users.dat for a missing one and set up a new SYSOP with the default password. */
    while (!(k = menu_load())) {
        puts(L_SYS_SD_WAIT);
        start = clock();
        while (ELAPSED(start) < 5 * CLOCKS_PER_SEC)
            if (sysop_idle() == 'Q') {
                sysop_exit();
                return 0;
            }
    }
    printf(L_SYS_MENU_ITEMS, k);
    cfg_load();
    printf(L_SYS_PORT, net_port);
    user_init();
    /* 24/7: never give up on the listener - retry every 10 s (Q still quits) */
    while (net_listen()) {
        puts(L_SYS_LISTEN_RETRY);
        start = clock();
        while (ELAPSED(start) < 10 * CLOCKS_PER_SEC)
            if (sysop_idle() == 'Q') {
                sysop_exit();
                return 0;
            }
    }
    while (!sysop_quit) {
        puts(L_SYS_WAITING);
        for (;;) {
            if (net_ringing()) {
                net_accept();
                puts(L_SYS_RING);
                session();
                net_hangup();
                printf(L_SYS_HUNGUP, stats.total);
                break;
            }
            k = sysop_idle();
            if (k == 'L') {
                local_session();
                break;
            }
            if (k == 'Q') {
                sysop_quit = 1;
                break;
            }
        }
    }
    net_close();
    sysop_exit();
    puts(L_SYS_STOPPED);
    return 0;
}
