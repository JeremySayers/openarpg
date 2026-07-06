#ifndef OPENARPG_ECS_H
#define OPENARPG_ECS_H

#include <stdbool.h>
#include <stdint.h>

// Must stay raylib-free: this is linked into the headless test runner.

#define MAX_ENTITIES 4096

// Handles carry the slot's generation at creation time; the generation is
// bumped when the slot is recycled, so a stored handle to a destroyed
// entity fails entity_valid() instead of silently hitting the new occupant.
typedef struct
{
    uint16_t index;
    uint16_t generation;
} entity_t;

#define ENTITY_INVALID ((entity_t){ UINT16_MAX, UINT16_MAX })

// COMP_ALIVE and COMP_DYING are managed by entity_create/entity_destroy/
// world_sweep, never by entity_add/entity_remove. A destroyed entity keeps
// COMP_DYING (mask stays nonzero) until the sweep, so its components stay
// readable for the rest of the tick. Systems must include COMP_ALIVE in
// their required mask so they skip dying entities. Mask 0 = empty slot.
typedef enum
{
    COMP_ALIVE      = 1u << 0,
    COMP_DYING      = 1u << 1,
    COMP_POSITION   = 1u << 2,
    COMP_VELOCITY   = 1u << 3,
    COMP_HEALTH     = 1u << 4,
    COMP_COLLIDER   = 1u << 5,
    COMP_PROJECTILE = 1u << 6,
    COMP_LIFETIME   = 1u << 7,
    COMP_CASTER     = 1u << 8,
    COMP_ENEMY      = 1u << 9,
    COMP_CORPSE     = 1u << 10, // tag only, no data array
} component_flag_t;

typedef struct
{
    float x;
    float y;
} position_t;

typedef struct
{
    float x;
    float y;
} velocity_t;

typedef struct
{
    int current;
    int max;
} health_t;

typedef struct
{
    float radius;
} collider_t;

typedef struct
{
    entity_t source;    // credited with any damage this projectile deals
    int      def_index; // into abilities.c's projectile_defs
} projectile_t;

// Counts down each tick; at zero the entity is destroyed. total is kept so
// rendering can fade things out proportionally.
typedef struct
{
    int remaining;
    int total;
} lifetime_t;

typedef struct
{
    int cooldown_ticks;
} caster_t;

typedef struct
{
    int def_index; // into enemy.c's enemy_defs
} enemy_t;

typedef struct
{
    uint32_t   masks[MAX_ENTITIES];
    uint16_t   generations[MAX_ENTITIES];

    position_t   positions[MAX_ENTITIES];
    velocity_t   velocities[MAX_ENTITIES];
    health_t     healths[MAX_ENTITIES];
    collider_t   colliders[MAX_ENTITIES];
    projectile_t projectiles[MAX_ENTITIES];
    lifetime_t   lifetimes[MAX_ENTITIES];
    caster_t     casters[MAX_ENTITIES];
    enemy_t      enemies[MAX_ENTITIES];

    uint16_t   free_indices[MAX_ENTITIES];
    int        free_count;
    int        live_count;

    uint16_t   pending_destroy[MAX_ENTITIES];
    int        pending_destroy_count;
} world_t;

void world_init(world_t *world);

// Returns ENTITY_INVALID when all MAX_ENTITIES slots are in use.
entity_t entity_create(world_t *world);

// Deferred: marks the entity dying; the slot is freed (and the handle
// invalidated) by world_sweep() at end of tick. Safe on stale handles.
void entity_destroy(world_t *world, entity_t entity);

// valid: the handle still points at the entity it was created for,
// including one that is dying but not yet swept.
// alive: valid and not marked for destruction.
bool entity_valid(const world_t *world, entity_t entity);
bool entity_alive(const world_t *world, entity_t entity);

// components is an OR of component_flag_t bits; COMP_ALIVE and COMP_DYING
// are ignored here. All are no-ops on stale handles.
void entity_add(world_t *world, entity_t entity, uint32_t components);
void entity_remove(world_t *world, entity_t entity, uint32_t components);
bool entity_has(const world_t *world, entity_t entity, uint32_t components);

// End-of-tick cleanup: frees every entity marked by entity_destroy() and
// bumps its slot's generation. Returns the number of entities freed.
int world_sweep(world_t *world);

#endif
