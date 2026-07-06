#ifndef OPENARPG_PLAYER_H
#define OPENARPG_PLAYER_H

#include "game.h"

void player_control_system(game_t *game, const input_state_t *input);
void player_ability_system(game_t *game, const input_state_t *input);

#endif
