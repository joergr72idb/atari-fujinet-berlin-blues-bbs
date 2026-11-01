/* Message boards: bNN.idx (48-byte headers) + bNN.dat (text, LF line ends). */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define MAX_BOARDS 16

void board_login(void);                 /* load "last read" for the caller, show new counts */
void board_show(uint8_t b);             /* board menu with message counts */
uint8_t board_action(uint8_t b, char key);  /* 0 = key not a board action */
void board_new_all(void);               /* read all new messages in all boards */

#endif
