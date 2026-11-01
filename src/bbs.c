/* Berlin Blues BBS - main loop (phase 1: terminal layer demo). */
#include <stdio.h>
#include <peekpoke.h>
#include "net.h"
#include "term.h"
#include "sound.h"

#define TEXTS "N2:SD:///bbs/texts/"

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

static const char *const modes[] = { "PC/Mac", "Schneider CPC", "C64", "Atari" };

static char line[40];
static unsigned calls;

static void session(void)
{
    int c, n;

    if (!term_detect())
        return;
    term_puts(banner);
    sprintf(line, "Terminal: %s, %u Zeichen\n", modes[term_mode], term_width);
    term_puts(line);
    sprintf(line, "Du bist Anrufer Nr. %u\n", ++calls);
    term_puts(line);

    for (;;) {
        term_page_reset();
        term_puts("\n      Testmenue\n"
                  "----------------------\n"
                  "1 Zeile eingeben\n"
                  "2 Langen Text zeigen\n"
                  "3 Passwort eingeben\n"
                  "----------------------\n"
                  "0=logoff\n\n=>");
        c = term_getc();
        if (c == KEY_GONE)
            return;
        term_putc((char) c);
        term_putc('\n');
        switch (c) {
        case '1':
            term_puts("Text: ");
            if ((n = term_getline(line, 30, 0)) == KEY_GONE)
                return;
            term_puts("Du schriebst: ");
            term_puts(line);
            term_putc('\n');
            break;
        case '2':
            if (!term_type_file(TEXTS "info.txt"))
                term_puts("Text nicht gefunden.\n");
            break;
        case '3':
            term_puts("Passwort ==>");
            if ((n = term_getline(line, 16, 1)) == KEY_GONE)
                return;
            sprintf(line, "%d Zeichen\n", n);
            term_puts(line);
            break;
        case '0':
            term_puts("\nBis zum naechsten Mal...\n");
            net_hangup();
            return;
        }
    }
}

int main(void)
{
    POKE(0x41, 0);              /* SOUNDR: no SIO beeping - FujiNet is polled all the time */
    putchar(0x7D);
    puts("Berlin Blues BBS");
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
        printf("\naufgelegt (%u Anrufe)\n", calls);
    }
}
