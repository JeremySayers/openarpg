#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

static const int screen_width = 800;
static const int screen_height = 450;

static void update_draw_frame(void);

int main(void)
{
    InitWindow(screen_width, screen_height, "OpenARPG");

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(update_draw_frame, 60, 1);
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
    BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("OpenARPG", 20, 20, 40, DARKGRAY);
        DrawText("Nothing here yet.", 20, 70, 20, GRAY);

    EndDrawing();
}
