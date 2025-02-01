#ifndef FILTER_H_
#define FILTER_H_

// Digital signal processing routines for the laser-tag project.
// Processing is performed in three stages, as described below.
// 1. The first filter is a decimating FIR filter.
// 2. The output from the decimating FIR filter is passed through a bank
// of IIR filters.
// 3. The energy is computed for the output of each IIR filter over a
// window of time.

#include <stdbool.h>
#include <stdint.h>

#include "delay.h"

// Number of IIR filter channels
#define FILTER_CHANNELS 10
// FIR filter needs this many new inputs to compute a new output.
#define FILTER_FIR_DECIMATION_FACTOR 8
// Window for calculating energy in decimated sample counts
#define FILTER_ENERGY_SAMPLE_COUNT 2000

// Type for filter data.
typedef delay_data_t filter_data_t;

/******************************************************************************
***** Main Filter Functions
******************************************************************************/

// Must call this prior to using any filter function.
void filter_init(void);

// Reset filter state to zero.
void filter_reset(void);

// Adds a sample to the filter pipeline and runs each of the stages as
// necessary: decimating FIR filter, IIR filters, power computation.
// Returns true if the filters were run (sample count was a multiple of
// the decimation factor). The result of each power computation is saved
// internally and is retrievable with one of the getEnergy functions.
bool filter_addSample(filter_data_t in);

// Invoke the FIR filter. Control decimation with the 'run' parameter.
// in:  Input to the filter.
// run: If true, perform computation; otherwise, skip computation.
// returns: The filter output when 'run' is true; otherwise zero.
filter_data_t filter_firFilter(filter_data_t in, bool run);

// Invoke the IIR filter.
// chan: Specify which channel.
// in:   Input to the filter.
// returns: The filter output.
filter_data_t filter_iirFilter(uint16_t chan, filter_data_t in);

// Incrementally compute the energy over a window of values stored in
// a buffer. A new value is added to the buffer displacing the oldest.
// chan: Specify which channel.
// in:   Next value to be stored in the buffer.
// returns: The total energy (sum of the square of each element).
filter_data_t filter_computeEnergy(uint16_t chan, filter_data_t in);

// Retrieve the current energy value for a channel.
// chan: Specify which channel.
// returns: The energy value.
filter_data_t filter_getEnergyValue(uint16_t chan);

// Copy all current energy values to the specified array.
// energy: Array that will be populated upon return.
void filter_getEnergyArray(filter_data_t energy[]);

/******************************************************************************
***** Verification-Assisting Functions
***** External test functions access the internal data structures of filter.c
***** via these functions. They are not used by the main filter functions.
******************************************************************************/

// Returns the array of FIR coefficients.
const filter_data_t *filter_getFirCoefArray(void);

// Returns the number of FIR coefficients.
uint32_t filter_getFirCoefCount(void);

// Returns the array of coefficients for a channel.
// chan: Specify which channel.
const filter_data_t *filter_getIirSosCoefArray(uint16_t chan);

// Returns the number of IIR second-order sections.
uint32_t filter_getIirSosSectionCount(void);

// Returns the number of IIR coefficients in a second-order section.
uint32_t filter_getIirSosCoefCount(void);

#endif // FILTER_H_
