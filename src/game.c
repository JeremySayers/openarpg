#include "game.h"

#include <math.h>
#include <stddef.h>

static void player_control_system(game_t *game, const input_state_t *input)
{
    world_t *world = &game->world;

    if (!entity_alive(world, game->player))
    {
        return;
    }

    float move_x = input->move_x;
    float move_y = input->move_y;

    // Normalize here rather than trusting the platform layer, so diagonal
    // movement is never faster than cardinal.
    float length_sq = move_x * move_x + move_y * move_y;
    if (length_sq > 1.0f)
    {
        float inv_length = 1.0f / sqrtf(length_sq);
        move_x *= inv_length;
        move_y *= inv_length;
    }

    world->velocities[game->player.index].x = move_x * PLAYER_MOVE_SPEED;
    world->velocities[game->player.index].y = move_y * PLAYER_MOVE_SPEED;

    game->aim_x = input->aim_x;
    game->aim_y = input->aim_y;
}

static void movement_system(world_t *world)
{
    const uint32_t needed = COMP_ALIVE | COMP_POSITION | COMP_VELOCITY;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if ((world->masks[i] & needed) != needed)
        {
            continue;
        }
        world->positions[i].x += world->velocities[i].x * TICK_DT;
        world->positions[i].y += world->velocities[i].y * TICK_DT;
    }
}

void game_init(game_t *game)
{
    world_init(&game->world);
    event_queue_init(&game->events);
    game->tick_count = 0;
    game->aim_x = 0.0f;
    game->aim_y = 0.0f;

    game->player = entity_create(&game->world);
    entity_add(&game->world, game->player, COMP_POSITION | COMP_VELOCITY);
    game->world.positions[game->player.index].x = 0.0f;
    game->world.positions[game->player.index].y = 0.0f;
    game->world.velocities[game->player.index].x = 0.0f;
    game->world.velocities[game->player.index].y = 0.0f;
}

void game_tick(game_t *game, const input_state_t *input)
{
    player_control_system(game, input);
    movement_system(&game->world);

    event_dispatch(&game->world, &game->events, NULL, 0);

    world_sweep(&game->world);

    game->tick_count++;
}
