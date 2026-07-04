#include "game.h"

#include <stddef.h>

void game_init(game_t *game)
{
    world_init(&game->world);
    event_queue_init(&game->events);
    game->tick_count = 0;
}

void game_tick(game_t *game)
{
    // Gameplay systems run here, in fixed order, pushing events as they go.
    // None exist yet, so the dispatch has no handlers to offer events to.
    event_dispatch(&game->world, &game->events, NULL, 0);

    world_sweep(&game->world);

    game->tick_count++;
}
