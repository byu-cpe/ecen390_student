#ifndef TEST_CONTINUOUS_H_
#define TEST_CONTINUOUS_H_

// Run tests for continuous mode.
// - Transmit continuously on the current channel while the trigger pressed.
// - The channel is changeable with NAV_UP/DN.
// - Run the DSP pipeline and display energy for each channel.
// - Flash hit indicator when NAV_RT is pressed.
// - Play sound when NAV_LT is pressed.
// Assumptions.
// - Transmit signal is looped back through receiver
//   (optically or electrically).
void test_continuous(void);

#endif // TEST_CONTINUOUS_H_
