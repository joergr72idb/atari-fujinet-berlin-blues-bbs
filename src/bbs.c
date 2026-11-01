/* Berlin Blues BBS - main loop. */
#include <stdio.h>
#include <peekpoke.h>
#include "net.h"
#include "term.h"
#include "store.h"
#include "user.h"
#include "board.h"
#include "menu.h"
#include "pm.h"

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

int main(void)
{
    POKE(0x41, 0);              /* SOUNDR: no SIO beeping - FujiNet is polled all the time */
    putchar(0x7D);
    puts("Berlin Blues BBS");
    user_init();
    printf("%u Menuepunkte\n", menu_load());
    if (net_listen()) {
        puts("FujiNet: listen failed");
        return 1;
    }
    for (;;) {
        puts("\nwarte auf Anruf...");
        net_wait_call();
        puts("Anruf!");
        session();
        net_hangup();
        printf("\naufgelegt (%lu Anrufe)\n", stats.total);
    }
}
