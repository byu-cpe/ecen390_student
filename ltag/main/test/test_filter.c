#include <stdio.h>
#include <stdbool.h>
#include <math.h> // fabsf

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // vTaskDelay
#include "esp_timer.h" // esp_timer_get_time

#include "config.h"
#include "filter.h"
/*******************************************************************************
 * Uncomment the line below to plot a frequency response histogram.
 ******************************************************************************/
#include "histogram.h"

/*******************************************************************************
 * test_filter.c provides a set of self-contained test functions that:
 * 1. test basic functionality of the FIR and IIR filters with a square wave.
 * 2. test output energy calculations.
 * 3. plot the frequency response of the decimating FIR filter.
 * 4. plot the frequency response of each of the IIR filters.
 ******************************************************************************/

#define msDelay(n) vTaskDelay(pdMS_TO_TICKS(n))

/*******************************************************************************
 * Uncomment the line below to enable printing of plot messages.
 ******************************************************************************/
// #define ENABLE_PLOT_MESSAGES 1

#define SAMPLE_FREQ_IN_KHZ (CONFIG_RX_SAMPLE_RATE/1000)

// Additional out-of-band channels used to test the FIR response.
#define OOB_CHAN_COUNT 9

// All channels include the filter channels plus the out-of-band channels.
#define ALL_CHAN_COUNT (FILTER_CHANNELS + OOB_CHAN_COUNT)

// A histogram bar for each channel.
#define HISTOGRAM_BAR_COUNT ALL_CHAN_COUNT
#define MAX_BUF 10 // Used for a temporary char buffer.

// Number of samples to generate in a pulse
#define PULSE_SAMPLES (CONFIG_RX_SAMPLE_RATE*CONFIG_TX_PULSE/1000)

#define MIN_INPUT_VALUE ((filter_data_t)-1.0) // Square wave bottom.
#define MAX_INPUT_VALUE ((filter_data_t)1.0) // Square wave top.
#define INPUT_OFFSET ((filter_data_t)1.0) // Offset for unipolar.

#define ONE_HALF(x) ((x) / 2) // Divide by 2.
#define ONE_HALF_FP(x) ((x) / (filter_data_t)2.0) // FP Divide by 2.0
#define TIMES2_FP(x) ((x) * (filter_data_t)2.0) // FP Multiply by 2.0

#define FABS(x) fabsf(x)
#define FP_EQ_EPSILON ((filter_data_t)1.0E-12)
#define FP_EQ(a,b) (FABS((a) - (b)) < FP_EQ_EPSILON)

#define MAX_ERROR_CNT 5

// Sample counts that are used to generate the filter frequencies.
static const uint16_t filter_frequencyTickTable[FILTER_CHANNELS] =
    {64, 54, 46, 40, 34, 30, 26, 24, 22, 20};

// Sample counts that are used to generate the out-of-band frequencies.
static const uint16_t
    firTestOutOfBandTickCounts[OOB_CHAN_COUNT] =
        {18, 16, 14, 12, 10, 8, 6, 4, 2};

// Sample counts that are used to generate all the test frequencies.
static uint16_t firTestTickCounts[ALL_CHAN_COUNT];

/*******************************************************************************
***** Start of functions
*******************************************************************************/

static bool initFlag = false; // True if the test_filter_init() was called.

// Initialize filterTest. Called by test_filter().
static void test_filter_init(void) {
  // Copy the filter and out-of-bound frequency tick counts to the test array.
  for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
    firTestTickCounts[i] = filter_frequencyTickTable[i];
  }
  for (uint16_t j = FILTER_CHANNELS;
       j < FILTER_CHANNELS + OOB_CHAN_COUNT; j++) {
    firTestTickCounts[j] =
        firTestOutOfBandTickCounts[j - FILTER_CHANNELS];
  }
  initFlag = true;
}

// Helper function to create input waveform.
inline static filter_data_t computeFilterInput(uint16_t sampleCnt, uint16_t samplesPerPeriod) {
  // The period has two halves: the -1.0 part and the 1.0 part.
  return (sampleCnt < ONE_HALF(samplesPerPeriod)) ?
      MIN_INPUT_VALUE // The first half
    :
      MAX_INPUT_VALUE // The second half
    ;
}

