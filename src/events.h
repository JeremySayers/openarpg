#ifndef OPENARPG_EVENTS_H
#define OPENARPG_EVENTS_H

#include "ecs.h"

// Must stay raylib-free: this is linked into the headless test runner.

// Capacity of the queue itself; pushes beyond this are dropped (and counted).
#define EVENT_QUEUE_CAPACITY 256

// Cascade guard: max events one event_dispatch() call will process before
// concluding the handlers are feeding each other forever.
#define MAX_EVENTS_PER_TICK 1024

typedef enum
{
    EVENT_DAMAGE_DEALT,
    EVENT_ENTITY_DIED,
} event_type_t;

typedef struct
{
    event_type_t type;
    union
    {
        struct { entity_t source; entity_t target; int amount; } damage;
        struct { entity_t entity; entity_t killer; } died;
    };
} event_t;

typedef struct
{
    event_t events[EVENT_QUEUE_CAPACITY];
    int     head;
    int     count;

    // Diagnostics for the (future) debug overlay: total pushes dropped to
    // overflow since init, and the deepest the queue has ever been.
    int     dropped_total;
    int     high_water;
} event_queue_t;

// Handlers receive every event, in the fixed order they appear in the
// handlers array (broadcast - no consuming). They may push new events;
// those are processed in the same dispatch, FIFO.
typedef void (*event_handler_t)(world_t *world, event_queue_t *queue,
                                const event_t *event);

void event_queue_init(event_queue_t *queue);

// Returns false (and logs + counts the drop) when the queue is full.
bool event_push(event_queue_t *queue, event_t event);

// Drains the queue FIFO, offering each event to every handler in order.
// If MAX_EVENTS_PER_TICK is hit, the remaining queue is discarded and an
// error is logged - that means handlers are generating an event loop.
// Returns the number of events processed.
int event_dispatch(world_t *world, event_queue_t *queue,
                   const event_handler_t *handlers, int handler_count);

#endif
