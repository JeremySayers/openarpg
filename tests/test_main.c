// Headless tests for the raylib-free simulation core (ecs, events).
// Plain assert-macro runner: every CHECK failure prints and counts, the
// process exits nonzero if any failed. No framework on purpose.

#include <math.h>
#include <stdio.h>

#include "abilities.h"
#include "combat.h"
#include "ecs.h"
#include "enemy.h"
#include "events.h"
#include "game.h"

static int checks_failed = 0;
static int checks_run = 0;

#define CHECK(cond) \
    do \
    { \
        checks_run++; \
        if (!(cond)) \
        { \
            checks_failed++; \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

// world_t is ~100KB - keep it off the stack.
static world_t world;
static event_queue_t queue;

// --- ECS ---------------------------------------------------------------

static void test_entity_create_and_alive(void)
{
    world_init(&world);

    entity_t e = entity_create(&world);
    CHECK(entity_valid(&world, e));
    CHECK(entity_alive(&world, e));
    CHECK(world.live_count == 1);

    CHECK(!entity_valid(&world, ENTITY_INVALID));
    CHECK(!entity_alive(&world, ENTITY_INVALID));
}

static void test_entity_hard_cap(void)
{
    world_init(&world);

    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        entity_t e = entity_create(&world);
        CHECK(entity_alive(&world, e));
    }
    CHECK(world.live_count == MAX_ENTITIES);

    entity_t overflow = entity_create(&world);
    CHECK(!entity_valid(&world, overflow));
    CHECK(overflow.index == ENTITY_INVALID.index);
}

static void test_deferred_destroy_and_stale_handles(void)
{
    world_init(&world);

    entity_t e = entity_create(&world);
    entity_add(&world, e, COMP_POSITION);
    world.positions[e.index].x = 42.0f;

    entity_destroy(&world, e);

    // Dying but unswept: handle still valid, components still readable,
    // but no longer "alive" (systems must skip it).
    CHECK(entity_valid(&world, e));
    CHECK(!entity_alive(&world, e));
    CHECK(entity_has(&world, e, COMP_POSITION));
    CHECK(world.positions[e.index].x == 42.0f);

    // Double-destroy is a safe no-op.
    entity_destroy(&world, e);
    CHECK(world.pending_destroy_count == 1);

    int freed = world_sweep(&world);
    CHECK(freed == 1);
    CHECK(!entity_valid(&world, e));
    CHECK(world.live_count == 0);

    // Slot gets recycled with a bumped generation: new entity is valid,
    // the old handle to the same slot stays invalid.
    entity_t recycled = entity_create(&world);
    CHECK(recycled.index == e.index);
    CHECK(recycled.generation != e.generation);
    CHECK(entity_alive(&world, recycled));
    CHECK(!entity_valid(&world, e));
}

static void test_destroy_component_less_entity(void)
{
    // Regression guard: an entity with only COMP_ALIVE must still get its
    // slot recycled by the sweep.
    world_init(&world);

    entity_t e = entity_create(&world);
    entity_destroy(&world, e);
    CHECK(world_sweep(&world) == 1);
    CHECK(world.free_count == MAX_ENTITIES);
    CHECK(!entity_valid(&world, e));
}

static void test_component_mask_ops(void)
{
    world_init(&world);

    entity_t e = entity_create(&world);
    CHECK(!entity_has(&world, e, COMP_POSITION));

    entity_add(&world, e, COMP_POSITION | COMP_VELOCITY);
    CHECK(entity_has(&world, e, COMP_POSITION | COMP_VELOCITY));
    CHECK(entity_has(&world, e, COMP_ALIVE | COMP_POSITION));

    entity_remove(&world, e, COMP_VELOCITY);
    CHECK(entity_has(&world, e, COMP_POSITION));
    CHECK(!entity_has(&world, e, COMP_VELOCITY));

    // COMP_ALIVE is create/destroy's job; add/remove must not touch it.
    entity_remove(&world, e, COMP_ALIVE);
    CHECK(entity_alive(&world, e));
    entity_destroy(&world, e);
    entity_add(&world, e, COMP_ALIVE);
    CHECK(!entity_alive(&world, e));
}

// --- Events ------------------------------------------------------------

static int seen_damage;
static int seen_died;
static int handler_a_calls;
static int handler_b_calls;

static void count_events_handler(world_t *w, event_queue_t *q,
                                 const event_t *event)
{
    (void)w;
    (void)q;
    handler_a_calls++;
    if (event->type == EVENT_DAMAGE_DEALT)
    {
        seen_damage++;
    }
    if (event->type == EVENT_ENTITY_DIED)
    {
        seen_died++;
    }
}

static void second_handler(world_t *w, event_queue_t *q,
                           const event_t *event)
{
    (void)w;
    (void)q;
    (void)event;
    handler_b_calls++;
}

// Cascades: turns every damage event into a died event, once per damage.
static void cascade_handler(world_t *w, event_queue_t *q,
                            const event_t *event)
{
    (void)w;
    if (event->type == EVENT_DAMAGE_DEALT)
    {
        event_t died = { .type = EVENT_ENTITY_DIED };
        died.died.entity = event->damage.target;
        died.died.killer = event->damage.source;
        event_push(q, died);
    }
}

// Runaway: pushes one event for every event, forever.
static void runaway_handler(world_t *w, event_queue_t *q,
                            const event_t *event)
{
    (void)w;
    event_push(q, *event);
}

static void reset_event_counters(void)
{
    seen_damage = 0;
    seen_died = 0;
    handler_a_calls = 0;
    handler_b_calls = 0;
}

static void test_event_fifo_and_broadcast(void)
{
    world_init(&world);
    event_queue_init(&queue);
    reset_event_counters();

    event_t damage = { .type = EVENT_DAMAGE_DEALT };
    event_t died = { .type = EVENT_ENTITY_DIED };
    CHECK(event_push(&queue, damage));
    CHECK(event_push(&queue, died));
    CHECK(queue.high_water == 2);

    const event_handler_t handlers[] = { count_events_handler, second_handler };
    int processed = event_dispatch(&world, &queue, handlers, 2);

    CHECK(processed == 2);
    CHECK(queue.count == 0);
    // Broadcast: both handlers saw both events.
    CHECK(handler_a_calls == 2);
    CHECK(handler_b_calls == 2);
    CHECK(seen_damage == 1);
    CHECK(seen_died == 1);
}

static void test_event_cascade_resolves_same_dispatch(void)
{
    world_init(&world);
    event_queue_init(&queue);
    reset_event_counters();

    event_t damage = { .type = EVENT_DAMAGE_DEALT };
    damage.damage.source = entity_create(&world);
    damage.damage.target = entity_create(&world);
    damage.damage.amount = 10;
    event_push(&queue, damage);

    const event_handler_t handlers[] = { cascade_handler, count_events_handler };
    int processed = event_dispatch(&world, &queue, handlers, 2);

    // damage -> died cascade fully resolved in one dispatch call.
    CHECK(processed == 2);
    CHECK(seen_damage == 1);
    CHECK(seen_died == 1);
    CHECK(queue.count == 0);
}

static void test_event_runaway_guard(void)
{
    world_init(&world);
    event_queue_init(&queue);

    event_t damage = { .type = EVENT_DAMAGE_DEALT };
    event_push(&queue, damage);

    const event_handler_t handlers[] = { runaway_handler };
    int processed = event_dispatch(&world, &queue, handlers, 1);

    // The guard stops the loop and discards the garbage it generated.
    CHECK(processed == MAX_EVENTS_PER_TICK);
    CHECK(queue.count == 0);
}

static void test_event_overflow_drops(void)
{
    world_init(&world);
    event_queue_init(&queue);

    event_t damage = { .type = EVENT_DAMAGE_DEALT };
    for (int i = 0; i < EVENT_QUEUE_CAPACITY; i++)
    {
        CHECK(event_push(&queue, damage));
    }
    CHECK(!event_push(&queue, damage));
    CHECK(queue.dropped_total == 1);
    CHECK(queue.count == EVENT_QUEUE_CAPACITY);
    CHECK(queue.high_water == EVENT_QUEUE_CAPACITY);
}

// --- Game tick ---------------------------------------------------------

static game_t game;
static const input_state_t no_input;

static void test_game_tick_sweeps(void)
{
    game_init(&game);

    entity_t e = entity_create(&game.world);
    entity_destroy(&game.world, e);
    CHECK(entity_valid(&game.world, e));

    game_tick(&game, &no_input);

    CHECK(!entity_valid(&game.world, e));
    CHECK(game.tick_count == 1);
}

// --- Player movement ---------------------------------------------------

#define CHECK_NEAR(a, b) CHECK(fabsf((a) - (b)) < 0.001f)

static void test_player_spawns(void)
{
    game_init(&game);

    CHECK(entity_alive(&game.world, game.player));
    CHECK(entity_has(&game.world, game.player, COMP_POSITION | COMP_VELOCITY));
    CHECK_NEAR(game.world.positions[game.player.index].x, 0.0f);
    CHECK_NEAR(game.world.positions[game.player.index].y, 0.0f);
}

static void test_player_moves_and_stops(void)
{
    game_init(&game);

    input_state_t input = { 0 };
    input.move_x = 1.0f;
    for (int i = 0; i < 60; i++)
    {
        game_tick(&game, &input);
    }

    // One second of ticks at full speed covers PLAYER_MOVE_SPEED units.
    CHECK_NEAR(game.world.positions[game.player.index].x, PLAYER_MOVE_SPEED);
    CHECK_NEAR(game.world.positions[game.player.index].y, 0.0f);

    game_tick(&game, &no_input);
    float stopped_x = game.world.positions[game.player.index].x;
    game_tick(&game, &no_input);
    CHECK_NEAR(game.world.positions[game.player.index].x, stopped_x);
    CHECK_NEAR(game.world.velocities[game.player.index].x, 0.0f);
}

static void test_player_diagonal_not_faster(void)
{
    game_init(&game);

    input_state_t input = { 0 };
    input.move_x = 1.0f;
    input.move_y = 1.0f;
    game_tick(&game, &input);

    velocity_t v = game.world.velocities[game.player.index];
    float speed = sqrtf(v.x * v.x + v.y * v.y);
    CHECK_NEAR(speed, PLAYER_MOVE_SPEED);
}

static void test_aim_is_stored(void)
{
    game_init(&game);

    input_state_t input = { 0 };
    input.aim_x = 123.0f;
    input.aim_y = -456.0f;
    game_tick(&game, &input);

    CHECK_NEAR(game.aim_x, 123.0f);
    CHECK_NEAR(game.aim_y, -456.0f);
}

// --- Combat --------------------------------------------------------------

static int count_with(const world_t *world, uint32_t components)
{
    uint32_t needed = COMP_ALIVE | components;
    int count = 0;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if ((world->masks[i] & needed) == needed)
        {
            count++;
        }
    }
    return count;
}

