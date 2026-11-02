/* Users, login, call counters. Data: bbs/users.dat (32-byte records), bbs/stats.dat. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "ticks.h"
#include "fujinet-clock.h"
#include "net.h"
#include "term.h"
#include "lang.h"
#include "store.h"
#include "user.h"
#include "sysop.h"

#define USERS     BBS_DIR "users.dat"
#define STATS     BBS_DIR "stats.dat"
#define SETUP_PW  BBS_DIR "sysop.txt"   /* first start only: initial sysop password */
#define TZ        "CET-1CEST,M3.5.0,M10.5.0/3"

User user;
uint16_t user_no;
Stats stats;
uint8_t now[6];

static User u;                  /* scratch record */
static char line[41];
static char pw[17];
static clock_t online_since;
static uint8_t prev_last[5];

static const char *const level_names[] = {
    L_LEVEL_GUEST, L_LEVEL_NEW, L_LEVEL_MEMBER, L_LEVEL_MEMBER, L_LEVEL_MEMBER, "SYSOP"
};

void clock_now(void)
{
    if (clock_get_time(now, APETIME_BINARY) != FN_ERR_OK)
        memset(now, 0, sizeof now);
}

static void put_date(const uint8_t *d)
{
    sprintf(line, L_DATE, d[0], d[1], d[2]);
    term_puts(line);
}

/* FNV-1a over the upper-case name, ':' and the (case sensitive) password. */
static uint32_t pw_hash(const char *name, const char *pass)
{
    uint32_t h = 2166136261UL;

    while (*name) {
        h ^= (uint8_t) toupper(*name++);
        h *= 16777619UL;
    }
    h ^= ':';
    h *= 16777619UL;
    while (*pass) {
        h ^= (uint8_t) *pass++;
        h *= 16777619UL;
    }
    return h;
}

static uint8_t same_name(const char *a, const char *b)
{
    while (*a && toupper(*a) == toupper(*b)) {
        ++a;
        ++b;
    }
    return toupper(*a) == toupper(*b);
}

/* Search users.dat; returns user no. (index + 1) and the record in u, 0 = not found. */
static uint16_t find_user(const char *name)
{
    uint16_t no = 0;

    if (!store_open(USERS))
        return 0;
    while (store_next(&u, sizeof u)) {
        ++no;
        if (same_name(u.name, name)) {
            store_close();
            return no;
        }
    }
    store_close();
    return 0;
}

static uint8_t save_user(uint16_t no, const User *rec)
{
    return store_put_check(USERS, (uint32_t) (no - 1) * sizeof(User), rec, sizeof(User));
}

uint16_t user_find(const char *name, char *canonical)
{
    uint16_t no = find_user(name);

    if (no && canonical)
        strcpy(canonical, u.name);
    return no;
}

void user_save(void)
{
    if (user_no)
        save_user(user_no, &user);
}

void user_init(void)
{
    uint16_t n;

    clock_set_tz(TZ);
    if (store_size_sure(USERS) >= sizeof(User))
        return;
    memset(&u, 0, sizeof u);
    strcpy(u.name, "SYSOP");
    u.level = LVL_SYSOP;
    n = store_size(SETUP_PW);
    if (n && n < sizeof pw && store_get(SETUP_PW, 0, pw, n)) {
        pw[n] = 0;
        pw[strcspn(pw, "\015\012")] = 0;    /* file bytes: CR/LF, not cc65 "\n" ($9B) */
    } else {
        strcpy(pw, "blues");
        puts(L_SYS_DEFAULT_PW);
    }
    u.pw = pw_hash(u.name, pw);
    store_put(USERS, 0, &u, sizeof u);
    puts(L_SYS_USERS_CREATED);
    if (store_delete(SETUP_PW))         /* don't leave the password on the card */
        puts(L_SYS_SETUP_DELETED);
}

void stats_count_call(void)
{
    if (!store_get(STATS, 0, &stats, sizeof stats))
        memset(&stats, 0, sizeof stats);
    clock_now();
    if (memcmp(stats.day, now, 3)) {
        memcpy(stats.day, now, 3);
        stats.today = 0;
    }
    ++stats.total;
    ++stats.today;
    store_put(STATS, 0, &stats, sizeof stats);
    user_clock_start();
}

void user_clock_start(void)
{
    online_since = clock();
}

static uint8_t valid_name(const char *s)
{
    uint8_t n = 0;

    for (; *s; ++s, ++n)
        if (!isalnum(*s) && *s != ' ' && *s != '-' && *s != '_' && *s != '.')
            return 0;
    return n >= 3;
}

/* Ask for a password twice; 1 = ok (in pw), 0 = mismatch, KEY_GONE = caller gone. */
static int ask_new_pw(void)
{
    char again[17];

    term_puts(L_PW_NEW);
    if (term_getline(pw, 16, 1) == KEY_GONE)
        return KEY_GONE;
    term_puts(L_PW_AGAIN);
    if (term_getline(again, 16, 1) == KEY_GONE)
        return KEY_GONE;
    if (strlen(pw) < 4 || strcmp(pw, again)) {
        term_puts(L_PW_INVALID);
        return 0;
    }
    return 1;
}

static uint8_t do_register(const char *name)
{
    int r;
    uint16_t n;

    term_puts(L_REGISTER_ASK);
    r = term_getc();
    if (r == KEY_GONE)
        return 0;
    term_putc('\n');
    if (!term_key(r, L_KEY_YES))
        return 2;
    if (!valid_name(name)) {
        term_puts(L_NAME_RULE);
        return 2;
    }
    term_puts(L_PW_HINT);
    do {
        if ((r = ask_new_pw()) == KEY_GONE)
            return 0;
    } while (!r);
    memset(&user, 0, sizeof user);
    strcpy(user.name, name);
    user.pw = pw_hash(name, pw);
    user.level = LVL_NEW;
    /* into the slot after the last whole record (not appended: a half-written record from a
       broken save must not shift all later user numbers); n = 0 means users.dat unreadable */
    n = store_size_sure(USERS) / sizeof(User);
    if (!n || !store_put_check(USERS, (uint32_t) n * sizeof(User), &user, sizeof user)) {
        term_puts(L_SAVE_ERROR);
        return 2;
    }
    user_no = n + 1;
    term_puts(L_WELCOME_NEW);
    return 1;
}

