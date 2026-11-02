/* Berlin Blues BBS - main loop. */
#include <stdio.h>
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
    user_init();
    printf(L_SYS_MENU_ITEMS, menu_load());
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
