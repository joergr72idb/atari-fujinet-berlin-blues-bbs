/* Berlin Blues BBS - main loop (phase 2: users + login). */
#include <stdio.h>
#include <peekpoke.h>
#include "net.h"
#include "term.h"
#include "store.h"
#include "user.h"

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

static void menu(void)
{
    int c;

    for (;;) {
        term_page_reset();
        term_puts("\n      Hauptmenue\n"
                  "----------------------\n"
                  "1 Status\n"
                  "2 Liste der Benutzer\n"
                  "3 Passwort aendern\n"
                  "4 Info-Text\n");
        if (user.level >= LVL_SYSOP)
            term_puts("9 Freischalten (Sysop)\n");
        term_puts("----------------------\n0=logoff\n\n=>");
        c = term_getc();
        if (c == KEY_GONE)
            return;
        term_putc((char) c);
        term_putc('\n');
        switch (c) {
        case '1': user_status(); break;
        case '2': user_list(); break;
        case '3': user_change_pw(); break;
        case '4':
            if (!term_type_file(BBS_DIR "texts/info.txt"))
                term_puts("Text nicht gefunden.\n");
            break;
        case '9': user_approve(); break;
        case '0':
            user_logoff();
            return;
        }
    }
}

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
    menu();
}

int main(void)
{
    POKE(0x41, 0);              /* SOUNDR: no SIO beeping - FujiNet is polled all the time */
    putchar(0x7D);
    puts("Berlin Blues BBS");
    user_init();
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
