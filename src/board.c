/* Message boards bNN (storage in msg.c).
   lastread.dat: per user 16 x u16 = highest message read in each board. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"
#include "store.h"
#include "user.h"
#include "menu.h"
#include "msg.h"
#include "board.h"

#define LASTREAD BBS_DIR "lastread.dat"

static uint16_t lastread[MAX_BOARDS];
static char line[41];

static uint16_t count(uint8_t b)
{
    msg_use('b', b);
    return msg_count();
}

static void save_lastread(void)
{
    if (user_no)
        store_put(LASTREAD, (uint32_t) (user_no - 1) * sizeof lastread, lastread, sizeof lastread);
}

void board_login(void)
{
    uint8_t b, any = 0;
    uint16_t n;

    memset(lastread, 0, sizeof lastread);
    if (user_no)
        store_get(LASTREAD, (uint32_t) (user_no - 1) * sizeof lastread, lastread, sizeof lastread);
    term_puts("\nNeue Msg: ");
    for (b = 1; b <= MAX_BOARDS; ++b) {
        if (!menu_board_name(b))
            continue;
        n = count(b);
        if (n > lastread[b - 1]) {
            sprintf(line, "%s%s (%u)", any ? ", " : "", menu_board_name(b), n - lastread[b - 1]);
            term_puts(line);
            any = 1;
        }
    }
    term_puts(any ? "\n" : "keine\n");
}

void board_show(uint8_t b)
{
    uint16_t n = count(b);

    sprintf(line, "\n   %s\n----------------------\n", menu_board_name(b));
    term_puts(line);
    sprintf(line, "%u Msg, %u neu\n", n, n > lastread[b - 1] ? n - lastread[b - 1] : 0);
    term_puts(line);
    term_puts("1 Neue Msg lesen\n"
              "2 Alle Msg lesen\n"
              "3 Msg Nr. lesen\n"
              "4 Msg schreiben\n");
    if (user.level >= LVL_SYSOP)
        term_puts("5 Msg loeschen\n");
    term_puts("----------------------\n");
}

/* msg_use() for board b is already set */
static void read_range(uint8_t b, uint16_t from, uint16_t to)
{
    for (; from <= to && !term_abort; ++from) {
        msg_show(from);
        if (from > lastread[b - 1])
            lastread[b - 1] = from;
    }
    save_lastread();
    if (from <= to)
        term_puts("\n(abgebrochen)\n");
}

static uint16_t ask_no(uint16_t n)
{
    uint16_t no;

    term_puts("Nr. ==>");
    if (term_getline(line, 5, 0) <= 0)
        return 0;
    no = atoi(line);
    if (no < 1 || no > n) {
        term_puts("Gibt es nicht.\n");
        return 0;
    }
    return no;
}

static void write_msg(uint8_t b)
{
    int len;
    uint16_t no;

    if (user.level < LVL_MEMBER) {
        term_puts(user_no ? "Du bist noch nicht freigeschaltet.\n"
                          : "Gaeste koennen nur lesen.\n");
        return;
    }
    if ((len = msg_compose(0)) <= 0)
        return;
    msg_use('b', b);
    if ((no = msg_save(len)) != 0) {
        sprintf(line, "Msg Nr. %u gespeichert.\n", no);
        term_puts(line);
        lastread[b - 1] = no;           /* own message isn't "new" */
        save_lastread();
    } else {
        term_puts("Fehler beim Speichern.\n");
    }
}

uint8_t board_action(uint8_t b, char key)
{
    uint16_t n = count(b), no;

    switch (key) {
    case '1':
        if (n > lastread[b - 1])
            read_range(b, lastread[b - 1] + 1, n);
        else
            term_puts("Keine neuen Msg.\n");
        return 1;
    case '2':
        read_range(b, 1, n);
        return 1;
    case '3':
        if ((no = ask_no(n)) != 0)
            read_range(b, no, no);
        return 1;
    case '4':
        write_msg(b);
        return 1;
    case '5':
        if (user.level < LVL_SYSOP)
            return 0;
        if ((no = ask_no(n)) != 0)
            term_puts(msg_delete(no) ? "Geloescht.\n" : "Fehler.\n");
        return 1;
    }
    return 0;
}

void board_new_all(void)
{
    uint8_t b, any = 0;
    uint16_t n;

    for (b = 1; b <= MAX_BOARDS && !term_abort; ++b) {
        if (!menu_board_name(b))
            continue;
        n = count(b);
        if (n <= lastread[b - 1])
            continue;
        sprintf(line, "\n=== %s ===\n", menu_board_name(b));
        term_puts(line);
        read_range(b, lastread[b - 1] + 1, n);
        any = 1;
    }
    if (!any)
        term_puts("Keine neuen Msg.\n");
}
