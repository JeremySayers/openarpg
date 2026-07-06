#include "input.h"

input_state_t input_read(Camera2D camera)
{
    input_state_t input = { 0 };

    if (IsKeyDown(KEY_D)) input.move_x += 1.0f;
    if (IsKeyDown(KEY_A)) input.move_x -= 1.0f;
    if (IsKeyDown(KEY_S)) input.move_y += 1.0f;
    if (IsKeyDown(KEY_W)) input.move_y -= 1.0f;

    Vector2 aim = GetScreenToWorld2D(GetMousePosition(), camera);
    input.aim_x = aim.x;
    input.aim_y = aim.y;

    input.attack = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    return input;
}
