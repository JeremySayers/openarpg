#include "raylib.h"

#include <math.h>

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

static const float grid_spacing = 100.0f;
static const float player_radius = 16.0f;

static game_t game;
static double accumulator = 0.0;
static Camera2D camera;

static void update_draw_frame(void);
static input_state_t read_input(void);
static void render(void);
static void draw_grid(void);

int main(void)
{
    InitWindow(screen_width, screen_height, "OpenARPG");

    game_init(&game);

    camera.offset = (Vector2){ screen_width / 2.0f, screen_height / 2.0f };
    camera.target = (Vector2){ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

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
    input_state_t input = read_input();

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

    position_t player_pos = game.world.positions[game.player.index];
    camera.target = (Vector2){ player_pos.x, player_pos.y };

    render();
}

static input_state_t read_input(void)
{
    input_state_t input = { 0 };

    if (IsKeyDown(KEY_D)) input.move_x += 1.0f;
    if (IsKeyDown(KEY_A)) input.move_x -= 1.0f;
    if (IsKeyDown(KEY_S)) input.move_y += 1.0f;
    if (IsKeyDown(KEY_W)) input.move_y -= 1.0f;

    Vector2 aim = GetScreenToWorld2D(GetMousePosition(), camera);
    input.aim_x = aim.x;
    input.aim_y = aim.y;

    return input;
}

static void render(void)
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

            draw_grid();

            position_t player_pos = game.world.positions[game.player.index];
            Vector2 player = { player_pos.x, player_pos.y };
            DrawCircleV(player, player_radius, MAROON);

            // Aim indicator: a short line from the player toward the cursor.
            float dx = game.aim_x - player.x;
            float dy = game.aim_y - player.y;
            float length = sqrtf(dx * dx + dy * dy);
            if (length > 1.0f)
            {
                float scale = (player_radius + 12.0f) / length;
                Vector2 tip = { player.x + dx * scale, player.y + dy * scale };
                DrawLineEx(player, tip, 3.0f, DARKGRAY);
            }

        EndMode2D();

        DrawText("WASD to move, mouse to aim", 10, 10, 20, GRAY);

    EndDrawing();
}

static void draw_grid(void)
{
    // Cover just the visible world rect, whatever the camera position.
    Vector2 top_left = GetScreenToWorld2D((Vector2){ 0.0f, 0.0f }, camera);
    Vector2 bottom_right = GetScreenToWorld2D(
        (Vector2){ (float)screen_width, (float)screen_height }, camera);

    float start_x = floorf(top_left.x / grid_spacing) * grid_spacing;
    float start_y = floorf(top_left.y / grid_spacing) * grid_spacing;

    for (float x = start_x; x <= bottom_right.x; x += grid_spacing)
    {
        DrawLineV((Vector2){ x, top_left.y }, (Vector2){ x, bottom_right.y },
                  LIGHTGRAY);
    }
    for (float y = start_y; y <= bottom_right.y; y += grid_spacing)
    {
        DrawLineV((Vector2){ top_left.x, y }, (Vector2){ bottom_right.x, y },
                  LIGHTGRAY);
    }

    // Mark the world origin so there's a fixed landmark.
    DrawCircleV((Vector2){ 0.0f, 0.0f }, 6.0f, SKYBLUE);
}
