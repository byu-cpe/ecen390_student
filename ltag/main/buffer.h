#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>

// This implements a dedicated circular buffer for storing integer values
// from the ADC until they are read and processed. The function of the
// buffer is similar to a queue or FIFO.

// Type of elements in the buffer.
typedef uint16_t buffer_data_t;

// Initialize the buffer to empty.
void buffer_init(void);

// Add a value to the buffer. Overwrite the oldest value if full.
void buffer_pushover(buffer_data_t value);

// Remove a value from the buffer. Return zero if empty.
buffer_data_t buffer_pop(void);

// Return the number of elements in the buffer.
uint32_t buffer_elements(void);

// Return the capacity of the buffer in elements.
uint32_t buffer_size(void);

#endif /* BUFFER_H_ */
