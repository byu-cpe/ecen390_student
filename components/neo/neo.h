#ifndef NEO_H_
#define NEO_H_

#include <stdbool.h>
#include <stdint.h>

// This component controls a strip of digital RGB LEDs. The color of each
// LED is specified in a byte array and then written serially to the LED
// strip. Specify the GPIO pin connected to the strip at initialization.
// The color of each pixel is represented by a triplet of bytes, in the order
// of green, red, then blue. The length of the array in bytes should be the
// number of LEDs in the strip times 3.

// Initialize the NeoPixel driver.
// gpio_num: GPIO pin number.
// Return zero if successful, or non-zero otherwise.
int32_t neo_init(int32_t gpio_num);

// Free resources used by the NeoPixel driver.
// Return zero if successful, or non-zero otherwise.
int32_t neo_deinit(void);

// Write the pixel data to the LED strip.
// pixels: array of pixel data. Each pixel is represented by 3 bytes (GRB).
// size: the size of the array in bytes.
// wait: if true, block until done writing, otherwise return straight away.
void neo_write(const uint8_t *pixels, uint32_t size, bool wait);

#endif // NEO_H_
