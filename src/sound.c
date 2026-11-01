/* Sysop signals on the Atari's POKEY. */
#include <atari.h>
#include <time.h>
#include "sound.h"

/* Wait n clock ticks (1/50 s PAL, 1/60 s NTSC). */
static void wait(clock_t n)
{
    clock_t t = clock() + n;

    while (clock() < t)
        ;
}

/* One ring: bell-like trill, two pitches alternating every tick. */
static void ring(void)
{
    unsigned char i;

    for (i = 0; i < 20; ++i) {
        _sound(0, (i & 1) ? 28 : 34, 10, 8);
        wait(1);
    }
    _sound(0, 0, 0, 0);
}

void sound_ring(void)
{
    ring();
    wait(10);
    ring();
}
