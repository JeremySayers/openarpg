#ifndef OPENARPG_INPUT_STATE_H
#define OPENARPG_INPUT_STATE_H

// What the simulation reads each tick. The platform layer translates raw
// raylib input into this once per render frame, so keybindings live in one
// place and the sim stays raylib-free.
typedef struct
{
    float move_x; // -1..1; +x is right
    float move_y; // -1..1; +y is down
    float aim_x;  // world-space point under the cursor
    float aim_y;
} input_state_t;

#endif
