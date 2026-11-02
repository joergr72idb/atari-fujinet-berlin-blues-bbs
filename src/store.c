/* Record storage on the FujiNet SD card via N:SD: (no DOS needed).
   Write safety (24/7): wherever "file missing" leads to a write (create = OPEN_MODE_WRITE truncates!,
   new record numbers), a failed open is retried first - a FujiNet hiccup must not look like a missing
   file. Plain reads try once: missing board files are normal and every SIO call costs time. */
#include <string.h>
#include "fujinet-network.h"
#include "store.h"

#define TRIES 3

static uint8_t open_once(const char *spec, uint8_t mode, uint32_t pos)
{
    if (network_open(spec, mode, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    if (pos && network_seek(spec, pos) != FN_ERR_OK) {
        network_close(spec);
        return 0;
    }
    return 1;
}

static uint8_t open_sure(const char *spec, uint8_t mode, uint32_t pos)
{
    uint8_t i;

    for (i = 0; i < TRIES; ++i)
        if (open_once(spec, mode, pos))
            return 1;
    return 0;
}

static uint16_t size(const char *spec, uint8_t sure)
{
    uint16_t bw = 0;
    uint8_t conn, err;

    if (!(sure ? open_sure : open_once)(spec, OPEN_MODE_READ, 0))
        return 0;
    if (network_status(spec, &bw, &conn, &err) != FN_ERR_OK)
        bw = 0;
    network_close(spec);
    return bw;          /* the firmware caps "bytes waiting" at 65534 */
}

uint16_t store_size(const char *spec)
{
    return size(spec, 0);
}

uint16_t store_size_sure(const char *spec)
{
    return size(spec, 1);
}

uint8_t store_get(const char *spec, uint32_t pos, void *buf, uint16_t len)
{
    int16_t n;

    if (!open_once(spec, OPEN_MODE_READ, pos))
        return 0;
    n = network_read(spec, buf, len);
    network_close(spec);
    return n == (int16_t) len;
}

/* Overwrite len bytes at pos; creates the file if it doesn't exist yet. */
uint8_t store_put(const char *spec, uint32_t pos, const void *buf, uint16_t len)
{
    uint8_t ok;

    if (!open_sure(spec, OPEN_MODE_RW, pos)) {
        /* Missing file: create it empty, then try again (seek past the end pads with zeros).
           Only if a read open fails too - OPEN_MODE_WRITE would truncate an existing file. */
        if (open_sure(spec, OPEN_MODE_READ, 0)) {
            network_close(spec);
            return 0;
        }
        if (!open_once(spec, OPEN_MODE_WRITE, 0))
            return 0;
        network_close(spec);
        if (!open_sure(spec, OPEN_MODE_RW, pos))
            return 0;
    }
    ok = network_write(spec, buf, len) == FN_ERR_OK;
    network_close(spec);
    return ok;
}

/* store_put, then read it back: the firmware buffers writes, so a write "OK" alone proves little. */
uint8_t store_put_check(const char *spec, uint32_t pos, const void *buf, uint16_t len)
{
    static uint8_t back[48];
    uint16_t n;

    if (!store_put(spec, pos, buf, len))
        return 0;
    n = len < sizeof back ? len : sizeof back;      /* the record head is enough */
    return store_get(spec, pos, back, n) && !memcmp(back, buf, n);
}

uint8_t store_delete(const char *spec)
{
    return network_fs_delete(spec) == FN_ERR_OK;
}

static const char *seq;
static char lbuf[64];
static uint8_t llen, lpos;

uint8_t store_open(const char *spec)
{
    seq = open_once(spec, OPEN_MODE_READ, 0) ? spec : 0;
    llen = lpos = 0;
    return seq != 0;
}

int store_gets(char *buf, uint8_t max)
{
    uint8_t n = 0, any = 0;
    int16_t r;
    char c;

    for (;;) {
        if (lpos == llen) {
            r = seq ? network_read_nb(seq, lbuf, sizeof lbuf) : 0;
            if (r <= 0) {
                buf[n] = 0;
                return any ? n : -1;
            }
            llen = (uint8_t) r;
            lpos = 0;
        }
        c = lbuf[lpos++];
        any = 1;
        if (c == 0x0A)
            break;
        if (c != 0x0D && n < max)
            buf[n++] = c;
    }
    buf[n] = 0;
    return n;
}

uint8_t store_next(void *buf, uint16_t len)
{
    return seq && network_read(seq, buf, len) == (int16_t) len;
}

void store_close(void)
{
    if (seq)
        network_close(seq);
    seq = 0;
}