// Pushes a single 1.0 through the filter. Golden output is just the FIR
// coefficients. If this test passes, you are multiplying the coefficient
// with the correct delay element. This is equivalent to passing the filter
// over an input containing only a delta function and thus returns the
// impulse response.
static bool firAlignment(void) {
  bool success = true; // Be optimistic.
  uint32_t error_cnt = 0;
  filter_reset(); // zero-out all delay lines.
  // Push a single 1.0 through the filter.
  for (unsigned i = 0; i < filter_getFirCoefCount(); i++) {
    filter_data_t firInput = (filter_data_t)(i == 0);
    filter_data_t firOutput = filter_firFilter(firInput, true);
    // Golden output is simply the FIR coefficient.
    filter_data_t firGoldenOutput = filter_getFirCoefArray()[i];
    // Print message if output does not match the computed golden value.
    if (!FP_EQ(firOutput, firGoldenOutput) && error_cnt < MAX_ERROR_CNT) {
      printf("firAlignment(): Output %.18e, expected %.18e at index %u.\n",
             firOutput, firGoldenOutput, i);
      error_cnt++;
      success = false; // Test failed.
    }
  }
  return success; // Return the success or failure of this test.
}

// Pushes a series of 1.0 values though the filter. Golden output data is
// the sum of the coefficients. The FIR-filter is probably computing outputs
// correctly if you pass this test.
static bool firArithmetic(void) {
  bool success = true; // Be optimistic.
  uint32_t error_cnt = 0;
  filter_reset(); // zero-out all delay lines.
  // Compute the golden output by accumulating the FIR coefficients.
  filter_data_t firGoldenOutput = (filter_data_t)0.0;
  // Loop enough times to go through the coefficients.
  for (unsigned i = 0; i < filter_getFirCoefCount(); i++) {
    filter_data_t firInput = (filter_data_t)1.0; // Only value.
    filter_data_t firOutput = filter_firFilter(firInput, true);
    // Golden output is simply the sum of the coefficient.
    firGoldenOutput += firInput * filter_getFirCoefArray()[i];
    // Print message if output does not match the computed golden value.
    if (!FP_EQ(firOutput, firGoldenOutput) && error_cnt < MAX_ERROR_CNT) {
      printf("firArithmetic(): Output %.18e, expected %.18e at index %u.\n",
             firOutput, firGoldenOutput, i);
      error_cnt++;
      success = false; // Test failed.
    }
  }
  if (!success)
    printf("Ensure that your FIR coefficients have at least %d significant "
           "digits.\n", sizeof(filter_data_t) == 4 ? 8 : 17);
  return success; // Return the success or failure of this test.
}

// Number of samples to send through the energy test.
#define TEST_SAMPLES (FILTER_ENERGY_SAMPLE_COUNT+100)

// TODO: Test energy delay line size?

// Performs a test of the filter_computeEnergy() function.
// Add a single value to each energy delay line each time step.
// Compares the results of filter_computeEnergy with a golden value.
static bool computeEnergy(void) {
  bool success = true; // Be optimistic.
  uint32_t error_cnt = 0;
  // Perform many incremental energy computations.
  filter_data_t input = (filter_data_t)1.0;
  for (unsigned j = 0; j < TEST_SAMPLES; j++) {
    filter_data_t goldenValue = (j < FILTER_ENERGY_SAMPLE_COUNT) ?
      (filter_data_t)(j+1) : (filter_data_t)FILTER_ENERGY_SAMPLE_COUNT;
    for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
      filter_data_t testValue = filter_computeEnergy(i, input);
      if (!FP_EQ(testValue, goldenValue) && error_cnt < MAX_ERROR_CNT) {
        printf("Sample count:%u\n", j);
        // Print out values that indicate the failure.
        printf("computeEnergy() failed for index: %u\n"
               "golden value:           %.18e\n"
               "filter_computeEnergy(): %.18e\n",
               i, goldenValue, testValue);
        printf("Difference between golden value and incrementally computed "
               "value: %.18e\n", FABS(testValue - goldenValue));
        error_cnt++;
        success = false; // Test failed.
      }
    }
  }
  return success;
}

