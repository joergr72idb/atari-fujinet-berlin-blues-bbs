/* Record storage on the FujiNet SD card (N2:SD:///bbs/...). The only module that
   knows the backend, so it can later be switched to TNFS or HTTP. */
#ifndef STORE_H
#define STORE_H

#include <stdint.h>

#define BBS_DIR "N2:SD:///bbs/"

uint16_t store_size(const char *spec);      /* file size, 0 if missing */
uint8_t store_get(const char *spec, uint32_t pos, void *buf, uint16_t len);
uint8_t store_put(const char *spec, uint32_t pos, const void *buf, uint16_t len);
uint8_t store_append(const char *spec, const void *buf, uint16_t len);
uint8_t store_delete(const char *spec);

/* Sequential reading: one open, then record after record. */
uint8_t store_open(const char *spec);
uint8_t store_next(void *buf, uint16_t len);   /* 0 at end of file */
int  store_gets(char *buf, uint8_t max);        /* next text line (LF/CR stripped), -1 at end */
void store_close(void);

#endif
