/* Personal messages ("PMsg"): mailbox pNNN of user NNN. user.pm_read = highest
   message read. Members write to anyone; new users only to the sysop (user 1). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"
#include "lang.h"
#include "user.h"
#include "msg.h"
#include "pm.h"

#define SYSOP_NO 1

static char line[41];
static char to_name[NAME_LEN + 1];

void pm_login(void)
{
    uint16_t n;

    if (!user_no)
        return;
    msg_use('p', user_no);
    n = msg_count();
    if (n > user.pm_read) {
        sprintf(line, L_PM_NEW, n - user.pm_read);
        term_puts(line);
    } else {
        term_puts(L_PM_NONE);
    }
}

static void send_to(uint16_t to, const char *subject)
{
    int len;

    if ((len = msg_compose(subject)) <= 0)
        return;
    msg_use('p', to);
    sprintf(line, msg_save(len) ? L_PM_SENT : L_SAVE_ERROR, to_name);
    term_puts(line);
}

static uint8_t logged_in(void)
{
    if (!user_no)
        term_puts(L_PM_GUEST);
    return user_no != 0;
}

void pm_write(void)
{
    uint16_t to;

    if (!logged_in())
        return;
    if (user.level < LVL_MEMBER) {
        term_puts(L_PM_NOT_APPROVED);
        return;
    }
    term_puts(L_PM_TO);
    if (term_getline(line, NAME_LEN, 0) <= 0)
        return;
    if (!(to = user_find(line, to_name))) {
        term_puts(L_PM_UNKNOWN);
        return;
    }
    send_to(to, 0);
}

void pm_to_sysop(void)
{
    if (!logged_in())
        return;
    strcpy(to_name, "SYSOP");
    term_puts(L_PM_TO_SYSOP);
    send_to(SYSOP_NO, 0);
}

static void list(uint16_t n)
{
    uint16_t i;

    term_puts(L_PM_LIST_HEAD);
    for (i = 1; i <= n && !term_abort; ++i) {
        if (!msg_load(i) || (msg.flags & F_DELETED))
            continue;
        sprintf(line, L_PM_LIST_ROW, i > user.pm_read ? '*' : ' ',
                i, msg.from, msg.date[0], msg.date[1], msg.subject);
        term_puts(line);
    }
}

static void reply(void)
{
    char re[20];
    uint16_t to;
    int r;

    if (!(to = user_find(msg.from, to_name)))
        return;
    if (user.level < LVL_MEMBER && to != SYSOP_NO)
        return;
    term_puts(L_PM_REPLY_ASK);
    if ((r = term_getc()) == KEY_GONE)
        return;
    term_putc('\n');
    if (!term_key(r, L_KEY_YES))
        return;
    sprintf(re, "Re: %.15s", msg.subject);
    send_to(to, re);
}

void pm_read(void)
{
    uint16_t n, no;
    uint8_t del;

    if (!logged_in())
        return;
    msg_use('p', user_no);
    if (!(n = msg_count())) {
        term_puts(L_PM_EMPTY);
        return;
    }
    list(n);
    for (;;) {
        term_page_reset();
        term_puts(L_PM_PROMPT);
        if (term_getline(line, 5, 0) <= 0)
            return;
        del = term_key(line[0], L_KEY_DELETE);
        no = atoi(line + del);
        if (no < 1 || no > n) {
            term_puts(L_NOT_FOUND);
            continue;
        }
        msg_use('p', user_no);
        if (del) {
            term_puts(msg_delete(no) ? L_DELETED : L_ERROR);
            continue;
        }
        msg_show(no);
        if (no > user.pm_read) {
            user.pm_read = no;
            user_save();
        }
        if (!(msg.flags & F_DELETED))
            reply();
    }
}
