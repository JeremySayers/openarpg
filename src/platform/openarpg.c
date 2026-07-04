#include "raylib.h"

#include "game.h"
#include "input.h"
#include "render.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// Cap on how much elapsed time one render frame may feed the simulation.
// A long hitch (window drag, debugger, laptop sleep) then causes brief
// slow-motion instead of an unbounded catch-up spiral.
static const double max_accumulated_time = 0.25;

static game_t game;
static double accumulator = 0.0;

static void update_draw_frame(void);

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenARPG");

    game_init(&game);
    render_init();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(update_draw_frame, 0, 1);
#else
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        update_draw_frame();
    }
#endif

    CloseWindow();

    return 0;
}

static void update_draw_frame(void)
{
    input_state_t input = input_read(render_camera());

    // Fixed-timestep simulation: accumulate real elapsed time, run whole
    // ticks out of it, render once. A slow render frame runs several
    // catch-up ticks, so the world keeps correct speed regardless of fps.
    accumulator += GetFrameTime();
    if (accumulator > max_accumulated_time)
    {
        accumulator = max_accumulated_time;
    }

    while (accumulator >= TICK_SECONDS)
    {
        game_tick(&game, &input);
        accumulator -= TICK_SECONDS;
    }

    render_follow_player(&game);
    render_frame(&game);
}