// Tests the filter_addSample() function. Sends a square wave signal through
// all the filter stages. Checks the energy output to see if it is in an
// acceptable range. Checks to see if the decimation factor is correct.
static bool addSample(void)
{
  bool success = true; // Be optimistic.
  uint32_t error_cnt = 0;
  int64_t t1, tt = 0; // Used for timing
  uint32_t cnt = 0; // Count of calls to function under test
  if (!initFlag) {
    printf("Must call test_filter_init() before running any filter tests.\n");
    return false;
  }
  filter_data_t chanEnergy[FILTER_CHANNELS];
  // Simulate a signal received on each filter channel.
  for (uint16_t schan = 0; schan < FILTER_CHANNELS; schan++) {
    filter_reset();
    // Generate a square wave with this many samples per period
    uint16_t samplesPerPeriod = firTestTickCounts[schan];
    uint32_t pulseCnt = 0; // Position in the pulse.
    uint16_t firDecimationCount = 0;
    // Synchronize decimation count
    while (!filter_addSample((filter_data_t)0.0));
    // Generate a pulse-width of periods.
    while (pulseCnt < PULSE_SAMPLES) {
      // This loop completes a single period.
      for (uint16_t sampleCnt = 0; sampleCnt < samplesPerPeriod; sampleCnt++) {
        filter_data_t filterIn = computeFilterInput(sampleCnt, samplesPerPeriod);
        // Code to check the decimation factor
        bool chkDec = ++firDecimationCount == FILTER_FIR_DECIMATION_FACTOR;
        if (chkDec) firDecimationCount = 0;
        // Put the sample data through the filter stages.
        t1 = esp_timer_get_time();
        bool ran = filter_addSample(filterIn);
        if (ran) { // The filters ran.
          tt += esp_timer_get_time() - t1;
          cnt++;
        }
        if (ran != chkDec && error_cnt < MAX_ERROR_CNT) {
          printf("addSample() returned wrong decimation condition at "
              "sample (%u)\n", sampleCnt);
          error_cnt++;
          success = false;
        }
        pulseCnt++; // Go to the next tick.
      }
    }
    filter_getEnergyArray(chanEnergy); // Save the energy for each channel.
    // Check to see if energy values are in an acceptable range.
    // From MATLAB with signal run through in-spec IIR filter:
    // Minimum max1 energy with no attenuation of input signal: 1501
    // Maximum max2 energy with no attenuation of input signal:  226 x 1.1 = 249
    // Minimum max1 energy with 1dB attenuation of input signal: 1192 x .9 = 1072
    // Maximum max2 energy with 1db attenuation of input signal:  179
    // Find top two energies
    uint16_t max1 = 0;
    for (uint16_t i = max1+1; i < FILTER_CHANNELS; i++)
      if (chanEnergy[i] > chanEnergy[max1]) max1 = i;
    uint16_t max2 = (max1 == 0) ? 1 : 0;
    for (uint16_t i = max2+1; i < FILTER_CHANNELS; i++)
      if (i != max1 && chanEnergy[i] > chanEnergy[max2]) max2 = i;
    // Check for errors
    if (schan != max1 && error_cnt < MAX_ERROR_CNT) {
      printf("Filter ch with max energy does not match signal ch:%2u\n", schan);
      printf("max1 filter ch:%2u, energy:%e\n", max1, chanEnergy[max1]);
      error_cnt++;
      success = false;
    } else if ((chanEnergy[max1] < 1072 || chanEnergy[max2] > 249)
         && error_cnt < MAX_ERROR_CNT) {
      printf("Filter out of spec for signal ch:%2u\n", schan);
      printf("max1 filter ch:%2u, energy:%e\n", max1, chanEnergy[max1]);
      printf("max2 filter ch:%2u, energy:%e\n", max2, chanEnergy[max2]);
      error_cnt++;
      success = false;
    }
  }
  printf("filter_addSample() average time:%lld us\n", tt/cnt);
  return success;
}

#ifdef PLOT_INPUT

#define PLOT_COLOR GREEN
#define PLOT_Y0_LINE_COLOR WHITE

// Finds the max in values[].
static filter_data_t findMax(filter_data_t values[], uint32_t size) {
  filter_data_t maxValue = values[0];
  for (uint32_t i = 1; i < size; i++)
    if (values[i] > maxValue)
      maxValue = values[i];
  return maxValue;
}

