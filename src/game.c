#include "game.h"

#include <math.h>

#include "abilities.h"
#include "combat.h"
#include "enemy.h"
#include "player.h"

#define TAU 6.28318530717958647692f

#define WAVE_ENEMY_COUNT 8
#define WAVE_RADIUS 400.0f

// Broadcast order for the dispatch phase: damage resolves before death
// reactions, so a died event's readers see final health state.
static const event_handler_t event_handlers[] = {
    combat_damage_handler,
    ability_projectile_death_handler,
    combat_corpse_handler,
};
static const int event_handler_count =
    (int)(sizeof(event_handlers) / sizeof(event_handlers[0]));

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
    entity_add(&game->world, game->player,
               COMP_POSITION | COMP_VELOCITY | COMP_CASTER);
    game->world.positions[game->player.index] = (position_t){ 0.0f, 0.0f };
    game->world.velocities[game->player.index] = (velocity_t){ 0.0f, 0.0f };
    game->world.casters[game->player.index].cooldown_ticks = 0;
}

void game_spawn_wave(game_t *game)
{
    for (int i = 0; i < WAVE_ENEMY_COUNT; i++)
    {
        float angle = TAU * (float)i / (float)WAVE_ENEMY_COUNT;
        enemy_spawn(&game->world, ENEMY_WALKER,
                    WAVE_RADIUS * cosf(angle), WAVE_RADIUS * sinf(angle));
    }
}

void game_tick(game_t *game, const input_state_t *input)
{
    player_control_system(game, input);
    player_ability_system(game, input);
    enemy_ai_system(&game->world, game->player);
    movement_system(&game->world);
    lifetime_system(&game->world, &game->events);
    collision_system(&game->world, &game->events);

    event_dispatch(&game->world, &game->events,
                   event_handlers, event_handler_count);

    world_sweep(&game->world);

    game->tick_count++;
}
