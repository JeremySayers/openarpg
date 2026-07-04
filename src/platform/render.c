#include "render.h"

#include <math.h>

static const float grid_spacing = 100.0f;
static const float player_radius = 16.0f;

static Camera2D camera;

static void draw_grid(void);

void render_init(void)
{
    camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.target = (Vector2){ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

Camera2D render_camera(void)
{
    return camera;
}

void render_follow_player(const game_t *game)
{
    position_t player_pos = game->world.positions[game->player.index];
    camera.target = (Vector2){ player_pos.x, player_pos.y };
}

void render_frame(const game_t *game)
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

            draw_grid();

            position_t player_pos = game->world.positions[game->player.index];
            Vector2 player = { player_pos.x, player_pos.y };
            DrawCircleV(player, player_radius, MAROON);

            // Aim indicator: a short line from the player toward the cursor.
            float dx = game->aim_x - player.x;
            float dy = game->aim_y - player.y;
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
        (Vector2){ (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, camera);

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
