/* Sysop console on the Atari: status line, local keys, chat, paging. */
#ifndef SYSOP_H
#define SYSOP_H

#include <stdint.h>

extern uint8_t sysop_present;   /* "Sysop ist: anwesend" - toggled with A */
extern uint8_t sysop_quit;      /* Q pressed: shut down after the current call */

void sysop_init(void);          /* status line display list, 40 columns */
void sysop_exit(void);          /* restore the normal screen */
int  sysop_key(void);           /* local key (ATASCII) or -1 */
void sysop_status(void);        /* refresh the status line (cheap, call often) */
char sysop_idle(void);          /* while waiting for a call: 'L' local login, 'Q' quit, 0 */
uint8_t sysop_poll(void);       /* during a call: C/K/A/Q keys; 1 = caller was kicked */
void sysop_page(void);          /* caller pages the sysop (Hauptmenue 9) */

#endif
