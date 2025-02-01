#ifndef HISTOGRAM_H_
#define HISTOGRAM_H_

#include <stdint.h>

#include "lcd.h"

#define HISTOGRAM_RESERVE_H (LCD_CHAR_H*2*4)
#define HISTOGRAM_TOP_LABEL_TEXT_SIZE 1
#define HISTOGRAM_BOTTOM_LABEL_TEXT_SIZE 2

#define HISTOGRAM_TOP_LABEL_ROWS 2
// Allow up to this many chars for the label on top of the histogram bar.
// Actually printed chars depends upon width of bar.
#define HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS 12
// Allow some room for a label above each bar (in pixels)
#define HISTOGRAM_TOP_LABEL_HEIGHT \
  (LCD_CHAR_H*HISTOGRAM_TOP_LABEL_TEXT_SIZE*HISTOGRAM_TOP_LABEL_ROWS)

#define HISTOGRAM_MAX_BAR_COUNT 25 // Can have up to 25 bars in histogram.
#define HISTOGRAM_BAR_X_GAP 1 // This is the gap, in pixels, between each bar.
#define HISTOGRAM_BAR_Y_GAP (LCD_CHAR_H*HISTOGRAM_BOTTOM_LABEL_TEXT_SIZE)
// Max value (height) for histogram bar, in pixels.
#define HISTOGRAM_MAX_BAR_DATA_IN_PIXELS \
  (LCD_H - HISTOGRAM_BAR_Y_GAP - HISTOGRAM_TOP_LABEL_HEIGHT - 2 \
  - HISTOGRAM_RESERVE_H)
#define HISTOGRAM_MAX_BAR_LABEL_WIDTH 7 // Defined in terms of characters + 1.

// Must call this before using the histogram functions.
void histogram_init(uint16_t barCount);

// This function only updates the data for the histogram.
// histogram_updateDisplay() will do the actual drawing. barIndex is the
// index of the histogram bar, 0 is left, larger indices to the right. data
// is the height of the bar in PIXELS. barTopLabel is a dynamic label that
// will be drawn at the top of the bar. It will be redrawn whenever a
// change is detected.
bool histogram_setBarData(uint16_t barIndex, coord_t data,
                          const char barTopLabel[]);

// Set the bar color for each bar. This overwrites the defaults.
// Call histogram_init() to restore the defaults.
void histogram_setBarColor(uint16_t barIndex, color_t color);

// Set the bottom label drawn under each bar. This overwrites the default.
// Call histogram_init() to restore the defaults.
void histogram_setBarLabel(uint16_t barIndex, const char *label);

// Handy function that shortens a label by removing the 'e' part of the
// exponent. Can be used to create a shortened top label that is drawn
// above the histogram bar.
void histogram_trimLabel(char label[]);

// Simply erases all of the pixels in the label area under the histogram bars
// and redraws the labels.
void histogram_redrawBottomLabels(void);

// Call this to draw the histogram with data set by histogram_setBarData().
// It loops across all bars, checking:
// If the height of the bar has changed, redraw both the bar and the top label.
// If the height of the bar has not changed, but the top label has changed,
// update the label.
void histogram_updateDisplay(void);

// Scale float values to histogram bar height in PIXELS.
void histogram_scaleFloat(coord_t out[], float in[], uint16_t size);

// Scale integer values to histogram bar height in PIXELS.
void histogram_scaleInteger(coord_t out[], uint16_t in[], uint16_t size);

// Plot histogram of float values.
void histogram_plotFloat(float values[], uint16_t size);

// Plot histogram of integer values.
void histogram_plotInteger(uint16_t values[], uint16_t size);

#endif // HISTOGRAM_H_
