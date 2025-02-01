#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "histogram.h"

static uint16_t histogram_barCount;
static coord_t histogram_barWidth; // Width of a bar.
static uint16_t topLabelMaxWidthInChars; // How many chars will be printed.
static coord_t
    currentBarData[HISTOGRAM_MAX_BAR_COUNT]; // Current histogram data.
static coord_t
    previousBarData[HISTOGRAM_MAX_BAR_COUNT]; // Keep the old stuff around so
                                              // you can erase things properly.
static char
    topLabel[HISTOGRAM_MAX_BAR_COUNT]
            [HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS]; // Labels at top of
                                                          // histogram bars.
static char
    oldTopLabel[HISTOGRAM_MAX_BAR_COUNT]
               [HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS]; // Old label so you
                                                             // only update as
                                                             // necessary.

#define ONE_HALF(x) ((x) / 2) // Integer divide by 2.

static bool initFlag =
    false; // Keep track whether histogram_init() has been called.
// These are the default colors for the bars.
const static color_t histogram_defaultBarColors[HISTOGRAM_MAX_BAR_COUNT] = {
    BLUE,    RED,    GREEN,   CYAN,
    MAGENTA, YELLOW, WHITE,   BLUE,
    RED,     GREEN,  BLUE,    RED,
    GREEN,   CYAN,   MAGENTA, YELLOW,
    WHITE,   BLUE,   RED,     GREEN,
    BLUE,    RED,    GREEN,   CYAN,
    MAGENTA};
static color_t histogram_barColors[HISTOGRAM_MAX_BAR_COUNT];
// Default colors for the white dynamic labels.
const static color_t
    histogram_defaultBarTopLabelColors[HISTOGRAM_MAX_BAR_COUNT] = {
        WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE,
        WHITE};
static color_t histogram_barTopLabelColors[HISTOGRAM_MAX_BAR_COUNT];
// Default labels for the histogram bars.
// These labels do not change during operation.
const static char histogram_defaultLabel[HISTOGRAM_MAX_BAR_COUNT]
                                        [HISTOGRAM_MAX_BAR_LABEL_WIDTH] = {
                                            {"0"}, {"1"}, {"2"}, {"3"}, {"4"},
                                            {"5"}, {"6"}, {"7"}, {"8"}, {"9"},
                                            {"A"}, {"B"}, {"C"}, {"D"}, {"E"},
                                            {"F"}, {"G"}, {"H"}, {"I"}, {"J"},
                                            {"K"}, {"L"}, {"M"}, {"N"}, {"O"}};
static char histogram_label[HISTOGRAM_MAX_BAR_COUNT]
                           [HISTOGRAM_MAX_BAR_LABEL_WIDTH];

static void strnzcpy(char *dest, const char *src, size_t count)
{
  while (count && (*dest++ = *src++)) count--;
  if (!count) dest[-1] = '\0';
}

// Internal helper function.
// Erases the old text (using a fillRect because it is small and fast) to erase
// the old label, if required. Finds the position for the label, just above the
// top of the bar.
static void histogram_drawTopLabel(uint16_t barIndex, coord_t data,
                            const char *topLabel, bool eraseOldLabel) {
  coord_t x = barIndex * (histogram_barWidth + HISTOGRAM_BAR_X_GAP);
  coord_t y = LCD_H - data - HISTOGRAM_BAR_Y_GAP - HISTOGRAM_TOP_LABEL_HEIGHT - 1;
  if (eraseOldLabel) {
    // Erase with a fillRect because the rect is small and should be faster than
    // hitting individual label pixels.
    lcd_fillRect(x, y, histogram_barWidth, HISTOGRAM_TOP_LABEL_HEIGHT, BLACK);
  }
  uint16_t len = strlen(topLabel);
  if (len > topLabelMaxWidthInChars*HISTOGRAM_TOP_LABEL_ROWS)
    len = topLabelMaxWidthInChars*HISTOGRAM_TOP_LABEL_ROWS;
  // uint16_t lwc = (len < topLabelMaxWidthInChars) ? len : topLabelMaxWidthInChars; // label width in char
  uint16_t lhc = (len-1) / topLabelMaxWidthInChars + 1; // label height in char
  uint16_t lwc = (len-1) / lhc + 1; // label width in char
  // This helps to center the label over the bar.
  coord_t topLabelXOffset = (histogram_barWidth - (lwc * LCD_CHAR_W))/2;
  lcd_setFontSize(HISTOGRAM_TOP_LABEL_TEXT_SIZE); // Use tiny text.
  // Wrap text to next row if needed.
  for (uint16_t r = HISTOGRAM_TOP_LABEL_ROWS-lhc; r < HISTOGRAM_TOP_LABEL_ROWS; r++)
    for (uint16_t c = 0; c < lwc && len; c++, len--)
      lcd_drawChar(
        x + topLabelXOffset + c*LCD_CHAR_W,
        y + r*LCD_CHAR_H,
        *topLabel++,
        histogram_barTopLabelColors[barIndex]
      );
}

