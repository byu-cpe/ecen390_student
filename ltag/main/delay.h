#ifndef DELAY_H_
#define DELAY_H_

// This module implements a delay line. It acts like an array of elements
// of size N that gets shifted down (to the next higher index) each time a
// new element is saved at index zero. The last element in the array falls
// off the end when shifted. An element retrieved from index zero is the
// most recent element saved. Each higher index goes back in time by one
// time step. The actual implementation uses a circular buffer.

#include <stdint.h>

// Big enough to represent the largest number of elements.
typedef uint32_t delay_size_t;

// Data type for delay elements.
typedef float delay_data_t;

typedef struct {
  delay_size_t pos; // Position (in data[]) of the last element saved.
  delay_size_t size; // Capacity of the queue in elements.
  delay_data_t *data; // Points to a dynamically-allocated array.
} delay_t;

// Initialize the delay line data structure. Allocate memory for the delay
// line (the data* pointer) with space for 'size' elements. If malloc()
// fails, abort() is called to print an error message and terminate. The
// content of the data[] array is initialized to zero.
void delay_init(delay_t *d, delay_size_t size);

// Frees the storage allocated for the delay line.
void delay_free(delay_t *d);

// Reset the entire content of the delay line to zero.
void delay_reset(delay_t *d);

// Save a value to the delay line
void delay_save(delay_t *d, delay_data_t value);

// Read a value from the delay line at the specified index. A zero index
// retrieves the most recent saved value (time zero). Higher indexes will
// retrieve values saved further back in time. if the index is out of
// bound, return zero.
delay_data_t delay_read(delay_t *d, delay_size_t index);

#endif // DELAY_H_
