#ifndef OPENARPG_GAME_H
#define OPENARPG_GAME_H

#include <stdint.h>

#include "ecs.h"
#include "events.h"

// The simulation, one fixed tick at a time. The platform layer (openarpg.c)
// owns the accumulator and decides how many ticks to run per render frame;
// this module never sees wall-clock time.
//
// Must stay raylib-free: this is linked into the headless test runner.

#define TICK_RATE 60
#define TICK_SECONDS (1.0 / TICK_RATE)
#define TICK_DT ((float)TICK_SECONDS)

typedef struct
{
    world_t       world;
    event_queue_t events;
    uint64_t      tick_count;
} game_t;

void game_init(game_t *game);

// One 60Hz simulation tick:
//   gameplay systems (push events) -> event dispatch -> cleanup sweep
void game_tick(game_t *game);

#endif