// The bottom labels are drawn at the bottom of the bar and are static.
static void histogram_drawBottomLabels() {
  coord_t labelOffset =
      ONE_HALF(histogram_barWidth -
               (LCD_CHAR_W *
                HISTOGRAM_BOTTOM_LABEL_TEXT_SIZE));       // Center the label.
  lcd_setFontSize(HISTOGRAM_BOTTOM_LABEL_TEXT_SIZE); // Set the text-size.
  for (uint16_t i = 0; i < histogram_barCount; i++) {          //
    lcd_drawString(
      i * (histogram_barWidth + HISTOGRAM_BAR_X_GAP) + labelOffset,
      LCD_H - (LCD_CHAR_H * HISTOGRAM_BOTTOM_LABEL_TEXT_SIZE),
      histogram_label[i],
      histogram_barColors[i]
    );
  }
}

// Must call this before using the histogram functions.
void histogram_init(uint16_t barCount) {
  // Assume display is initialized previously
  histogram_barCount = barCount;
  if (histogram_barCount > HISTOGRAM_MAX_BAR_COUNT) {
    printf("Error! histogram_init(): histogram_barCount (%d) is larger than "
           "the max allowed (%d)",
           histogram_barCount, HISTOGRAM_MAX_BAR_COUNT);
    exit(0);
  }
  histogram_barWidth =
      (LCD_W / histogram_barCount) - HISTOGRAM_BAR_X_GAP;
  topLabelMaxWidthInChars =
      (histogram_barWidth /
       LCD_CHAR_W); // The top-label can be this wide.
  // But, double-check to make sure that it will fit in the memory allocated for
  // the label array. Set to fit allocated area in any case. The -1 allows space
  // for the 0 that ends the string.
  topLabelMaxWidthInChars =
      (topLabelMaxWidthInChars <
       (HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS - 1))
          ? topLabelMaxWidthInChars
          : HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS - 1;
  for (uint16_t i = 0; i < histogram_barCount; i++) {
    currentBarData[i] = 0;
    previousBarData[i] = 0;
    topLabel[i][0] = 0;    // Start out with empty strings.
    oldTopLabel[i][0] = 0; // Start out with empty strings.
  }
  for (uint16_t i = 0; i < HISTOGRAM_MAX_BAR_COUNT; i++) {
    strnzcpy(histogram_label[i], histogram_defaultLabel[i],
            HISTOGRAM_MAX_BAR_LABEL_WIDTH);
    histogram_barColors[i] = histogram_defaultBarColors[i];
    histogram_barTopLabelColors[i] = histogram_defaultBarTopLabelColors[i];
  }
  lcd_fillRect(0, HISTOGRAM_RESERVE_H, LCD_W, LCD_H-HISTOGRAM_RESERVE_H, BLACK);

  histogram_drawBottomLabels();
  initFlag = true;
}

// This function only updates the data for the histogram.
// histogram_updateDisplay() will do the actual drawing. barIndex is the
// index of the histogram bar, 0 is left, larger indices to the right. data
// is the height of the bar in PIXELS. barTopLabel is a dynamic label that
// will be drawn at the top of the bar. It will be redrawn whenever a
// change is detected.
bool histogram_setBarData(uint16_t barIndex, coord_t data,
                          const char barTopLabel[]) {
  if (!initFlag) {
    printf("Error! histogram_setBarData(): must call histogram_init() before "
           "calling this function.\n");
    return false;
  }
  // Error checking.
  if (barIndex > histogram_barCount) {
    printf("Error! histogram_setBarData(): barIndex(%d) is greater than "
           "maximum (%d)\n",
           barIndex, histogram_barCount - 1);
    return false;
  }
  // Error checking.
  if (data > HISTOGRAM_MAX_BAR_DATA_IN_PIXELS) {
    printf("Error! histogram_setBarData(): data (%d) is greater than maximum "
           "(%d) for index(%u)\n",
           (int)data, HISTOGRAM_MAX_BAR_DATA_IN_PIXELS - 1, barIndex);
    return false;
  }
  // If it has changed, update the data in the array but don't render anything
  // on the display.
  if (data !=
      currentBarData[barIndex]) { // Only modify the data if it has changed.
    previousBarData[barIndex] =
        currentBarData[barIndex]; // Save the old data so you render things
                                  // properly.
    // Copy the old string so that you can render things properly.
    currentBarData[barIndex] = data; // Store the data because it changed.
  }
  // If the label has changed, move the previous data to old, new data to
  // current. Labels are handled separately from data because the label may
  // change even if the underlying bar data does not. This allows the top label
  // to change and to be redrawn even if the bars stay the same height.
  if (strncmp(barTopLabel, topLabel[barIndex],
              HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS)) {
    // If you get here, the new label is different from the last one.
    strnzcpy(oldTopLabel[barIndex], topLabel[barIndex],
            HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS);
    // Copy the current label to become the new old label.
    strnzcpy(topLabel[barIndex], barTopLabel,
            HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS);
  }
  return true; // Everything is OK.
}

