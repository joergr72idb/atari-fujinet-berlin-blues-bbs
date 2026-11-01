/* Caller connection: one TCP listener on N1 (port 6502), one caller at a time. */
#ifndef NET_H
#define NET_H

#include <stdint.h>

#define NET_NONE  -1    /* net_getc: no byte waiting */
#define NET_GONE  -2    /* net_getc: caller hung up */

extern uint8_t net_telnet;      /* filter telnet IAC sequences from input */

uint8_t net_listen(void);       /* open the listener, 0 = ok */
void net_wait_call(void);       /* block until a caller is accepted */
int  net_getc(void);            /* byte, NET_NONE or NET_GONE */
void net_putc(uint8_t c);
void net_write(const void *buf, uint8_t len);
void net_flush(void);
void net_hangup(void);

#endif
