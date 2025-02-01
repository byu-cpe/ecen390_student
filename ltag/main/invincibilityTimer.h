#ifndef INVINCIBILITYTIMER_H_
#define INVINCIBILITYTIMER_H_

#include <stdbool.h>
#include <stdint.h>

// Each time _start is called, the timer runs once for the period specified
// in _init. The timer is started when a life is lost. While the timer is
// active, hits are ignored and the trigger is disabled.

// Initialize the timer.
// period: Specify the timer period in milliseconds.
// Return zero if successful, or non-zero otherwise.
int32_t invincibilityTimer_init(uint32_t period);

// Start the timer from the beginning.
void invincibilityTimer_start(void);

// Return true if the timer is running.
bool invincibilityTimer_running(void);

#endif // INVINCIBILITYTIMER_H_
