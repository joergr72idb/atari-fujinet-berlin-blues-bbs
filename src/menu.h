/* Menu tree from bbs/menus.txt. */
#ifndef MENU_H
#define MENU_H

#include <stdint.h>

uint8_t menu_load(void);                    /* at startup; number of items, 0 = file missing */
const char *menu_board_name(uint8_t b);     /* 0 if the board isn't in the menus */
void menu_run(void);                        /* until logoff or caller gone */

#endif
