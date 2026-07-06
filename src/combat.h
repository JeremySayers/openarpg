#ifndef OPENARPG_COMBAT_H
#define OPENARPG_COMBAT_H

#include "ecs.h"
#include "events.h"

#define CORPSE_LIFETIME_TICKS 30

// Ticks down every COMP_LIFETIME entity; expiry destroys it and pushes
// EVENT_ENTITY_DIED so death effects fire the same way as on impact.
void lifetime_system(world_t *world, event_queue_t *queue);

// Projectile-vs-enemy circle overlap. A hit pushes EVENT_DAMAGE_DEALT and
// kills the projectile (no piercing).
void collision_system(world_t *world, event_queue_t *queue);

// Dispatch handlers.
void combat_damage_handler(world_t *world, event_queue_t *queue,
                           const event_t *event);
void combat_corpse_handler(world_t *world, event_queue_t *queue,
                           const event_t *event);

#endif
