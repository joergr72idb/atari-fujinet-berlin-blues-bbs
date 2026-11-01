/* Caller connection: one TCP listener on N1 (port 6502), one caller at a time. */
#ifndef NET_H
#define NET_H

#include <stdint.h>

#define NET_NONE  -1    /* net_getc: no byte waiting */
#define NET_GONE  -2    /* net_getc: caller hung up */

extern uint8_t net_telnet;      /* filter telnet IAC sequences from input */
extern uint16_t net_relistens;  /* times the listener had to be reopened */

uint8_t net_listen(void);       /* open the listener, 0 = ok */
uint8_t net_ringing(void);      /* a caller is waiting to be accepted (reopens a lost listener) */
void net_accept(void);
uint8_t net_online(void);       /* a caller is connected */
int  net_getc(void);            /* byte, NET_NONE or NET_GONE */
void net_putc(uint8_t c);
void net_write(const void *buf, uint8_t len);
void net_flush(void);
void net_hangup(void);
void net_close(void);            /* close the listener (shutdown) */

#endif
