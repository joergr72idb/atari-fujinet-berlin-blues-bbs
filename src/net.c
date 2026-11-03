/* Caller connection over FujiNet: TCP listener on port 6502 (test builds: BBS_PORT=6503, bbs.cfg: port = n),
   buffered I/O. */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ticks.h"
#include "fujinet-network.h"
#include "net.h"
#include "lang.h"

#ifndef BBS_PORT
#define BBS_PORT 6502
#endif
#define STR_(x) #x
#define STR(x) STR_(x)

/* listener spec; the port can be changed at boot by bbs.cfg (net_set_port) */
static char LISTEN[24] = "N1:TCP://:" STR(BBS_PORT) "/";
uint16_t net_port = BBS_PORT;

void net_set_port(uint16_t port)
{
    net_port = port;
    sprintf(LISTEN, "N1:TCP://:%u/", port);
}

uint8_t net_telnet = 1;

static uint8_t rxbuf[64];
static uint8_t rxlen, rxpos;
static uint8_t txbuf[128];
static uint8_t txlen;
static uint8_t iac;             /* bytes of an IAC sequence still to skip */
static uint8_t gone;
static clock_t last_poll;

/* Polls that find nothing are spaced out: every SIO call blocks the sysop's keyboard. */
#define POLL_TICKS  (CLOCKS_PER_SEC / 8)

uint16_t net_relistens;

uint8_t net_listen(void)
{
    return network_open(LISTEN, OPEN_MODE_RW, OPEN_TRANS_NONE);
}

static uint8_t online;
static uint8_t errs;            /* listener status errors in a row */

/* The listener is gone (FujiNet restarted, network trouble): open it again. */
static void relisten(void)
{
    errs = 0;
    network_close(LISTEN);
    if (net_listen() == FN_ERR_OK) {
        ++net_relistens;
        printf(L_SYS_RELISTEN, net_relistens);
    }
}

uint8_t net_ringing(void)
{
    uint16_t bw;
    uint8_t conn, err;

    if (ELAPSED(last_poll) < POLL_TICKS * 2)
        return 0;
    last_poll = clock();
    /* An SIO error, or err 207 (NOT_CONNECTED) = FujiNet has no listener on N1 (e.g. it
       restarted); a healthy idle listener reports its last real error (1 = ok) instead. */
    if (network_status(LISTEN, &bw, &conn, &err) != FN_ERR_OK || (!conn && err == 207)) {
        if (++errs >= 4)
            relisten();
        return 0;
    }
    errs = 0;
    return conn;
}

void net_accept(void)
{
    network_accept(LISTEN);
    rxlen = rxpos = txlen = iac = gone = 0;
    net_telnet = 1;
    online = 1;
}

uint8_t net_online(void)
{
    return online && !gone;
}

/* Refill the receive buffer; returns 0 if nothing is waiting. */
static uint8_t fill(void)
{
    uint16_t bw;
    uint8_t conn, err;
    int16_t n;

    if (ELAPSED(last_poll) < POLL_TICKS)
        return 0;
    if (network_status(LISTEN, &bw, &conn, &err) != FN_ERR_OK || !conn) {
        gone = 1;
        return 0;
    }
    if (!bw) {
        last_poll = clock();
        return 0;
    }
    n = network_read_nb(LISTEN, rxbuf, bw < sizeof rxbuf ? bw : sizeof rxbuf);
    if (n <= 0)
        return 0;
    rxlen = (uint8_t) n;
    rxpos = 0;
    return 1;
}

int net_getc(void)
{
    uint8_t c;

    for (;;) {
        if (gone)
            return NET_GONE;
        if (rxpos == rxlen) {
            net_flush();
            if (!fill())
                return gone ? NET_GONE : NET_NONE;
        }
        c = rxbuf[rxpos++];
        if (net_telnet) {
            /* IAC cmd [opt]: WILL/WONT/DO/DONT (251-254) carry an option byte */
            if (iac) {
                iac = (iac == 2 && c >= 251 && c <= 254) ? 1 : 0;
                continue;
            }
            if (c == 255) {
                iac = 2;
                continue;
            }
        }
        return c;
    }
}

void net_flush(void)
{
    if (txlen && !gone && network_write(LISTEN, txbuf, txlen) != FN_ERR_OK)
        gone = 1;                   /* caller vanished while we were sending */
    txlen = 0;
}

void net_putc(uint8_t c)
{
    if (!online)
        return;                     /* no caller (e.g. local sysop session) */
    txbuf[txlen++] = c;
    if (txlen == sizeof txbuf)
        net_flush();
}

void net_write(const void *buf, uint8_t len)
{
    const uint8_t *p = buf;

    while (len--)
        net_putc(*p++);
}

void net_hangup(void)
{
    if (!online)
        return;
    net_flush();
    network_close_client(LISTEN);
    gone = 1;
    online = 0;
}

void net_close(void)
{
    network_close(LISTEN);
}
