// Headless tests for the raylib-free simulation core (ecs, events).
// Plain assert-macro runner: every CHECK failure prints and counts, the
// process exits nonzero if any failed. No framework on purpose.

#include <stdio.h>

#include "ecs.h"
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

static void test_game_tick_sweeps(void)
{
    game_init(&game);

    entity_t e = entity_create(&game.world);
    entity_destroy(&game.world, e);
    CHECK(entity_valid(&game.world, e));

    game_tick(&game);

    CHECK(!entity_valid(&game.world, e));
    CHECK(game.tick_count == 1);
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

    printf("%d checks, %d failed\n", checks_run, checks_failed);
    return checks_failed == 0 ? 0 : 1;
}
