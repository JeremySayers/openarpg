#include "ecs.h"

// Lifecycle bits only entity_create/entity_destroy/world_sweep may touch.
#define INTERNAL_COMPONENTS ((uint32_t)COMP_ALIVE | (uint32_t)COMP_DYING)

void world_init(world_t *world)
{
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        world->masks[i] = 0;
        world->generations[i] = 0;
        // Filled in reverse so entity 0 is the first one handed out.
        world->free_indices[i] = (uint16_t)(MAX_ENTITIES - 1 - i);
    }
    world->free_count = MAX_ENTITIES;
    world->live_count = 0;
    world->pending_destroy_count = 0;
}

entity_t entity_create(world_t *world)
{
    if (world->free_count == 0)
    {
        return ENTITY_INVALID;
    }

    world->free_count--;
    uint16_t index = world->free_indices[world->free_count];

    world->masks[index] = COMP_ALIVE;
    world->live_count++;

    entity_t entity = { index, world->generations[index] };
    return entity;
}

void entity_destroy(world_t *world, entity_t entity)
{
    if (!entity_alive(world, entity))
    {
        return;
    }
    world->masks[entity.index] &= ~(uint32_t)COMP_ALIVE;
    world->masks[entity.index] |= COMP_DYING;
    world->live_count--;
    world->pending_destroy[world->pending_destroy_count] = entity.index;
    world->pending_destroy_count++;
}

bool entity_valid(const world_t *world, entity_t entity)
{
    return entity.index < MAX_ENTITIES
        && world->masks[entity.index] != 0
        && world->generations[entity.index] == entity.generation;
}

bool entity_alive(const world_t *world, entity_t entity)
{
    return entity_valid(world, entity)
        && (world->masks[entity.index] & COMP_ALIVE) != 0;
}

void entity_add(world_t *world, entity_t entity, uint32_t components)
{
    if (!entity_valid(world, entity))
    {
        return;
    }
    world->masks[entity.index] |= components & ~INTERNAL_COMPONENTS;
}

void entity_remove(world_t *world, entity_t entity, uint32_t components)
{
    if (!entity_valid(world, entity))
    {
        return;
    }
    world->masks[entity.index] &= ~(components & ~INTERNAL_COMPONENTS);
}

bool entity_has(const world_t *world, entity_t entity, uint32_t components)
{
    return entity_valid(world, entity)
        && (world->masks[entity.index] & components) == components;
}

int world_sweep(world_t *world)
{
    int freed = world->pending_destroy_count;
    for (int i = 0; i < freed; i++)
    {
        uint16_t index = world->pending_destroy[i];
        world->masks[index] = 0;
        world->generations[index]++;
        world->free_indices[world->free_count] = index;
        world->free_count++;
    }
    world->pending_destroy_count = 0;
    return freed;
}
