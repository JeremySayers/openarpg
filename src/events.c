#include "events.h"

#include <stdio.h>

void event_queue_init(event_queue_t *queue)
{
    queue->head = 0;
    queue->count = 0;
    queue->dropped_total = 0;
    queue->high_water = 0;
}

bool event_push(event_queue_t *queue, event_t event)
{
    if (queue->count == EVENT_QUEUE_CAPACITY)
    {
        queue->dropped_total++;
        fprintf(stderr, "events: queue full (%d), dropping event type %d\n",
                EVENT_QUEUE_CAPACITY, (int)event.type);
        return false;
    }

    int tail = (queue->head + queue->count) % EVENT_QUEUE_CAPACITY;
    queue->events[tail] = event;
    queue->count++;
    if (queue->count > queue->high_water)
    {
        queue->high_water = queue->count;
    }
    return true;
}

int event_dispatch(world_t *world, event_queue_t *queue,
                   const event_handler_t *handlers, int handler_count)
{
    int processed = 0;

    while (queue->count > 0)
    {
        if (processed == MAX_EVENTS_PER_TICK)
        {
            fprintf(stderr,
                    "events: dispatch cap (%d) hit - event loop between "
                    "handlers? discarding %d queued events\n",
                    MAX_EVENTS_PER_TICK, queue->count);
            queue->count = 0;
            break;
        }

        event_t event = queue->events[queue->head];
        queue->head = (queue->head + 1) % EVENT_QUEUE_CAPACITY;
        queue->count--;

        for (int i = 0; i < handler_count; i++)
        {
            handlers[i](world, queue, &event);
        }
        processed++;
    }

    return processed;
}
