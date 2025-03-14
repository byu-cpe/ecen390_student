#ifndef TEST_SHOOTER_H_
#define TEST_SHOOTER_H_

#include <stdint.h>

// Initialize the application state.
// period: Specify the period in milliseconds between calls to _tick().
// Return zero if successful, or non-zero otherwise.
int32_t test_shooter_init(uint32_t period);

// Standard tick function.
// This function is safe to call from an ISR context.
void test_shooter_tick(void);

// Run tests for shooter mode.
// - Transmit pulse on the current channel when the trigger is pressed.
// - The control option is selected with NAV_LT/RT.
// - The transmit (TX) option (on or shot) is selected with NAV_UP/DN.
// - The channel (CH) is changeable with NAV_UP/DN.
// - The threshold factor (THRESH) is selectable with NAV_UP/DN.
// - 40 shots are initially available. Hits are counted and displayed.
// - Shots and hits can be reset by holding the trigger for 3 sec.
// - Run the receive pipeline (with detector) and display total hits
//   for each channel.
// - This test program can be used to determine the default threshold
//   factor when tag units are separated by 40 ft.
void test_shooter(void);

#endif // TEST_SHOOTER_H_
