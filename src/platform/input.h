#ifndef OPENARPG_PLATFORM_INPUT_H
#define OPENARPG_PLATFORM_INPUT_H

#include "raylib.h"

#include "input_state.h"

// Samples raylib input into the sim's input state; all keybindings live in
// input.c. The camera is needed to map the mouse to a world-space aim point.
input_state_t input_read(Camera2D camera);

#endif
