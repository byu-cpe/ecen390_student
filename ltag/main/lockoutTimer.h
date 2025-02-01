#ifndef LOCKOUTTIMER_H_
#define LOCKOUTTIMER_H_

#include <stdbool.h>
#include <stdint.h>

// Each time _start is called, the timer runs once for the period specified
// in _init. The timer is started after a hit is detected. While the timer
// is active, hits are ignored. Only one hit is detected per timer interval.

// Initialize the timer.
// period: Specify the timer period in milliseconds.
// Return zero if successful, or non-zero otherwise.
int32_t lockoutTimer_init(uint32_t period);

// Start the timer from the beginning.
void lockoutTimer_start(void);

// Return true if the timer is running.
bool lockoutTimer_running(void);

#endif // LOCKOUTTIMER_H_