// Find the min in values[].
static filter_data_t findMin(filter_data_t values[], uint32_t size) {
  filter_data_t minValue = values[0];
  for (uint32_t i = 1; i < size; i++)
    if (values[i] < minValue)
      minValue = values[i];
  return minValue;
}

// Incoming ADC values are assumed unipolar: 0 to some max value. Values are
// scaled in X and Y and mapped so that the plotted range is: -1.0 to 1.0.
// Negative values are plotted on the bottom half of the screen, positive
// values are plotted on the top half. A single horizontal line marks the y
// value of 0.
static void plotInputValues(filter_data_t xValues[], filter_data_t yValues[],
                                       uint32_t size) {
#ifdef HISTOGRAM_H_
  // Assume display is initialized previously
  lcd_setRotation(DIRECTION0);
  lcd_fillScreen(BLACK); // Clear the screen.
  filter_data_t xScale =
      findMax(xValues, size) - findMin(xValues, size); // Scale in x.
  filter_data_t yScale =
      findMax(yValues, size) - findMin(yValues, size); // Scale in y.
  uint16_t y0Point = ONE_HALF(LCD_H); // y0-point is always from the
                                      // y=0 line on the LCD display.
  lcd_drawLine(0, y0Point, LCD_W - 1, y0Point,
                   PLOT_Y0_LINE_COLOR); // Draw the y=0 line.
  for (uint32_t i = 0; i < size; i++) {
    // Convert y-coordinate range to -1.0 to 1.0.
    filter_data_t y1PointFl = TIMES2_FP(yValues[i] / yScale) -
        (filter_data_t)1.0;
    // Scale X by the width of the display.
    uint16_t x0Point = (xValues[i] / xScale) * LCD_W - 1;
    uint16_t x1Point = x0Point;
    if (y1PointFl < (filter_data_t)0.0) { // negative is drawn in lower half.
      uint16_t y1Point =
          (-y1PointFl * ((filter_data_t)ONE_HALF(LCD_H) - 1)) +
          ONE_HALF(LCD_H);
      lcd_drawLine(x0Point, y0Point, x1Point, y1Point, PLOT_COLOR);
    } else { // positive is drawn in upper half.
      uint16_t y1Point =
          y0Point - (y1PointFl)*ONE_HALF(LCD_H); // y = 1.0 = 0 (top of LCD display).
      lcd_drawLine(x0Point, y0Point, x1Point, y1Point, PLOT_COLOR);
    }
  }
#endif // HISTOGRAM_H_
}
#endif // PLOT_INPUT

// Plots the FIR energy (frequency response).
// The filter channels are drawn in blue.
// The remaining out-of-band channels are drawn in red.
// This plotting routine assumes that:
// 1. The first frequencies are the filter frequencies.
// 2. The remaining out-of-band frequencies are between 4 kHz and 40 kHz.
// 3. The number of samples per period for each test frequency is
//    contained in firTestTickCounts[].
static void plotFirFrequencyResponse(filter_data_t firEnergyValues[]) {
#ifdef HISTOGRAM_H_
  // Now we start plotting the results.
  coord_t scaledEnergyValues[ALL_CHAN_COUNT];
  histogram_init(HISTOGRAM_BAR_COUNT); // Init the histogram.
  histogram_scaleFloat(scaledEnergyValues, firEnergyValues, ALL_CHAN_COUNT);

  // Set labels and colors (blue) for the filter channels.
  for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
    histogram_setBarColor(i, BLUE); // Sets the color of the bar.
    char tempLabel[MAX_BUF]; // Temp variable for label generation.
    snprintf(
        tempLabel, MAX_BUF, "%d",
        i); // Create the label that represents one of the filter channels.
    histogram_setBarLabel(i, tempLabel); // Finally, set the label.
  }
  // The following block handles the out-of-band channels. The start
  // and end channel frequencies are printed using labels.

  // Set the colors for the out-of-band channels to be red so that
  // they stand out. Plot the out-of-band frequency response.
  for (uint16_t i = FILTER_CHANNELS; i < ALL_CHAN_COUNT; i++) {
    histogram_setBarColor(i, RED);
    char tempLabel[MAX_BUF]; // Used to create labels.
    if (i == FILTER_CHANNELS) {
      // The lower bound for out-of-bound channels.
      snprintf(tempLabel, MAX_BUF, "%3.1f",
               (filter_data_t)SAMPLE_FREQ_IN_KHZ / firTestTickCounts[i]);
      histogram_setBarLabel(i, tempLabel);
    } else if (i > 12 && i < ALL_CHAN_COUNT-4) {
      // This abuses the labels a bit to show the out-of-band frequencies.
      // Print a "-" to separate the bounds for readability.
      histogram_setBarLabel(i, "-");
    } else if (i == ALL_CHAN_COUNT-4) {
      // Print the end frequency further to the left to make it readable.
      snprintf(tempLabel, MAX_BUF, "%dkHz",
               SAMPLE_FREQ_IN_KHZ / firTestTickCounts[ALL_CHAN_COUNT-1]);
      histogram_setBarLabel(i, tempLabel);
    } else {
      // Print blank space for labels everywhere else.
      histogram_setBarLabel(i, "");
    }
  }
  // Set the actual histogram-bar data to be energy values.
  for (uint16_t barIndex = 0; barIndex < HISTOGRAM_BAR_COUNT;
       barIndex++) {
    histogram_setBarData(barIndex, scaledEnergyValues[barIndex], "");
  }
  histogram_redrawBottomLabels(); // Need to redraw the bottom labels because I
                                  // changed the colors.
  histogram_updateDisplay();      // Redraw the histogram.
