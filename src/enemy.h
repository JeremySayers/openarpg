#ifndef OPENARPG_ENEMY_H
#define OPENARPG_ENEMY_H

#include "ecs.h"

typedef struct
{
    int   max_health;
    float move_speed; // world units per second
    float radius;
} enemy_def_t;

typedef enum
{
    ENEMY_WALKER,
    ENEMY_DEF_COUNT
} enemy_def_id_t;

const enemy_def_t *enemy_def(int id);

entity_t enemy_spawn(world_t *world, int def_index, float x, float y);

// Walks every enemy straight toward the player.
void enemy_ai_system(world_t *world, entity_t player);

#endif
