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

/* Time limits in seconds. Before login they are short, so a silent bot or port scanner
   can't hold the single line for long; the test builds use shorter values (Makefile). */
#ifndef IDLE_SECONDS
#define IDLE_SECONDS   300      /* logged in: hang up after 5 minutes without a key */
#endif
#ifndef LOGIN_IDLE
#define LOGIN_IDLE      60      /* before login: 1 minute without a key */
#endif
#ifndef LOGIN_TOTAL
#define LOGIN_TOTAL    300      /* before login: 5 minutes from the call at most */
#endif

extern uint8_t term_mode;
extern uint8_t term_width;
extern uint8_t term_abort;      /* caller answered "Q" at --Mehr-- */
extern uint8_t term_paging;
extern uint8_t term_local;      /* local sysop session on the Atari keyboard/screen */

uint8_t term_detect(void);      /* ask for RETURN, pick the mode; 0 = caller gone */
void term_putc(char c);
void term_puts(const char *s);
void term_cls(void);
int  term_getc(void);           /* key in internal code, or KEY_GONE */
int  term_poll(void);           /* caller key without waiting: key, -2 = none, KEY_GONE */
void term_set_local(void);
void term_limits(uint16_t idle, uint16_t total);  /* idle / total seconds from now, total 0 = none */
int  term_getline(char *buf, uint8_t max, uint8_t hidden);   /* length or KEY_GONE */
void term_page_reset(void);
uint8_t term_type_file(const char *spec);  /* show a text file from SD, 0 = not found */

#endif