// Set the bar color for each bar. This overwrites the defaults.
// Call histogram_init() to restore the defaults.
void histogram_setBarColor(uint16_t barIndex, color_t color) {
  if (/*barIndex < 0 ||*/ barIndex > HISTOGRAM_MAX_BAR_COUNT) {
    printf("Error! histogram_setBarColor: barIndex(%d) not in range.\n",
           barIndex);
    return;
  }
  histogram_barColors[barIndex] = color;
}

// Set the bottom label drawn under each bar. This overwrites the default.
// Call histogram_init() to restore the defaults.
void histogram_setBarLabel(uint16_t barIndex, const char *label) {
  if (/*barIndex < 0 ||*/ barIndex > HISTOGRAM_MAX_BAR_COUNT) {
    printf("Error! histogram_setBarLabel: barIndex(%d) not in range.\n",
           barIndex);
    return;
  }
  strnzcpy(histogram_label[barIndex], label, HISTOGRAM_MAX_BAR_LABEL_WIDTH);
}

// Handy function that shortens a label by removing the 'e' part of the
// exponent. Can be used to create a shortened top label that is drawn
// above the histogram bar.
#define EXPONENT_CHARACTER 'e'
void histogram_trimLabel(char label[]) {
  uint16_t len = strlen(label); // Get the length of the label.
  bool found_e = false;         // You looking for the exponent character.
  uint16_t e_index = 0; // will point to the exponent character when completed.
  // Look for the e and keeps its index.
  for (uint16_t i = 0; i < len; i++) {
    if (label[i] == EXPONENT_CHARACTER) {
      found_e = true; // found the exponent character.
      e_index = i;    // Note its position.
      break;
    }
  }
  // If you found the "e", just copy over it.
  if (found_e) {
    for (uint16_t j = e_index; j < len; j++) {
      label[j] = label[j + 1];
    }
  }
}

// Simply erases all of the pixels in the label area under the histogram bars
// and redraws the labels.
void histogram_redrawBottomLabels(void) {
  lcd_fillRect2(
    0, LCD_H - (LCD_CHAR_H * HISTOGRAM_BOTTOM_LABEL_TEXT_SIZE),
    LCD_W-1, LCD_H-1, BLACK);
  histogram_drawBottomLabels();
}

// Call this to draw the histogram with data set by histogram_setBarData().
// It loops across all bars, checking:
// If the height of the bar has changed, redraw both the bar and the top label.
// If the height of the bar has not changed, but the top label has changed,
// update the label.
void histogram_updateDisplay(void) {
  if (!initFlag) {
    printf("Error! histogram_updateDisplay(): must call histogram_init() "
           "before calling this function.\n");
    return;
  }
  for (uint16_t i = 0; i < histogram_barCount; i++) {
    coord_t oldData = previousBarData[i]; // Get the previous data.
    coord_t data = currentBarData[i];     // Get the current bar data.
    if (oldData !=
        data) { // If not equal, redraw the bar and the top-label.
      // Erase the old bar and extend the erase rectangle to include the
      // top-label so that everything is erased at once. Also, redraw the top
      // label.
      coord_t x = i * (histogram_barWidth + HISTOGRAM_BAR_X_GAP);
      coord_t y = LCD_H - oldData - HISTOGRAM_BAR_Y_GAP - HISTOGRAM_TOP_LABEL_HEIGHT - 1;
      lcd_fillRect(x, y,
        histogram_barWidth, oldData+HISTOGRAM_TOP_LABEL_HEIGHT,
        BLACK);
      if (data != 0) { // Only draw the bar & top label if the bar data != 0.
        // Draw the new bar.
        y = LCD_H - data - HISTOGRAM_BAR_Y_GAP - 1;
        lcd_fillRect(x, y,
          histogram_barWidth, data,
          histogram_barColors[i]);
        histogram_drawTopLabel(i, data, topLabel[i],
                               false); // false means that the old label does
                                       // not need to be erased.
        previousBarData[i] = currentBarData[i]; // Old data and new data are the
                                                // same after the update.
        // Old label and new label are the same after the update.
        strnzcpy(oldTopLabel[i], topLabel[i],
                HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS);
      }
    } else if ((data != 0) &&
               strncmp(topLabel[i], oldTopLabel[i],
                       HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS)) {
      histogram_drawTopLabel(
          i, data, topLabel[i],
          true); // True means that the old label needs to be erased.
      // After the update, copy the label to old data so that it won't
      // re-update until the next change.
      strnzcpy(oldTopLabel[i], topLabel[i],
              HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS);
    }
  }
}

