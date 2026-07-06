#include "abilities.h"

#include <math.h>

#define TAU 6.28318530717958647692f

static const ability_def_t ability_defs[ABILITY_COUNT] = {
    [ABILITY_BASIC_SHOT] = {
        .cooldown_ticks = 15,
        .projectile_def = PROJ_BASIC_SHOT,
        .count = 1,
        .spread_degrees = 0.0f,
    },
    [ABILITY_BURST_SHOT] = {
        .cooldown_ticks = 45,
        .projectile_def = PROJ_BURST_SHELL,
        .count = 1,
        .spread_degrees = 0.0f,
    },
};

static const projectile_def_t projectile_defs[PROJ_COUNT] = {
    [PROJ_BASIC_SHOT] = {
        .speed = 600.0f,
        .lifetime_ticks = 60,
        .damage = 25,
        .radius = 4.0f,
        .on_death_def = -1,
        .on_death_count = 0,
    },
    [PROJ_BURST_SHELL] = {
        .speed = 450.0f,
        .lifetime_ticks = 40,
        .damage = 10,
        .radius = 6.0f,
        .on_death_def = PROJ_BURST_SHRAPNEL,
        .on_death_count = 8,
    },
    [PROJ_BURST_SHRAPNEL] = {
        .speed = 350.0f,
        .lifetime_ticks = 25,
        .damage = 15,
        .radius = 3.0f,
        .on_death_def = -1,
        .on_death_count = 0,
    },
};

const ability_def_t *ability_def(int id)
{
    return &ability_defs[id];
}

const projectile_def_t *projectile_def(int id)
{
    return &projectile_defs[id];
}

static void spawn_projectile(world_t *world, entity_t source, int def_index,
                             float x, float y, float dir_x, float dir_y)
{
    const projectile_def_t *def = &projectile_defs[def_index];

    entity_t projectile = entity_create(world);
    if (!entity_valid(world, projectile))
    {
        return;
    }

    entity_add(world, projectile,
               COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER |
               COMP_PROJECTILE | COMP_LIFETIME);

    world->positions[projectile.index] = (position_t){ x, y };
    world->velocities[projectile.index] =
        (velocity_t){ dir_x * def->speed, dir_y * def->speed };
    world->colliders[projectile.index].radius = def->radius;
    world->projectiles[projectile.index] =
        (projectile_t){ source, def_index };
    world->lifetimes[projectile.index] =
        (lifetime_t){ def->lifetime_ticks, def->lifetime_ticks };
}

void ability_cast(world_t *world, entity_t caster, int ability_id,
                  float target_x, float target_y)
{
    if (!entity_has(world, caster, COMP_ALIVE | COMP_POSITION))
    {
        return;
    }

    const ability_def_t *def = &ability_defs[ability_id];
    position_t origin = world->positions[caster.index];

    float dir_x = target_x - origin.x;
    float dir_y = target_y - origin.y;
    float length = sqrtf(dir_x * dir_x + dir_y * dir_y);
    if (length < 0.001f)
    {
        dir_x = 1.0f;
        dir_y = 0.0f;
    }
    else
    {
        dir_x /= length;
        dir_y /= length;
    }

    float aim_angle = atan2f(dir_y, dir_x);
    float spread = def->spread_degrees * (TAU / 360.0f);

    for (int i = 0; i < def->count; i++)
    {
        float angle = aim_angle;
        if (def->count > 1)
        {
            angle += spread * ((float)i / (float)(def->count - 1) - 0.5f);
        }
        spawn_projectile(world, caster, def->projectile_def,
                         origin.x, origin.y, cosf(angle), sinf(angle));
    }
}

void ability_projectile_death_handler(world_t *world, event_queue_t *queue,
                                      const event_t *event)
{
    (void)queue;

    if (event->type != EVENT_ENTITY_DIED)
    {
        return;
    }
    // The dying projectile is unswept until end of tick, so its position
    // and def are still readable here.
    entity_t dead = event->died.entity;
    if (!entity_has(world, dead, COMP_PROJECTILE))
    {
        return;
    }

    const projectile_t *projectile = &world->projectiles[dead.index];
    const projectile_def_t *def = &projectile_defs[projectile->def_index];
    if (def->on_death_def < 0 || def->on_death_count <= 0)
    {
        return;
    }

    position_t origin = world->positions[dead.index];
    for (int i = 0; i < def->on_death_count; i++)
    {
        float angle = TAU * (float)i / (float)def->on_death_count;
        spawn_projectile(world, projectile->source, def->on_death_def,
                         origin.x, origin.y, cosf(angle), sinf(angle));
    }
}
