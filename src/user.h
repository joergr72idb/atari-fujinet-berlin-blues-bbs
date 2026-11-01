/* Users, login, call counters. */
#ifndef USER_H
#define USER_H

#include <stdint.h>

/* User levels, as in 1987 ("Benutzerlevel: 5") */
#define LVL_GUEST   0
#define LVL_NEW     1           /* registered, waiting for sysop approval: read only */
#define LVL_MEMBER  2
#define LVL_SYSOP   5

#define NAME_LEN    15

typedef struct {                /* users.dat: fixed 32-byte records, user no. = index + 1 */
    char     name[NAME_LEN + 1];
    uint32_t pw;                /* FNV-1a hash of upper-case name + password */
    uint16_t calls;
    uint8_t  level;
    uint8_t  flags;
    uint8_t  last[5];           /* last call: D, M, Y, h, m */
    uint16_t pm_read;           /* highest personal message read */
    uint8_t  spare;
} User;

typedef struct {                /* stats.dat */
    uint32_t total;             /* "Du bist Anrufer Nr." */
    uint16_t today;
    uint8_t  day[3];            /* D, M, Y of "today" */
    char     last_caller[NAME_LEN + 1];
} Stats;

extern User user;               /* the caller (guest: level 0, name "Gast") */
extern uint16_t user_no;        /* 0 = guest */
extern Stats stats;
extern uint8_t now[6];          /* D, M, Y, h, m, s - refreshed by clock_now() */

void clock_now(void);
void user_init(void);           /* first start: create users.dat with SYSOP */
void stats_count_call(void);
uint8_t user_login(void);       /* 0 = caller gone */
void user_logoff(void);
void user_online(char *buf);     /* time online as h:mm:ss */
void user_status(void);         /* "Dein Status" + "System Status" screens */
void user_list(void);
void user_change_pw(void);
void user_approve(void);        /* sysop: approve new users */
uint16_t user_find(const char *name, char *canonical);   /* user no., 0 = unknown */
void user_save(void);           /* write the caller's record back */

#endif