// Scale float values to histogram bar height in PIXELS.
void histogram_scaleFloat(coord_t out[], float in[], uint16_t size) {
  float max = 0.0f;
  // First, find the max. value in the input array.
  for (uint16_t i = 0; i < size; i++) {
    if (in[i] > max)
      max = in[i];
  }
  if (max == 0.0f)
    max = 1.0f;
  // Scale between zero and max bar height.
  for (uint16_t i = 0; i < size; i++) {
    if (in[i] < 0.0f)
      out[i] = 0;
    else
      out[i] = in[i] / max * HISTOGRAM_MAX_BAR_DATA_IN_PIXELS;
  }
}

// Scale integer values to histogram bar height in PIXELS.
void histogram_scaleInteger(coord_t out[], uint16_t in[], uint16_t size) {
  int max = 0;
  // First, find the max. value in the input array.
  for (uint16_t i = 0; i < size; i++) {
    if (in[i] > max)
      max = in[i];
  }
  if (max == 0)
    max = 1;
  // Scale between zero and max bar height.
  for (uint16_t i = 0; i < size; i++) {
    out[i] = (float)in[i] / max * HISTOGRAM_MAX_BAR_DATA_IN_PIXELS;
  }
}

// Plot histogram of float values.
void histogram_plotFloat(float values[], uint16_t size) {
  if (size > histogram_barCount) {
    printf("Error! histogram_plotFloat(): size too large.\n");
    return;
  }
  coord_t out[size]; // Array of scaled values for the histogram.
  histogram_scaleFloat(out, values, size); // Scale the values.
  for (uint16_t i = 0; i < size; i++) {
    char label[HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS];
    // Create the label, based upon the hit count.
    if (snprintf(label, HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS, "%0.1e",
                 values[i]) == -1)
      printf("Error! snprintf encountered an error during conversion.\n");
    // Pull out the 'e' from the exponent to make better use of space.
    histogram_trimLabel(label);
    histogram_setBarData(i, out[i], label);
    histogram_updateDisplay(); // Redraw the histogram.
  }
}

// Plot histogram of integer values.
void histogram_plotInteger(uint16_t values[], uint16_t size) {
  if (size > histogram_barCount) {
    printf("Error! histogram_plotInteger(): size too large.\n");
    return;
  }
  coord_t out[size]; // Array of scaled values for the histogram.
  histogram_scaleInteger(out, values, size); // Scale the values.
  for (uint16_t i = 0; i < size; i++) {
    char label[HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS];
    // Create the label, based upon the hit count.
    if (snprintf(label, HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS, "%d",
                 values[i]) == -1)
      printf("Error! snprintf encountered an error during conversion.\n");
    histogram_setBarData(i, out[i], label);
    histogram_updateDisplay(); // Redraw the histogram.
  }
}

#if 0
// Runs a short test that writes random values to the histogram bar-values as
// specified by the #defines below.
#define RANDOM_LABEL "99"
#define HISTOGRAM_RUN_TEST_ITERATION_COUNT 10
#define HISTOGRAM_RUN_TEST_LOOP_DELAY_MS 500
void histogram_runTest() {
  histogram_init(
      HISTOGRAM_DEFAULT_BAR_COUNT); // Must init the histogram data structures.
  for (uint16_t i = 0; i < HISTOGRAM_RUN_TEST_ITERATION_COUNT;
       i++) { // Loop as required.
    for (uint16_t j = 0; j < histogram_barCount;
         j++) { // set each of the bar values.
      histogram_setBarData(j, rand() % HISTOGRAM_MAX_BAR_DATA_IN_PIXELS,
                           RANDOM_LABEL); // set the bar data to a random value.
    }
    histogram_updateDisplay();                       // update the display.
    utils_msDelay(HISTOGRAM_RUN_TEST_LOOP_DELAY_MS); // Slow the update so you
                                                     // can see it happen.
  }
}
#endif
