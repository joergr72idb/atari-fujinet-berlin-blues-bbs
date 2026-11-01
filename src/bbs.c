/* Berlin Blues BBS - main loop. */
#include <stdio.h>
#include <string.h>
#include <peekpoke.h>
#include "net.h"
#include "term.h"
#include "store.h"
#include "user.h"
#include "board.h"
#include "menu.h"
#include "pm.h"
#include "sysop.h"

static const char banner[] =
    "\n"
    "  ***  **** ***  **   **** *  *\n"
    "  *  * *    *  * **    **  *  *\n"
    "  ***  **** ***  **    **  ** *\n"
    "  *  * *    * *  **    **  * **\n"
    "  ***  **** *  * **** **** *  *\n"
    "\n"
    "      ***\n"
    "      *  * *   * * *** ***\n"
    "      ***  *   * * *   **\n"
    "      *  * *   * * **   **\n"
    "      ***  ***  *  *** **Box-System\n"
    "\n";

static char line[41];

static void session(void)
{
    memset(&user, 0, sizeof user);
    user_no = 0;
    if (!term_detect())
        return;
    stats_count_call();
    term_puts(banner);
    sprintf(line, "Du bist Anrufer Nr. %lu\n", stats.total);
    term_puts(line);
    if (!user_login())
        return;
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

    POKE(0x41, 0);              /* SOUNDR: no SIO beeping - FujiNet is polled all the time */
    sysop_init();
    puts("Berlin Blues BBS");
    user_init();
    printf("%u Menuepunkte\n", menu_load());
    if (net_listen()) {
        puts("FujiNet: listen failed");
        return 1;
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
