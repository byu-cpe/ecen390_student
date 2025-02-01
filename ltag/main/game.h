#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>

// Initialize the game state.
// period: Specify the period in milliseconds between calls to game_tick().
// Return zero if successful, or non-zero otherwise.
int32_t game_init(uint32_t period);

// Standard tick function.
// This function is safe to call from an ISR context.
void game_tick(void);

// This game supports two teams, Team-A and Team-B.
// Each team operates on its own configurable frequency.
// Each player has a fixed set of lives and once they
// have expended all lives, operation ceases and they are told
// to return to base to await the ultimate end of the game.
// The gun is clip-based and each clip contains a fixed number of shots
// that takes a short time to reload a new clip.
// The clips are automatically loaded.
void game_twoTeamTag(void);

#endif // GAME_H_