static void test_projectile_damages_enemy(void)
{
    game_init(&game);
    entity_t enemy = enemy_spawn(&game.world, ENEMY_WALKER, 200.0f, 0.0f);

    // Aim at the enemy and hold attack. Basic shot covers the gap well
    // within its lifetime; the enemy also walks toward the player.
    input_state_t input = { 0 };
    input.aim_x = 200.0f;
    input.attack = true;

    int start_health = game.world.healths[enemy.index].current;
    int ticks = 0;
    while (game.world.healths[enemy.index].current == start_health && ticks < 30)
    {
        game_tick(&game, &input);
        ticks++;
    }

    CHECK(ticks < 30);
    CHECK(game.world.healths[enemy.index].current ==
          start_health - projectile_def(PROJ_BASIC_SHOT)->damage);
}

static void test_enemy_dies_and_leaves_corpse(void)
{
    game_init(&game);
    entity_t enemy = enemy_spawn(&game.world, ENEMY_WALKER, 200.0f, 0.0f);

    input_state_t input = { 0 };
    input.aim_x = 200.0f;
    input.attack = true;

    int ticks = 0;
    while (entity_valid(&game.world, enemy) && ticks < 300)
    {
        game_tick(&game, &input);
        ticks++;
    }

    CHECK(!entity_valid(&game.world, enemy));
    CHECK(count_with(&game.world, COMP_CORPSE) == 1);
    CHECK(count_with(&game.world, COMP_ENEMY) == 0);

    input.attack = false;
    for (int i = 0; i <= CORPSE_LIFETIME_TICKS; i++)
    {
        game_tick(&game, &input);
    }
    CHECK(count_with(&game.world, COMP_CORPSE) == 0);
}