#endif // HISTOGRAM_H_
}

// Plot the frequency response of the specified IIR filter over all the
// filter channel frequencies. iirEnergyValues[] contains the computed
// energy from each IIR filter. Histogram bars are drawn in red and blue.
// Red bars represent channels where you want a minimal response. Blue
// histogram bars represent channels where you want a maximal response (when
// a passband matches the frequency). iirEnergyValues are indexed using
// channel numbers : iirEnergyValue[0] contains the computed energy for
// channel 0, from IIR filter 0. iirEnergyValue[9] contains the computed
// energy for channel 9, from IIR filter 9.
static void plotIirFrequencyResponse(filter_data_t iirEnergyValues[],
                                     uint16_t filterChan) {
#ifdef HISTOGRAM_H_
  coord_t scaledEnergyValues[FILTER_CHANNELS];
  histogram_init(FILTER_CHANNELS);
  histogram_scaleFloat(scaledEnergyValues, iirEnergyValues, FILTER_CHANNELS);
  // Set the default label and bar color to red for all channels.
  for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
    histogram_setBarColor(i, RED);
    char tempLabel[MAX_BUF];
    snprintf(tempLabel, MAX_BUF, "%d", i);
    histogram_setBarLabel(i, tempLabel);
  }
  // The filter channel under test is set to blue.
  histogram_setBarColor(filterChan, BLUE);
  for (uint16_t barIndex = 0; barIndex < FILTER_CHANNELS; barIndex++) {
    char label[HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS]; // Get a buffer for
                                                            // the label.
    // Create the top label, based upon the actual energy value. Use floor() +0.5
    // to round the number instead of truncate.
    if (snprintf(label, HISTOGRAM_BAR_TOP_MAX_LABEL_WIDTH_IN_CHARS, "%0.1e",
                 floor(iirEnergyValues[barIndex]) + ONE_HALF_FP(1.0)) == -1)
      printf("Error: snprintf encountered an error during conversion.\n");
    // Pull out the 'e' from the exponent to make better use of space.
    histogram_trimLabel(label);
    histogram_setBarData(barIndex, scaledEnergyValues[barIndex], label);
  }
  histogram_redrawBottomLabels(); // Need to redraw the bottom labels because I
                                  // changed the colors.
  histogram_updateDisplay();      // Redraw the histogram.
#endif // HISTOGRAM_H_
}

// You can collect up to this many values to plot.
#define PLOT_VALUE_MAX_COUNT 360
// The number of period's worth of data to collect and plot.
#define PERIODS_TO_PLOT 2
// The plot will be visible for this long.
#define INPUT_PLOT_VIEW_DELAY 1000

