#ifndef SHOT_H_
#define SHOT_H_

#include <stdbool.h>
#include <stdint.h>

/*
Background: The shot module manages the shot count and can be used to support 
laser tag with a clip that contains N shots. Once a player has fired N shots 
they must wait S seconds for the clip to auto reload. The player may force a 
reload of the clip at any time by pulling and holding the trigger for S seconds 
(if the clip contains shots, the initial press of the trigger will fire a shot).

Implementation: During initialization, the shot count is set to the maximum 
count N. Also, a reload timer is created. The shot count is decremented when a 
user fires a shot and can be reloaded after a delay (S). A call to 
shot_timer_start() initiates a timer that runs once for the period specified in 
shot_init(). At timer expiration, the shot count is reloaded to the maximum 
count and the reload callback function is called. The timer can be stopped 
before expiration with a call to shot_timer_stop(), thus preventing a reload of 
shots.

Usage Instructions: when the trigger is pressed with shots remaining, call 
shot_decrement(). Call shot_timer_start() each time the trigger is pressed to 
start/restart a manual or auto reload sequence. When the trigger is released, 
call shot_timer_stop() if the shot count is greater than zero to prevent a 
manual reload (i.e., the trigger is released before the timer expires). Use the 
reload callback function to play a sound associated with a reload.
*/

// Initialize the shot count to max_shots and create a reload timer.
// max_shots: Specify the maximum shots.
// period: Specify the timer period in milliseconds.
// Return zero if successful, or non-zero otherwise.
int32_t shot_init(uint32_t max_shots, uint32_t period);

// Return the count of remaining shots. Range [0, max_shots]
uint32_t shot_remaining(void);

// Decrement the shot count if greater than zero
void shot_decrement(void);

// Register a function to be called when reload occurs. Default is NULL.
void shot_register_reload(void (*reload)(void));

// Start or restart the timer from the beginning.
// Shots will be reloaded to maximum count if the timer expires.
void shot_timer_start(void);

// Stop the timer. Prevent shot reload.
void shot_timer_stop(void);

// Return true if the timer is running.
bool shot_timer_running(void);

#endif // SHOT_H_
