#ifndef DETECTOR_H_
#define DETECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "filter.h"

// Provides support for detecting hits based on the energy values
// output from the filter stages.

// Initialize the detector module.
// By default, all channels are considered for hits.
// Assumes the filter module is initialized previously.
void detector_init(void);

// Set channels that are enabled for hit detection. The chanArray
// is indexed by channel number (0-9). If an element is set to true,
// the channel will be enabled. Enabling all but your own transmit
// channel is a good default.
void detector_setChannels(bool chanArray[]);

// Set the threshold factor used in determining a hit. A lower threshold
// factor gives a greater sensitivity to hits.
void detector_setThreshFactor(filter_data_t tfac);

// The detector will ignore all hits if the flag is true, otherwise it
// will respond to hits normally. Used to provide limited invincibility
// in some game modes.
void detector_ignoreAllHits(bool flagValue);

// Check for a hit. This is the core hit detection function.
// Inputs:
//   energy values from each channel
//   skip detection if ignoring hits or a previous hit has not been cleared
//   only consider enabled channels
// Outputs: sets hit status variables retrievable with
//   detector_getHit(void)
//   detector_getHitChannel(void)
void detector_checkHit(filter_data_t energyValues[]);

// Returns true if a hit was detected.
bool detector_getHit(void);

// Returns the channel that had a hit.
uint16_t detector_getHitChannel(void);

// Clear the detected hit once you have accounted for it.
void detector_clearHit(void);

// Implements the receive path: read ADC buffer, filter stages, hit detection.
// Get a count of available samples in the ADC receive buffer.
// Process the count of samples:
//   Get a sample from the ADC receive buffer.
//   Convert the sample from integer to floating point (-1.0 to +1.0).
//   Provide a new input to the filter stages (call filter_addSample()).
//   If filter_addSample() returns true, meaning decimation occurred, then:
//     Get a copy of the energy values from each frequency channel.
//     Do hit detection based on these energy values.
// Assumptions:
//   1) Reading from the ADC buffer is protected with a critical section.
//   2) Draining the ADC buffer occurs faster than it can fill.
void detector_run(void);

#endif // DETECTOR_H_
