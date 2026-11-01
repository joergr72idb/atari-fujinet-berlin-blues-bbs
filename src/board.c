/* Message boards. Per board: bNN.idx = fixed 48-byte headers (msg no. = index + 1),
   bNN.dat = message texts back to back, lines end with LF (readable on Linux).
   lastread.dat: per user 16 x u16 = highest message read in each board. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"
#include "store.h"
#include "user.h"
#include "menu.h"
#include "board.h"

#define LASTREAD   BBS_DIR "lastread.dat"
#define MAX_LINES  40
#define F_DELETED  0x01

typedef struct {
    uint32_t offset;            /* in bNN.dat */
    uint16_t len;
    char     from[NAME_LEN + 1];
    uint8_t  date[5];           /* D, M, Y, h, m */
    uint8_t  flags;
    char     subject[20];
} Msg;

static Msg m;
static uint16_t lastread[MAX_BOARDS];
static char idx[32], dat[32];
static char line[41];
static char text[1700];         /* one message: 40 lines x up to 39 chars + LF */

static void files(uint8_t b)
{
    sprintf(idx, BBS_DIR "b%02u.idx", b);
    sprintf(dat, BBS_DIR "b%02u.dat", b);
}

static uint16_t count(uint8_t b)
{
    files(b);
    return store_size(idx) / sizeof(Msg);
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

/* Show message no in board b (files() already set). */
static void show(uint8_t b, uint16_t no)
{
    uint16_t len, i;

    if (!store_get(idx, (uint32_t) (no - 1) * sizeof m, &m, sizeof m))
        return;
    sprintf(line, "\nMsg %u von %s", no, m.from);
    term_puts(line);
    sprintf(line, "  %02u.%02u.%02u %02u:%02u\n", m.date[0], m.date[1], m.date[2], m.date[3], m.date[4]);
    term_puts(line);
    if (m.flags & F_DELETED) {
        term_puts("(geloescht)\n");
    } else {
        term_puts("Betreff: ");
        term_puts(m.subject);
        term_puts("\n----------------------\n");
        len = m.len < sizeof text ? m.len : sizeof text;
        if (store_get(dat, m.offset, text, len))
            for (i = 0; i < len && !term_abort; ++i)
                term_putc(text[i] == 0x0A ? '\n' : text[i]);
    }
    if (no > lastread[b - 1])
        lastread[b - 1] = no;
}

static void read_range(uint8_t b, uint16_t from, uint16_t to)
{
    for (; from <= to && !term_abort; ++from)
        show(b, from);
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
    uint16_t n, len = 0;
    uint8_t lines = 0;
    int r;

    if (user.level < LVL_MEMBER) {
        term_puts(user_no ? "Du bist noch nicht freigeschaltet.\n"
                          : "Gaeste koennen nur lesen.\n");
        return;
    }
    term_puts("Betreff ==>");
    if (term_getline(m.subject, sizeof m.subject - 1, 0) <= 0)
        return;
    term_puts("Text, max. 40 Zeilen. Ende: '.' allein\n");
    for (;;) {
        sprintf(line, "%02u:", lines + 1);
        term_puts(line);
        r = term_getline(line, term_width > 40 ? 39 : term_width - 4, 0);
        if (r == KEY_GONE)
            return;
        if (r == 1 && line[0] == '.')
            break;
        memcpy(text + len, line, r);
        len += r;
        text[len++] = 0x0A;
        if (++lines == MAX_LINES) {
            term_puts("(40 Zeilen voll)\n");
            break;
        }
    }
    if (!lines)
        return;
    term_puts("Speichern (J/N)? ");
    r = term_getc();
    if (r == KEY_GONE)
        return;
    term_putc('\n');
    if (r != 'j' && r != 'J')
        return;
    /* the new text goes behind the last one */
    n = count(b);
    m.offset = 0;
    if (n && store_get(idx, (uint32_t) (n - 1) * sizeof m, &m, sizeof m))
        m.offset += m.len;
    m.len = len;
    strcpy(m.from, user.name);
    clock_now();
    memcpy(m.date, now, 5);
    m.flags = 0;
    if (store_append(dat, text, len) && store_append(idx, &m, sizeof m)) {
        sprintf(line, "Msg Nr. %u gespeichert.\n", n + 1);
        term_puts(line);
        lastread[b - 1] = n + 1;        /* own message isn't "new" */
        save_lastread();
    } else {
        term_puts("Fehler beim Speichern.\n");
    }
}

static void delete_msg(uint8_t b)
{
    uint16_t no = ask_no(count(b));

    if (!no || !store_get(idx, (uint32_t) (no - 1) * sizeof m, &m, sizeof m))
        return;
    m.flags |= F_DELETED;
    term_puts(store_put(idx, (uint32_t) (no - 1) * sizeof m, &m, sizeof m) ? "Geloescht.\n" : "Fehler.\n");
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
        delete_msg(b);
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