/* 1 = logged in (or guest), 0 = caller gone/kicked */
uint8_t user_login(void)
{
    uint8_t tries;
    int r;

    for (;;) {
        term_puts(L_LOGIN_PROMPT);
        if (term_getline(line, NAME_LEN, 0) == KEY_GONE)
            return 0;
        if (!line[0]) {
            memset(&user, 0, sizeof user);
            strcpy(user.name, L_GUEST_NAME);
            user_no = 0;
            return 1;
        }
        user_no = find_user(line);
        if (!user_no) {
            r = do_register(line);
            if (r == 2)
                continue;
            break;
        }
        user = u;
        for (tries = 0; tries < 3; ++tries) {
            term_puts(L_PW_PROMPT);
            if (term_getline(pw, 16, 1) == KEY_GONE)
                return 0;
            if (pw_hash(user.name, pw) == user.pw)
                break;
            term_puts(L_PW_WRONG);
        }
        if (tries == 3) {
            term_puts(L_PW_TOO_MANY);
            return 0;
        }
        break;
    }
    if (!user_no)
        return 0;
    /* count the call for the user, remember the previous "last call" for the status */
    memcpy(prev_last, user.last, sizeof prev_last);
    clock_now();
    memcpy(user.last, now, 5);
    ++user.calls;
    save_user(user_no, &user);
    strcpy(stats.last_caller, user.name);
    store_put(STATS, 0, &stats, sizeof stats);
    return 1;
}

void user_status(void)
{
    sprintf(line, L_STATUS_HEAD, level_names[user.level]);
    term_puts(line);
    if (user_no) {
        sprintf(line, L_STATUS_CALLS, user.calls);
        term_puts(line);
        term_puts(L_STATUS_LAST);
        if (prev_last[0])
            put_date(prev_last);
        else
            term_puts("-");
        term_putc('\n');
    }
    sprintf(line, L_STATUS_LEVEL, user.level);
    term_puts(line);

    clock_now();
    term_puts(L_SYSTEM_HEAD);
    put_date(now);
    sprintf(line, L_SYSTEM_TIME, now[3], now[4], now[5]);
    term_puts(line);
    sprintf(line, L_SYSTEM_CALLERS, stats.total, stats.today);
    term_puts(line);
    sprintf(line, L_SYSTEM_LAST, stats.last_caller[0] ? stats.last_caller : "-");
    term_puts(line);
    term_puts(sysop_present ? L_SYSOP_PRESENT : L_SYSOP_ABSENT);
}

void user_online(char *buf)
{
    unsigned long s = (ELAPSED(online_since)) / CLOCKS_PER_SEC;

    sprintf(buf, "%lu:%02lu:%02lu", s / 3600, s / 60 % 60, s % 60);
}

void user_logoff(void)
{
    char t[12];

    clock_now();
    sprintf(line, L_LOGOFF_TIME, now[3], now[4], now[5]);
    term_puts(line);
    user_online(t);
    sprintf(line, L_LOGOFF_ONLINE, t);
    term_puts(line);
    term_puts(L_LOGOFF_BYE);
}

void user_list(void)
{
    uint16_t no = 0;

    term_puts(L_USERLIST_HEAD);
    if (!store_open(USERS))
        return;
    while (!term_abort && store_next(&u, sizeof u)) {
        sprintf(line, "%3u %-15s %u   %5u ", ++no, u.name, u.level, u.calls);
        term_puts(line);
        if (u.last[0])
            put_date(u.last);
        term_putc('\n');
    }
    store_close();
}

void user_change_pw(void)
{
    int r;

    if (!user_no) {
        term_puts(L_GUEST_NO_PW);
        return;
    }
    term_puts(L_PW_OLD);
    if (term_getline(pw, 16, 1) == KEY_GONE)
        return;
    if (pw_hash(user.name, pw) != user.pw) {
        term_puts(L_PW_WRONG);
        return;
    }
    if ((r = ask_new_pw()) != 1)
        return;
    user.pw = pw_hash(user.name, pw);
    term_puts(save_user(user_no, &user) ? L_PW_CHANGED : L_SAVE_ERROR);
}

void user_approve(void)
{
    uint16_t no, n;
    uint8_t found = 0;

    if (user.level < LVL_SYSOP)
        return;
    term_puts(L_APPROVE_HEAD);
    if (store_open(USERS)) {
        for (no = 1; store_next(&u, sizeof u); ++no)
            if (u.level == LVL_NEW) {
                sprintf(line, "%3u %s\n", no, u.name);
                term_puts(line);
                found = 1;
            }
        store_close();
    }
    if (!found) {
        term_puts(L_NONE);
        return;
    }
    for (;;) {
        term_puts(L_APPROVE_PROMPT);
        if (term_getline(line, 4, 0) <= 0)
            return;
        n = atoi(line);
        if (!n || !store_get(USERS, (uint32_t) (n - 1) * sizeof(User), &u, sizeof u) || u.level != LVL_NEW) {
            term_puts(L_BAD_NUMBER);
            continue;
        }
        u.level = LVL_MEMBER;
        sprintf(line, save_user(n, &u) ? L_APPROVED : L_ERROR, u.name);
        term_puts(line);
    }
}
