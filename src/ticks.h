/* Elapsed time in clock ticks, safe across the wrap of the Atari clock.
   cc65's clock() reads the 24-bit RTCLOK: it wraps to 0 every 2^24 ticks (PAL: 3.9 days),
   so a plain clock() - t would jump to ~4 billion right after the wrap. */
#ifndef TICKS_H
#define TICKS_H

#include <time.h>

#define ELAPSED(t)  ((clock() - (t)) & 0xFFFFFFUL)

#endif
