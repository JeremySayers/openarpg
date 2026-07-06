#include "enemy.h"

#include <math.h>

static const enemy_def_t enemy_defs[ENEMY_DEF_COUNT] = {
    [ENEMY_WALKER] = {
        .max_health = 100,
        .move_speed = 100.0f,
        .radius = 14.0f,
    },
};

const enemy_def_t *enemy_def(int id)
{
    return &enemy_defs[id];
}

entity_t enemy_spawn(world_t *world, int def_index, float x, float y)
{
    const enemy_def_t *def = &enemy_defs[def_index];

    entity_t enemy = entity_create(world);
    if (!entity_valid(world, enemy))
    {
        return enemy;
    }

    entity_add(world, enemy,
               COMP_POSITION | COMP_VELOCITY | COMP_HEALTH |
               COMP_COLLIDER | COMP_ENEMY);

    world->positions[enemy.index] = (position_t){ x, y };
    world->velocities[enemy.index] = (velocity_t){ 0.0f, 0.0f };
    world->healths[enemy.index] = (health_t){ def->max_health, def->max_health };
    world->colliders[enemy.index].radius = def->radius;
    world->enemies[enemy.index].def_index = def_index;

    return enemy;
}

void enemy_ai_system(world_t *world, entity_t player)
{
    if (!entity_has(world, player, COMP_ALIVE | COMP_POSITION))
    {
        return;
    }
    position_t player_pos = world->positions[player.index];

    const uint32_t needed =
        COMP_ALIVE | COMP_ENEMY | COMP_POSITION | COMP_VELOCITY;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if ((world->masks[i] & needed) != needed)
        {
            continue;
        }

        float dx = player_pos.x - world->positions[i].x;
        float dy = player_pos.y - world->positions[i].y;
        float length = sqrtf(dx * dx + dy * dy);
        if (length < 1.0f)
        {
            world->velocities[i] = (velocity_t){ 0.0f, 0.0f };
            continue;
        }

        float speed = enemy_defs[world->enemies[i].def_index].move_speed;
        world->velocities[i].x = dx / length * speed;
        world->velocities[i].y = dy / length * speed;
    }
}
