#include "player.h"

#include <math.h>

#include "abilities.h"

void player_control_system(game_t *game, const input_state_t *input)
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

void player_ability_system(game_t *game, const input_state_t *input)
{
    world_t *world = &game->world;

    if (!entity_alive(world, game->player))
    {
        return;
    }

    caster_t *caster = &world->casters[game->player.index];
    if (caster->cooldown_ticks > 0)
    {
        caster->cooldown_ticks--;
    }

    if (input->attack && caster->cooldown_ticks == 0)
    {
        ability_cast(world, game->player, ABILITY_BASIC_SHOT,
                     input->aim_x, input->aim_y);
        caster->cooldown_ticks = ability_def(ABILITY_BASIC_SHOT)->cooldown_ticks;
    }
}
