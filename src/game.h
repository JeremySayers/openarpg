#ifndef OPENARPG_GAME_H
#define OPENARPG_GAME_H

#include <stdint.h>

#include "ecs.h"
#include "events.h"
#include "input_state.h"

// The simulation, one fixed tick at a time. The platform layer (openarpg.c)
// owns the accumulator and decides how many ticks to run per render frame;
// this module never sees wall-clock time.
//
// Must stay raylib-free: this is linked into the headless test runner.

#define TICK_RATE 60
#define TICK_SECONDS (1.0 / TICK_RATE)
#define TICK_DT ((float)TICK_SECONDS)

// World units per second; world units are pixels at camera zoom 1.
#define PLAYER_MOVE_SPEED 250.0f

typedef struct
{
    world_t       world;
    event_queue_t events;
    uint64_t      tick_count;

    entity_t      player;
    // World-space point the player is aiming at, as of the latest tick.
    float         aim_x;
    float         aim_y;
} game_t;

void game_init(game_t *game);

// One simulation tick:
//   gameplay systems (push events) -> event dispatch -> cleanup sweep
// The same input state is reused for every tick run in a render frame.
void game_tick(game_t *game, const input_state_t *input);

#endif
