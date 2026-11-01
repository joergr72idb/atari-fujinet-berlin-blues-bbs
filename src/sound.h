/* Sysop signals on the Atari's POKEY. */
#ifndef SOUND_H
#define SOUND_H

void sound_ring(void);          /* incoming call: telephone double ring */
void sound_page(void);          /* caller pages the sysop: rising triple beep + pause, ~1 s */

#endif
