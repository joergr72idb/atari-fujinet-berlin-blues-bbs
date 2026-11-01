/* Message bases (boards and personal mailboxes): xNN.idx = 48-byte headers
   (msg no. = index + 1), xNN.dat = texts back to back with LF line ends. */
#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include "user.h"

#define F_DELETED  0x01

typedef struct {
    uint32_t offset;            /* in .dat */
    uint16_t len;
    char     from[NAME_LEN + 1];
    uint8_t  date[5];           /* D, M, Y, h, m */
    uint8_t  flags;
    char     subject[20];
} Msg;

extern Msg msg;                 /* header of the last loaded/shown message */

void msg_use(char kind, uint16_t n);    /* 'b' = board n, 'p' = mailbox of user n */
uint16_t msg_count(void);
uint8_t msg_load(uint16_t no);          /* header into msg */
void msg_show(uint16_t no);
int  msg_compose(const char *subject);  /* subject preset or 0; text length, 0 = cancelled, KEY_GONE */
uint16_t msg_save(uint16_t len);        /* append; new msg no., 0 = error */
uint8_t msg_delete(uint16_t no);

#endif