static void test_fire_rate_is_cooldown_gated(void)
{
    game_init(&game);

    // Fire into empty space; nothing expires within the window observed.
    input_state_t input = { 0 };
    input.aim_x = 10000.0f;
    input.attack = true;

    for (int i = 0; i < 50; i++)
    {
        game_tick(&game, &input);
    }

    // Casts land on the cooldown boundary: ticks 1, 16, 31, 46.
    int cooldown = ability_def(ABILITY_BASIC_SHOT)->cooldown_ticks;
    CHECK(cooldown == 15);
    CHECK(count_with(&game.world, COMP_PROJECTILE) == 4);
}

static void test_missed_projectile_expires_harmlessly(void)
{
    game_init(&game);
    entity_t enemy = enemy_spawn(&game.world, ENEMY_WALKER, 300.0f, 0.0f);

    // One shot aimed away from the enemy.
    input_state_t input = { 0 };
    input.aim_y = -300.0f;
    input.attack = true;
    game_tick(&game, &input);
    input.attack = false;

    for (int i = 0; i < projectile_def(PROJ_BASIC_SHOT)->lifetime_ticks + 1; i++)
    {
        game_tick(&game, &input);
    }

    CHECK(count_with(&game.world, COMP_PROJECTILE) == 0);
    CHECK(game.world.healths[enemy.index].current ==
          enemy_def(ENEMY_WALKER)->max_health);
}

