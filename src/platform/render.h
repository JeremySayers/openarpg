#ifndef OPENARPG_PLATFORM_RENDER_H
#define OPENARPG_PLATFORM_RENDER_H

#include "raylib.h"

#include "game.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

void render_init(void);

// The camera is owned here; input sampling borrows it to convert the mouse
// position into world space.
Camera2D render_camera(void);

void render_follow_player(const game_t *game);
void render_frame(const game_t *game);

#endif
