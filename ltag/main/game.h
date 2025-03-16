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

// This game supports multiple teams.
// Each team operates on its own configurable frequency channel.
// Each player has a fixed set of lives and once they
// have expended all lives, operation ceases and they are told
// to return to base to await the ultimate end of the game.
// The tag unit has a clip that contains a fixed number of shots.
// The clip can be manually reloaded, after a short time interval.
// The clip is automatically reloaded when empty, after a short time.
void game_loop(void);

#endif // GAME_H_