static void test_projectile_explodes_into_children(void)
{
    game_init(&game);

    // Cast the burst shell into empty space and let it expire.
    ability_cast(&game.world, game.player, ABILITY_BURST_SHOT, 100.0f, 0.0f);
    CHECK(count_with(&game.world, COMP_PROJECTILE) == 1);

    int shell_lifetime = projectile_def(PROJ_BURST_SHELL)->lifetime_ticks;
    for (int i = 0; i < shell_lifetime; i++)
    {
        game_tick(&game, &no_input);
    }

    // The shell died this tick and its children spawned in the same tick's
    // dispatch phase.
    int children = projectile_def(PROJ_BURST_SHELL)->on_death_count;
    CHECK(count_with(&game.world, COMP_PROJECTILE) == children);

    // The radial pattern spreads them in every direction.
    for (int i = 0; i < 5; i++)
    {
        game_tick(&game, &no_input);
    }
    float min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        uint32_t needed = COMP_ALIVE | COMP_PROJECTILE;
        if ((game.world.masks[i] & needed) != needed)
        {
            continue;
        }
        position_t p = game.world.positions[i];
        if (p.x < min_x) min_x = p.x;
        if (p.x > max_x) max_x = p.x;
        if (p.y < min_y) min_y = p.y;
        if (p.y > max_y) max_y = p.y;
    }
    CHECK(max_x - min_x > 20.0f);
    CHECK(max_y - min_y > 20.0f);

    // Shrapnel expires; the world ends up quiet.
    for (int i = 0; i < projectile_def(PROJ_BURST_SHRAPNEL)->lifetime_ticks; i++)
    {
        game_tick(&game, &no_input);
    }
    CHECK(count_with(&game.world, COMP_PROJECTILE) == 0);
}

static void test_enemies_walk_toward_player(void)
{
    game_init(&game);
    entity_t enemy = enemy_spawn(&game.world, ENEMY_WALKER, 400.0f, 0.0f);

    for (int i = 0; i < 60; i++)
    {
        game_tick(&game, &no_input);
    }

    // One second at walker speed closes 100 units of the 400-unit gap.
    CHECK_NEAR(game.world.positions[enemy.index].x,
               400.0f - enemy_def(ENEMY_WALKER)->move_speed);
    CHECK_NEAR(game.world.positions[enemy.index].y, 0.0f);
}

int main(void)
{
    test_entity_create_and_alive();
    test_entity_hard_cap();
    test_deferred_destroy_and_stale_handles();
    test_destroy_component_less_entity();
    test_component_mask_ops();
    test_event_fifo_and_broadcast();
    test_event_cascade_resolves_same_dispatch();
    test_event_runaway_guard();
    test_event_overflow_drops();
    test_game_tick_sweeps();
    test_player_spawns();
    test_player_moves_and_stops();
    test_player_diagonal_not_faster();
    test_aim_is_stored();
    test_projectile_damages_enemy();
    test_enemy_dies_and_leaves_corpse();
    test_fire_rate_is_cooldown_gated();
    test_missed_projectile_expires_harmlessly();
    test_projectile_explodes_into_children();
    test_enemies_walk_toward_player();

    printf("%d checks, %d failed\n", checks_run, checks_failed);
    return checks_failed == 0 ? 0 : 1;
}
