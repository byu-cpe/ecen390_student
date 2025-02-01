#ifndef HITLEDTIMER_H_
#define HITLEDTIMER_H_

#include <stdbool.h>
#include <stdint.h>

// Each time _start is called, the timer runs once for the period specified
// in _init. The timer is started after a hit is detected. While the timer
// is active, the hit LED is turned on.

// Initialize the timer.
// period: Specify the timer period in milliseconds.
// gpio_num: GPIO pin number of the hit LED.
// Return zero if successful, or non-zero otherwise.
int32_t hitLedTimer_init(uint32_t period, int32_t gpio_num);

// Start the timer from the beginning.
void hitLedTimer_start(void);

// Return true if the timer is running.
bool hitLedTimer_running(void);

// Set the default color for the hit LED when no hit is indicated.
// color: An array of color intensities ([0]:red, [1]:green, [2]:blue)
void hitLedTimer_setColor(const uint8_t color[3]);

// Set the color used when a hit is indicated.
// color: An array of color intensities ([0]:red, [1]:green, [2]:blue)
void hitLedTimer_setHitColor(const uint8_t color[3]);

#endif // HITLEDTIMER_H_
