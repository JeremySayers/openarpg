#ifndef OPENARPG_ABILITIES_H
#define OPENARPG_ABILITIES_H

#include "ecs.h"
#include "events.h"

// Abilities are data: an ability_def_t row says what to fire, a
// projectile_def_t row says how it flies and what happens when it dies.
// Adding an ability means adding rows, not systems. New code is only
// needed for a genuinely new effect kind, which then every def can use.

typedef struct
{
    float speed;          // world units per second
    int   lifetime_ticks;
    int   damage;
    float radius;

    // Death effect: when this projectile dies (impact or expiry), spawn
    // on_death_count projectiles of on_death_def in an even radial burst
    // at its final position. -1 = nothing. Chains are allowed; a cycle in
    // the defs is a content bug, capped at runtime by the projectiles'
    // own lifetimes and the dispatch guard.
    int   on_death_def;
    int   on_death_count;
} projectile_def_t;

typedef struct
{
    int   cooldown_ticks;
    int   projectile_def;
    int   count;          // projectiles per cast
    float spread_degrees; // total arc for count > 1, centered on the aim
} ability_def_t;

typedef enum
{
    ABILITY_BASIC_SHOT,
    ABILITY_BURST_SHOT,
    ABILITY_COUNT
} ability_id_t;

typedef enum
{
    PROJ_BASIC_SHOT,
    PROJ_BURST_SHELL,
    PROJ_BURST_SHRAPNEL,
    PROJ_COUNT
} projectile_id_t;

const ability_def_t *ability_def(int id);
const projectile_def_t *projectile_def(int id);

// Fires ability_id from caster's position toward the target point. Works
// for any caster with a position; cooldown gating is the caller's job.
void ability_cast(world_t *world, entity_t caster, int ability_id,
                  float target_x, float target_y);

// Dispatch handler: executes on_death_def spawns for dying projectiles.
void ability_projectile_death_handler(world_t *world, event_queue_t *queue,
                                      const event_t *event);

#endif