// Plots the frequency response of the FIR filter on the LCD.
// Frequencies range from 1.25 kHz to 40 kHz. Energy is computed
// internally without using the filter_computeEnergy function.
static void firEnergy(void) {
  if (!initFlag) {
    printf("Must call test_filter_init() before running any filter tests.\n");
    return;
  }
  filter_data_t firEnergy = (filter_data_t)0.0; // Energy will be accumulated here.
#ifdef ENABLE_PLOT_MESSAGES
  printf(
      "running firEnergy() - plotting energy for "
      "frequencies %1.2lf kHz to %1.2lf kHz for FIR filter.\n",
      ((filter_data_t)((SAMPLE_FREQ_IN_KHZ)) /
       firTestTickCounts[0]),
      ((filter_data_t)((SAMPLE_FREQ_IN_KHZ)) /
       firTestTickCounts[ALL_CHAN_COUNT - 1]));
#endif
  filter_data_t chanEnergy[ALL_CHAN_COUNT];
#ifdef PLOT_INPUT
  filter_data_t xValues[PLOT_VALUE_MAX_COUNT]; // Store the x-values here.
  filter_data_t yValues[PLOT_VALUE_MAX_COUNT]; // Store the y-values here.
#endif // PLOT_INPUT
  // Supply either 1.0 or -1.0 to the filter input at the frequency being
  // simulated. Iterate over all of the channels.
  for (uint16_t chan = 0; chan < ALL_CHAN_COUNT; chan++) {
    firEnergy = (filter_data_t)0.0; // Reset the energy value.
    uint16_t samplesPerPeriod = firTestTickCounts[chan];
    uint32_t pulseCnt = 0; // Position in the pulse.
    int64_t t1, tt = 0;
    uint32_t cnt = 0;
    // Generate a pulse-width of periods.
    while (pulseCnt < PULSE_SAMPLES) {
      // This loop completes a single period.
      for (uint16_t sampleCnt = 0; sampleCnt < samplesPerPeriod; sampleCnt++) {
        filter_data_t filterIn;
        filterIn = computeFilterInput(sampleCnt, samplesPerPeriod);
        // Put the sample data through the FIR filter.
        static uint16_t firDecimationCount = 0;
        bool runFIR = ++firDecimationCount == FILTER_FIR_DECIMATION_FACTOR;
        t1 = esp_timer_get_time();
        filter_data_t firOutput = filter_firFilter(filterIn, runFIR);
        if (runFIR) {
          tt += esp_timer_get_time() - t1;
          firDecimationCount = 0; // Reset the decimation count.
          cnt++;
          firEnergy += firOutput * firOutput; // Compute the energy so far.
        }
#ifdef PLOT_INPUT
        // You can capture multiple periods of data.
        if (pulseCnt < samplesPerPeriod * PERIODS_TO_PLOT) {
          xValues[pulseCnt] = pulseCnt; // You just need x to increment.
          yValues[pulseCnt] = filterIn +
              INPUT_OFFSET; // The offset ensures that the input is
                            // unipolar for plotting purposes.
        }
#endif // PLOT_INPUT
        pulseCnt++;
      }
    }
#ifdef PLOT_INPUT
    printf("plotting input square wave.\n");
    plotInputValues(xValues, yValues, samplesPerPeriod * PERIODS_TO_PLOT);
    msDelay(INPUT_PLOT_VIEW_DELAY);
#endif // PLOT_INPUT
    chanEnergy[chan] = firEnergy; // Save the energy for each channel.
#ifdef ENABLE_PLOT_MESSAGES
    printf("channel:%2u, energy:%e, time:%lld us\n", chan,
           chanEnergy[chan], tt/cnt);
#endif
  }
  // After running all of the data through the filters, plot it out.
#ifdef ENABLE_PLOT_MESSAGES
  printf("Plotting response to square-wave input.\n");
#endif
  plotFirFrequencyResponse(chanEnergy);
}

