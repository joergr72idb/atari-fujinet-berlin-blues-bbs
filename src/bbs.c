/* Berlin Blues BBS - main loop. */
#include <stdio.h>
#include <string.h>
#include <peekpoke.h>
#include "ticks.h"
#include "net.h"
#include "term.h"
#include "store.h"
#include "user.h"
#include "board.h"
#include "menu.h"
#include "pm.h"
#include "sysop.h"

/* The banner comes from bbs/texts/banner.txt on the SD card (editable by the sysop);
   this one is only shown if the file is missing. */
static const char banner[] = "\nBERLIN BLUES BOX\n\n";

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
    sprintf(line, "Du bist Anrufer Nr. %lu\n", stats.total);
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
    puts("\n[Lokale Anmeldung]");
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
    puts("Berlin Blues BBS");
    user_init();
    printf("%u Menuepunkte\n", menu_load());
    /* 24/7: never give up on the listener - retry every 10 s (Q still quits) */
    while (net_listen()) {
        puts("FujiNet: listen failed - neuer Versuch in 10 s");
        start = clock();
        while (ELAPSED(start) < 10 * CLOCKS_PER_SEC)
            if (sysop_idle() == 'Q') {
                sysop_exit();
                return 0;
            }
    }
    while (!sysop_quit) {
        puts("\nwarte auf Anruf...");
        for (;;) {
            if (net_ringing()) {
                net_accept();
                puts("Anruf!");
                session();
                net_hangup();
                printf("\naufgelegt (%lu Anrufe)\n", stats.total);
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
    puts("\nBerlin Blues BBS beendet.");
    return 0;
}
