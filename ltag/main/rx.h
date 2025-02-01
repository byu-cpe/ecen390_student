#ifndef RX_H_
#define RX_H_

#include <stdbool.h>
#include <stdint.h>

// The receiver captures an input signal on a GPIO pin. The GPIO pin and
// sample frequency are selected at initialization. The samples are stored in
// a circular buffer until retrieved with rx_get_sample(). Writes to the
// buffer overwrite previous values if they are not retrieved fast enough.

// Receive sample type
typedef uint16_t rx_data_t;

// Initialize the receiver (ADC unit).
// gpio_num: GPIO pin number.
// freq_hz: sample frequency of the input signal in Hz.
// Return zero if successful, or non-zero otherwise.
int32_t rx_init(int32_t gpio_num, uint32_t freq_hz);

// Free resources used by the receiver (ADC unit).
// Return zero if successful, or non-zero otherwise.
int32_t rx_deinit(void);

// Get a sample from the input buffer.
// Returns the next sample
rx_data_t rx_get_sample(void);

// Get a count of samples in the input buffer.
// Returns the number of samples available.
uint32_t rx_get_count(void);

// Clear the input buffer.
void rx_clear_buffer(void);

// Enable or disable capture from the input device.
// enable: if true, enable capture, otherwise disable.
void rx_device(bool enable);

#endif // RX_H_