// Plots the frequency response for the selected filter channel over all
// the filter channel frequencies using a square-wave. Energy is computed
// internally without using the filter_computeEnergy function.
static void iirEnergy(uint16_t filterChan) {
  if (!initFlag) {
    printf("Must call test_filter_init() before running any filter tests.\n");
    return;
  }
#ifdef ENABLE_PLOT_MESSAGES
  printf("running iirEnergy() - plotting energy for all filter "
         "frequencies for IIR filter(%u).\n",
         filterChan);
#endif
  filter_data_t chanEnergy[FILTER_CHANNELS];
  // Simulate a signal received on each filter channel.
  for (uint16_t chan = 0; chan < FILTER_CHANNELS; chan++) {
    filter_reset();
    filter_data_t energy = (filter_data_t)0.0;
    // Generate a square wave with this many samples per period
    uint16_t samplesPerPeriod = firTestTickCounts[chan];
    uint32_t pulseCnt = 0; // Position in the pulse.
    int64_t t1, tt = 0;
    uint32_t cnt = 0;
    // Generate a pulse-width of periods.
    while (pulseCnt < PULSE_SAMPLES) {
      // This loop completes a single period.
      for (uint16_t sampleCnt = 0; sampleCnt < samplesPerPeriod; sampleCnt++) {
        filter_data_t filterIn = computeFilterInput(sampleCnt, samplesPerPeriod);
        filter_data_t iirOutput; // Output from the IIR filter.
        // Put the sample data through the FIR filter and the IIR filter.
        static uint16_t firDecimationCount = 0;
        bool runFIR = ++firDecimationCount == FILTER_FIR_DECIMATION_FACTOR;
        filter_data_t firOutput = filter_firFilter(filterIn, runFIR);
        if (runFIR) { // Run the IIR filter if the FIR filter ran.
          t1 = esp_timer_get_time();
          iirOutput = filter_iirFilter(filterChan, firOutput);
          tt += esp_timer_get_time() - t1;
          firDecimationCount = 0;
          cnt++;
          energy += iirOutput * iirOutput; // Compute the energy so far.
        }
        pulseCnt++; // Go to the next tick.
      }
    }
    chanEnergy[chan] = energy; // Save the energy for each channel.
#ifdef ENABLE_PLOT_MESSAGES
    printf("channel:%u, energy:%e, time:%lld us\n", chan,
           chanEnergy[chan], tt/cnt);
#endif
  }
  // Finally, plot the results.
  plotIirFrequencyResponse(chanEnergy, filterChan);
}

#define TWO_SECONDS 2000
#define FOUR_SECONDS 4000

// Performs several tests of the filter code.
// 1. Test alignment of FIR constants with input.
// 2. Test the the FIR filter arithmetic.
// 3. Test the output energy calculations for each channel.
// 4. Test all filter stages: FIR & IIR filters, and energy calc.
// 5. Plots the frequency response of the FIR filter on the LCD display.
// 6. Plots the frequency response of each IIR bandpass filter on the
// LCD display. Various informational prints are provided in the
// console during the run of the test.
void test_filter(void) {
  printf("******** test_filter() ********\n");
  bool success = true; // Be optimistic.
  filter_init();       // Always must init stuff.
  test_filter_init();  // More init stuff.

  /* * * * * * * * Critical test functions * * * * * * * */
  // Confirm that the FIR coefficients and data are properly aligned.
  printf("filter_firFilter() alignment test\n");
  success &= firAlignment();

  // Confirm that the FIR properly computes its output.
  printf("filter_firFilter() arithmetic test\n");
  success &= firArithmetic();

  // Verifies correct functionality of the energy computation.
  printf("filter_computeEnergy() test\n");
  success &= computeEnergy();

  // Test all filter stages: FIR & IIR filters, and energy calc.
  printf("filter_addSample() test\n");
  success &= addSample(); 

  printf("******** test_filter() %s ********\n\n", success ? "Done" : "Error");

  // TODO: Check that the frequency response of the FIR and IIR filters
  // is in spec mathematically instead of just visually.

  /* * * * * * * * Plot functions * * * * * * * */
  // Plot the frequency response of the FIR filter in the passband,
  // transition band, and stopband. A square wave is used for the signal.
  firEnergy();
  msDelay(FOUR_SECONDS); // Leave on the display for a few of seconds.
  // Plot the frequency response of each IIR filter over all the
  // filter channel frequencies.
  for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
    iirEnergy(i); // Test FIR & IIR filters.
    msDelay(TWO_SECONDS); // Leave on the display for a few seconds.
  }
}
