/* Terminal layer: caller's character set, line editor, paging.
   Text inside the BBS is ASCII with '\n' (= ATASCII EOL $9B in cc65) as line end;
   term converts it for the caller's machine. */
#ifndef TERM_H
#define TERM_H

#include <stdint.h>

enum { TERM_PC, TERM_CPC, TERM_C64, TERM_ATARI };

#define KEY_ENTER  '\n'
#define KEY_BS     0x08
#define KEY_GONE   -1           /* caller hung up or idle timeout */

extern uint8_t term_mode;
extern uint8_t term_width;
extern uint8_t term_abort;      /* caller answered "Q" at --Mehr-- */
extern uint8_t term_paging;

uint8_t term_detect(void);      /* ask for RETURN, pick the mode; 0 = caller gone */
void term_putc(char c);
void term_puts(const char *s);
void term_cls(void);
int  term_getc(void);           /* key in internal code, or KEY_GONE */
int  term_getline(char *buf, uint8_t max, uint8_t hidden);   /* length or KEY_GONE */
void term_page_reset(void);
uint8_t term_type_file(const char *spec);  /* show a text file from SD, 0 = not found */

#endif
