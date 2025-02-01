#ifndef TX_H_
#define TX_H_

#include <stdbool.h>
#include <stdint.h>

// The transmitter generates a square-wave output on a GPIO pin at a user
// specified frequency. The GPIO pin is selected at initialization.

// Initialize the transmitter driver.
// gpio_num: GPIO pin number.
// Return zero if successful, or non-zero otherwise.
int32_t tx_init(int32_t gpio_num);

// Set the transmit frequency.
// freq_hz: frequency of the square-wave output signal in Hz.
void tx_set_freq(uint32_t freq_hz);

// Start or stop emitting at the set frequency.
// on: true = start emitting, false = stop emitting.
void tx_emit(bool on);

// Emit a pulse at the set frequency for the duration specified.
// ms: pulse duration in milliseconds.
void tx_pulse(uint32_t ms);

#endif // TX_H_
