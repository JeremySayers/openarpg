#include "combat.h"

#include "abilities.h"

void lifetime_system(world_t *world, event_queue_t *queue)
{
    const uint32_t needed = COMP_ALIVE | COMP_LIFETIME;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if ((world->masks[i] & needed) != needed)
        {
            continue;
        }

        world->lifetimes[i].remaining--;
        if (world->lifetimes[i].remaining > 0)
        {
            continue;
        }

        entity_t entity = { (uint16_t)i, world->generations[i] };
        entity_destroy(world, entity);

        event_t died = { .type = EVENT_ENTITY_DIED };
        died.died.entity = entity;
        died.died.killer = ENTITY_INVALID;
        event_push(queue, died);
    }
}

void collision_system(world_t *world, event_queue_t *queue)
{
    const uint32_t projectile_mask =
        COMP_ALIVE | COMP_PROJECTILE | COMP_POSITION | COMP_COLLIDER;
    const uint32_t target_mask =
        COMP_ALIVE | COMP_ENEMY | COMP_POSITION | COMP_COLLIDER | COMP_HEALTH;

    for (int p = 0; p < MAX_ENTITIES; p++)
    {
        if ((world->masks[p] & projectile_mask) != projectile_mask)
        {
            continue;
        }

        for (int t = 0; t < MAX_ENTITIES; t++)
        {
            if ((world->masks[t] & target_mask) != target_mask)
            {
                continue;
            }

            float dx = world->positions[t].x - world->positions[p].x;
            float dy = world->positions[t].y - world->positions[p].y;
            float reach = world->colliders[t].radius + world->colliders[p].radius;
            if (dx * dx + dy * dy > reach * reach)
            {
                continue;
            }

            entity_t projectile = { (uint16_t)p, world->generations[p] };
            entity_t target = { (uint16_t)t, world->generations[t] };
            const projectile_def_t *def =
                projectile_def(world->projectiles[p].def_index);

            event_t damage = { .type = EVENT_DAMAGE_DEALT };
            damage.damage.source = world->projectiles[p].source;
            damage.damage.target = target;
            damage.damage.amount = def->damage;
            event_push(queue, damage);

            entity_destroy(world, projectile);

            event_t died = { .type = EVENT_ENTITY_DIED };
            died.died.entity = projectile;
            died.died.killer = target;
            event_push(queue, died);

            break;
        }
    }
}

void combat_damage_handler(world_t *world, event_queue_t *queue,
                           const event_t *event)
{
    if (event->type != EVENT_DAMAGE_DEALT)
    {
        return;
    }

    entity_t target = event->damage.target;
    // Skip targets already dead from an earlier event this tick.
    if (!entity_alive(world, target) ||
        !entity_has(world, target, COMP_HEALTH))
    {
        return;
    }

    world->healths[target.index].current -= event->damage.amount;
    if (world->healths[target.index].current > 0)
    {
        return;
    }

    entity_destroy(world, target);

    event_t died = { .type = EVENT_ENTITY_DIED };
    died.died.entity = target;
    died.died.killer = event->damage.source;
    event_push(queue, died);
}

void combat_corpse_handler(world_t *world, event_queue_t *queue,
                           const event_t *event)
{
    (void)queue;

    if (event->type != EVENT_ENTITY_DIED)
    {
        return;
    }
    if (!entity_has(world, event->died.entity, COMP_ENEMY))
    {
        return;
    }

    entity_t corpse = entity_create(world);
    if (!entity_valid(world, corpse))
    {
        return;
    }

    entity_add(world, corpse, COMP_POSITION | COMP_CORPSE | COMP_LIFETIME);
    world->positions[corpse.index] = world->positions[event->died.entity.index];
    world->lifetimes[corpse.index] =
        (lifetime_t){ CORPSE_LIFETIME_TICKS, CORPSE_LIFETIME_TICKS };
}
