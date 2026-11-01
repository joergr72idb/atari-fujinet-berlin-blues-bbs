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
        if (pos || !open_at(spec, OPEN_MODE_WRITE, 0))
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

uint8_t store_open(const char *spec)
{
    seq = open_at(spec, OPEN_MODE_READ, 0) ? spec : 0;
    return seq != 0;
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
