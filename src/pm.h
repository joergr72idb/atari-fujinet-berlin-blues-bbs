/* Personal messages ("PMsg"): one mailbox pNNN per user (storage in msg.c). */
#ifndef PM_H
#define PM_H

void pm_login(void);            /* "Es ist keine Post fuer Dich da" */
void pm_write(void);
void pm_read(void);
void pm_to_sysop(void);         /* "Msg an Sysop" - also for not yet approved users */

#endif
