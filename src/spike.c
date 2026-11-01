/* Phase 0 spike: TCP server on port 6502 (echo, telnet IAC filter) and
   file access on the FujiNet SD card via N:SD: (write, append, seek). */
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "fujinet-network.h"

#define LISTEN "N1:TCP://:6502/"
#define FILE_  "N2:SD:///bbs/spike.txt"

static char buf[80];
static const unsigned char telnet_init[] = { 255, 251, 1, 255, 251, 3 }; /* WILL ECHO, WILL SGA */
static unsigned calls;

static void file_test(void)
{
    int16_t n;

    printf("SD write: %d\n", network_open(FILE_, OPEN_MODE_WRITE, OPEN_TRANS_NONE));
    network_write(FILE_, "Hello ", 6);
    network_close(FILE_);
    printf("SD append: %d\n", network_open(FILE_, OPEN_MODE_APPEND, OPEN_TRANS_NONE));
    network_write(FILE_, "Blues!", 6);
    network_close(FILE_);
    printf("SD read: %d\n", network_open(FILE_, OPEN_MODE_READ, OPEN_TRANS_NONE));
    printf("seek 6: %d\n", network_seek(FILE_, 6));
    n = network_read(FILE_, buf, 6);
    network_close(FILE_);
    buf[n > 0 ? n : 0] = 0;
    printf("read %d: '%s'\n", n, buf);
}

static void send(const char *s)
{
    network_write(LISTEN, s, strlen(s));
}

static void session(void)
{
    uint16_t bw;
    uint8_t conn, err, i, o, iac = 0;
    int16_t n;

    network_write(LISTEN, telnet_init, sizeof telnet_init);
    sprintf(buf, "\015\012BERLIN BLUES spike - Anrufer Nr. %u\015\012Q = auflegen\015\012", ++calls);
    send(buf);
    for (;;) {
        if (network_status(LISTEN, &bw, &conn, &err) != FN_ERR_OK || !conn)
            return;
        if (!bw)
            continue;
        n = network_read_nb(LISTEN, buf, bw < sizeof buf ? bw : sizeof buf);
        for (i = o = 0; n > 0 && i < (uint8_t) n; ++i) {
            if (iac) {                      /* skip IAC cmd [opt] */
                if (buf[i] < 251 || --iac == 0) iac = 0;
                continue;
            }
            if ((uint8_t) buf[i] == 255) { iac = 2; continue; }
            if (buf[i] == 'q' || buf[i] == 'Q') { send("\015\012Tschuess!\015\012"); return; }
            buf[o++] = buf[i];
            cputc(buf[i] == '\r' ? '\n' : buf[i]);
        }
        if (o)
            network_write(LISTEN, buf, o);
    }
}

int main(void)
{
    uint16_t bw;
    uint8_t conn, err;

    clrscr();
    printf("Berlin Blues spike\n\n");
    file_test();

    printf("\nlisten: %d\n", network_open(LISTEN, OPEN_MODE_RW, OPEN_TRANS_NONE));
    for (;;) {
        printf("warte auf Anruf...\n");
        do {
            if (network_status(LISTEN, &bw, &conn, &err) != FN_ERR_OK) conn = 0;
        } while (!conn);
        printf("accept: %d\n", network_accept(LISTEN));
        session();
        network_close_client(LISTEN);
        printf("\naufgelegt (%u Anrufe)\n", calls);
    }
}
