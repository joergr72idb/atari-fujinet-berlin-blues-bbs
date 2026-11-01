/* Menu tree from bbs/menus.txt, navigation like the 1987 box:
   digits select, '*' = Hauptmenue, chains like "*42" work, RETURN = back, 0 = logoff.

   menus.txt, one item per line:  <menu> <key> <type> <arg> <level> <label>
     type M = submenu (arg = menu no.), B = board (arg = board no.),
          T = text file in bbs/texts/, F = function (status users passwd approve new pmwrite pmread sysop)
     level = minimum user level to see/use the item
   Menu 0 is the Hauptmenue; a submenu's title is the label of the item leading to it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"
#include "store.h"
#include "user.h"
#include "board.h"
#include "pm.h"
#include "menu.h"

#define MENUS     BBS_DIR "menus.txt"
#define MAX_ITEMS 48

typedef struct {
    uint8_t menu, key, type, level, num;
    char    arg[9];
    char    label[23];
} Item;

static Item items[MAX_ITEMS];
static uint8_t n_items;
static char line[48];

/* ---- loading --------------------------------------------------------- */

static char *word(char **p)
{
    char *s = *p, *w;

    while (*s == ' ')
        ++s;
    w = s;
    while (*s && *s != ' ')
        ++s;
    if (*s)
        *s++ = 0;
    *p = s;
    return w;
}

uint8_t menu_load(void)
{
    Item *it;
    char *p;

    n_items = 0;
    if (!store_open(MENUS))
        return 0;
    while (n_items < MAX_ITEMS && store_gets(line, sizeof line - 1) >= 0) {
        if (!line[0] || line[0] == '#')
            continue;
        it = &items[n_items];
        p = line;
        it->menu = atoi(word(&p));
        it->key = *word(&p);
        it->type = *word(&p);
        strncpy(it->arg, word(&p), sizeof it->arg - 1);
        it->num = atoi(it->arg);
        it->level = atoi(word(&p));
        while (*p == ' ')
            ++p;
        strncpy(it->label, p, sizeof it->label - 1);
        ++n_items;
    }
    store_close();
    return n_items;
}

const char *menu_board_name(uint8_t b)
{
    uint8_t i;

    for (i = 0; i < n_items; ++i)
        if (items[i].type == 'B' && items[i].num == b)
            return items[i].label;
    return 0;
}

/* ---- running --------------------------------------------------------- */

static Item *find(uint8_t menu, char key)
{
    uint8_t i;

    for (i = 0; i < n_items; ++i)
        if (items[i].menu == menu && items[i].key == key && user.level >= items[i].level)
            return &items[i];
    return 0;
}

static const char *title(uint8_t menu)
{
    uint8_t i;

    if (menu)
        for (i = 0; i < n_items; ++i)
            if (items[i].type == 'M' && items[i].num == menu)
                return items[i].label;
    return "Hauptmenue";
}

static uint8_t parent(uint8_t menu)
{
    uint8_t i;

    for (i = 0; i < n_items; ++i)
        if (items[i].type == 'M' && items[i].num == menu)
            return items[i].menu;
    return 0;
}

static void show(uint8_t menu)
{
    uint8_t i;

    sprintf(line, "\n      %s\n----------------------\n", title(menu));
    term_puts(line);
    for (i = 0; i < n_items; ++i)
        if (items[i].menu == menu && user.level >= items[i].level) {
            sprintf(line, "%c %s\n", items[i].key, items[i].label);
            term_puts(line);
        }
    term_puts("----------------------\n");
}

static void function(const char *f)
{
    if (!strcmp(f, "status"))       user_status();
    else if (!strcmp(f, "users"))   user_list();
    else if (!strcmp(f, "passwd"))  user_change_pw();
    else if (!strcmp(f, "approve")) user_approve();
    else if (!strcmp(f, "new"))     board_new_all();
    else if (!strcmp(f, "pmwrite")) pm_write();
    else if (!strcmp(f, "pmread"))  pm_read();
    else if (!strcmp(f, "sysop"))   pm_to_sysop();
    else                            term_puts("Kommt bald.\n");
}

void menu_run(void)
{
    uint8_t menu = 0, board = 0, shown = 0, acted, i;
    Item *it;
    int n;

    for (;;) {
        term_page_reset();
        if (!shown) {
            if (board)
                board_show(board);
            else
                show(menu);
            term_puts("0=logoff  *=Hauptmenue\n");
            shown = 1;
        }
        user_online(line);
        term_puts("\n");
        term_puts(line);
        term_puts(" =>");
        if ((n = term_getline(line, 20, 0)) == KEY_GONE)
            return;
        if (n == 0) {                   /* RETURN: one level back */
            if (board)
                board = 0;
            else
                menu = parent(menu);
            shown = 0;
            continue;
        }
        acted = 0;
        for (i = 0; line[i] && !acted; ++i) {
            if (line[i] == '*') {
                menu = board = 0;
                continue;
            }
            if (line[i] == '0') {
                user_logoff();
                return;
            }
            if (board) {
                acted = board_action(board, line[i]);
                continue;
            }
            if (!(it = find(menu, line[i])))
                break;
            switch (it->type) {
            case 'M':
                menu = it->num;
                break;
            case 'B':
                board = it->num;
                break;
            case 'T':
                sprintf(line, BBS_DIR "texts/%s", it->arg);
                if (!term_type_file(line))
                    term_puts("Text nicht gefunden.\n");
                acted = 1;
                break;
            case 'F':
                function(it->arg);
                acted = 1;
                break;
            }
        }
        shown = acted;          /* after an action keep the prompt, else show the new menu */
    }
}
