#include "raylib.h"

#include "game.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

static const int screen_width = 800;
static const int screen_height = 450;

// Cap on how much elapsed time one render frame may feed the simulation.
// A long hitch (window drag, debugger, laptop sleep) then causes brief
// slow-motion instead of an unbounded catch-up spiral.
static const double max_accumulated_time = 0.25;

static game_t game;
static double accumulator = 0.0;

static void update_draw_frame(void);
static void render(void);

int main(void)
{
    InitWindow(screen_width, screen_height, "OpenARPG");

    game_init(&game);

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
        game_tick(&game);
        accumulator -= TICK_SECONDS;
    }

    render();
}

static void render(void)
{
    BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("OpenARPG", 20, 20, 40, DARKGRAY);
        DrawText("Nothing here yet.", 20, 70, 20, GRAY);

    EndDrawing();
}
