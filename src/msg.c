/* Message bases shared by boards (bNN) and personal mailboxes (pNNN). */
#include <stdio.h>
#include <string.h>
#include "term.h"
#include "lang.h"
#include "store.h"
#include "user.h"
#include "msg.h"

#define MAX_LINES 40

Msg msg;

static char idx[32], dat[32];
static char line[41];
static char text[1700];         /* one message: 40 lines x up to 39 chars + LF */

void msg_use(char kind, uint16_t n)
{
    sprintf(idx, kind == 'b' ? BBS_DIR "b%02u.idx" : BBS_DIR "p%03u.idx", n);
    strcpy(dat, idx);
    strcpy(dat + strlen(dat) - 3, "dat");
}

uint16_t msg_count(void)
{
    return store_size(idx) / sizeof(Msg);
}

uint8_t msg_load(uint16_t no)
{
    return store_get(idx, (uint32_t) (no - 1) * sizeof msg, &msg, sizeof msg);
}

void msg_show(uint16_t no)
{
    uint16_t len, i;

    if (!msg_load(no))
        return;
    sprintf(line, L_MSG_HEAD, no, msg.from);
    term_puts(line);
    sprintf(line, L_MSG_DATE,
            msg.date[0], msg.date[1], msg.date[2], msg.date[3], msg.date[4]);
    term_puts(line);
    if (msg.flags & F_DELETED) {
        term_puts(L_MSG_GONE);
        return;
    }
    term_puts(L_MSG_SUBJECT);
    term_puts(msg.subject);
    term_puts("\n----------------------\n");
    len = msg.len < sizeof text ? msg.len : sizeof text;
    if (store_get(dat, msg.offset, text, len))
        for (i = 0; i < len && !term_abort; ++i)
            term_putc(text[i] == 0x0A ? '\n' : text[i]);
}

int msg_compose(const char *subject)
{
    uint16_t len = 0;
    uint8_t lines = 0;
    int r;

    if (subject) {
        strncpy(msg.subject, subject, sizeof msg.subject - 1);
        msg.subject[sizeof msg.subject - 1] = 0;
        term_puts(L_MSG_SUBJECT);
        term_puts(msg.subject);
        term_putc('\n');
    } else {
        term_puts(L_MSG_SUBJECT_PROMPT);
        if ((r = term_getline(msg.subject, sizeof msg.subject - 1, 0)) <= 0)
            return r;
    }
    term_puts(L_MSG_TEXT_HINT);
    for (;;) {
        sprintf(line, "%02u:", lines + 1);
        term_puts(line);
        r = term_getline(line, term_width > 40 ? 39 : term_width - 4, 0);
        if (r == KEY_GONE)
            return KEY_GONE;
        if (r == 1 && line[0] == '.')
            break;
        memcpy(text + len, line, r);
        len += r;
        text[len++] = 0x0A;
        if (++lines == MAX_LINES) {
            term_puts(L_MSG_FULL);
            break;
        }
    }
    if (!lines)
        return 0;
    term_puts(L_MSG_SAVE_ASK);
    r = term_getc();
    if (r == KEY_GONE)
        return KEY_GONE;
    term_putc('\n');
    return term_key(r, L_KEY_YES) ? len : 0;
}

uint16_t msg_save(uint16_t len)
{
    uint16_t n = msg_count();
    char subject[sizeof msg.subject];

    strcpy(subject, msg.subject);
    /* the new text goes behind the last one */
    msg.offset = 0;
    if (n && msg_load(n))
        msg.offset += msg.len;
    strcpy(msg.subject, subject);
    msg.len = len;
    strcpy(msg.from, user.name);
    clock_now();
    memcpy(msg.date, now, 5);
    msg.flags = 0;
    if (store_append(dat, text, len) && store_append(idx, &msg, sizeof msg))
        return n + 1;
    return 0;
}

uint8_t msg_delete(uint16_t no)
{
    if (!msg_load(no))
        return 0;
    msg.flags |= F_DELETED;
    return store_put(idx, (uint32_t) (no - 1) * sizeof msg, &msg, sizeof msg);
}
