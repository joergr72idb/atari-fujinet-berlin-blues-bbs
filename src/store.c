/* Record storage on the FujiNet SD card via N:SD: (no DOS needed). */
#include "fujinet-network.h"
#include "store.h"

static uint8_t open_at(const char *spec, uint8_t mode, uint32_t pos)
{
    if (network_open(spec, mode, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    if (pos && network_seek(spec, pos) != FN_ERR_OK) {
        network_close(spec);
        return 0;
    }
    return 1;
}

uint16_t store_size(const char *spec)
{
    uint16_t bw = 0;
    uint8_t conn, err;

    if (!open_at(spec, OPEN_MODE_READ, 0))
        return 0;
    if (network_status(spec, &bw, &conn, &err) != FN_ERR_OK)
        bw = 0;
    network_close(spec);
    return bw;
}

uint8_t store_get(const char *spec, uint32_t pos, void *buf, uint16_t len)
{
    int16_t n;

    if (!open_at(spec, OPEN_MODE_READ, pos))
        return 0;
    n = network_read(spec, buf, len);
    network_close(spec);
    return n == (int16_t) len;
}

/* Overwrite len bytes at pos; creates the file if it doesn't exist yet. */
uint8_t store_put(const char *spec, uint32_t pos, const void *buf, uint16_t len)
{
    uint8_t ok;

    if (!open_at(spec, OPEN_MODE_RW, pos)) {
        /* missing file: create it empty, then try again (seek past the end pads with zeros) */
        if (!open_at(spec, OPEN_MODE_WRITE, 0))
            return 0;
        network_close(spec);
        if (!open_at(spec, OPEN_MODE_RW, pos))
            return 0;
    }
    ok = network_write(spec, buf, len) == FN_ERR_OK;
    network_close(spec);
    return ok;
}

uint8_t store_append(const char *spec, const void *buf, uint16_t len)
{
    uint8_t ok;

    if (!open_at(spec, OPEN_MODE_APPEND, 0))
        return 0;
    ok = network_write(spec, buf, len) == FN_ERR_OK;
    network_close(spec);
    return ok;
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
    seq = open_at(spec, OPEN_MODE_READ, 0) ? spec : 0;
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
