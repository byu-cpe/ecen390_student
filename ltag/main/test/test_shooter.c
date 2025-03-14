#include <stdbool.h>
#include <string.h> // memset

#include "esp_timer.h" // esp_timer_get_time, for alternate lockout timer
#include "driver/gpio.h"

#include "config.h"
#include "hw.h"
#include "panel.h"
#include "histogram.h"
#include "detector.h"
#include "filter.h"
#include "hitLedTimer.h"
#include "shot.h"
#include "trigger.h"
#include "tx.h"
#include "rx.h"

// If defined, enables alternate lockout timer.
// Remove when lockoutTimer is implemented.
#define ALT_LOCKOUT 1

#define EP3(x) ((x)*1000)

#define DEFAULT_CH 6
#define DEFAULT_THRESH 64

#define MAX_THRESH_FAC (1<<16)

static const uint16_t play_freq[FILTER_CHANNELS] = CONFIG_PLAY_FREQ;
static uint8_t pixels_hit[] = {255,  0,  0}; // red
static uint16_t hit_count[FILTER_CHANNELS];
static bool hit_up; // Hit histogram update flag

//---------------------------------------------------------------------------//
// Display Panel Code:
// Each panel consists of two rows of characters, the first for the heading,
// and the second for the value. With a font size of 2, the laser tag unit
// display can accommodate 20 characters per row. The panel location and
// block dimensions are specified by x, y, and w. The height is always 2.

/*
   00000000001111111111
   01234567890123456789
  |--------------------
0 |  TX CH       THRESH
1 |xxxx  c        ttttt
2 |        HITS   SHOTS
3 |          hh      ss
*/

enum {TX_SHOT, TX_ON};
static char *tx_mode[] = {"shot", "on"};

typedef enum {TX, CH, /*VOL,*/ THRESH} ctl_e; // Control panel names
static ctl_e c_sel; // Control select
static panel_t ctl[] = { // Control variables
	{
		.head = "TX",
		.x = 0, .y = 0, .w = 4,
		.t = P_MAP,
		.u.m = {.x = TX_SHOT, .v = tx_mode},
	},
	{
		.head = "CH",
		.x = 5, .y = 0, .w = 2,
		.t = P_INT,
		.u.i = DEFAULT_CH,
	},
	// {
		// .head = "VOL",
		// .x = 8, .y = 0, .w = 3,
		// .t = P_INT,
		// .u.i = DEFAULT_VOL,
	// },
	{
		.head = "THRESH",
		.x = 14, .y = 0, .w = 6,
		.t = P_INT,
		.u.i = DEFAULT_THRESH,
	},
};

// TODO: add battery status
typedef enum {/*LIVES,*/ HITS, SHOTS} sts_e; // Status panel names
static panel_t sts[] = { // Status variables
	// {
		// .head = "LIVES",
		// .x = 0, .y = 2, .w = 5,
		// .t = P_INT,
		// .u.i = 0,
	// },
	{
		.head = "HITS",
		.x = 8, .y = 2, .w = 4,
		.t = P_INT,
		.u.i = 0,
	},
	{
		.head = "SHOTS",
		.x = 15, .y = 2, .w = 5,
		.t = P_INT,
		.u.i = 0,
	},
};
//---------------------------------------------------------------------------//

// Trigger pressed callback.
static void trig_pressed(void)
{
	if (shot_remaining() && ctl[TX].u.m.x == TX_SHOT) {
		tx_pulse(CONFIG_TX_PULSE);
		shot_decrement();
		sts[SHOTS].u.i = shot_remaining(); // Update status panel
		sts[SHOTS].up = true;
	}
	shot_timer_start(); // Initiate manual reload
}

// Trigger released callback.
static void trig_released(void)
{
	shot_timer_stop(); // Stop manual reload
}

// Shot reload callback.
static void reload(void)
{
	sts[SHOTS].u.i = shot_remaining(); // Update status panel
	sts[SHOTS].up = true;
	// Also reset hit count
	sts[HITS].u.i = 0; // Update status panel
	sts[HITS].up = true;
	memset(hit_count, 0, sizeof(hit_count));
	hit_up = true; // Can't draw to LCD from timer callback.
	// histogram_plotInteger() will be called from test_shooter().
}

// Disable hit detection on the specified channel.
static void disable_channel(uint16_t ch)
{
	bool chan[FILTER_CHANNELS];
	for (uint16_t i = 0; i < FILTER_CHANNELS; i++) chan[i] = i != ch;
	detector_setChannels(chan);
}

// Initialize the application state.
// period: Specify the period in milliseconds between calls to _tick().
// Return zero if successful, or non-zero otherwise.
int32_t test_shooter_init(uint32_t period)
{
	// Configuration... keep init functions in main()
	detector_setThreshFactor(ctl[THRESH].u.i);
	// Disable hit detection on current transmit frequency
	disable_channel(ctl[CH].u.i);
	tx_set_freq(play_freq[ctl[CH].u.i]);
	trigger_register_pressed(trig_pressed);
	trigger_register_released(trig_released);
	shot_register_reload(reload);
	hitLedTimer_setHitColor(pixels_hit);
	ctl[c_sel].sel = true; // highlight first panel
	sts[SHOTS].u.i = shot_remaining();
	panel_init(ctl, sizeof(ctl)/sizeof(panel_t));
	panel_init(sts, sizeof(sts)/sizeof(panel_t));
	panel_update(ctl, sizeof(ctl)/sizeof(panel_t));
	panel_update(sts, sizeof(sts)/sizeof(panel_t));
	return 0;
}

// Standard tick function.
// This function is safe to call from an ISR context.
void test_shooter_tick(void)
{
	// Read buttons
	static bool pressed = false;
	uint64_t btns = 0;
	btns |= !gpio_get_level(HW_NAV_LT) << HW_NAV_LT;
	btns |= !gpio_get_level(HW_NAV_RT) << HW_NAV_RT;
	btns |= !gpio_get_level(HW_NAV_DN) << HW_NAV_DN;
	btns |= !gpio_get_level(HW_NAV_UP) << HW_NAV_UP;
	if (!pressed && btns) {
		pressed = true;
		// NAV Left and Right buttons select the control to change
		if (btns & (1ULL << HW_NAV_LT)) {
			if (c_sel > TX) {
				ctl[c_sel].sel = false; ctl[c_sel--].up = true;
				ctl[c_sel].sel = true;  ctl[c_sel].up = true;
			}
		} else if (btns & (1ULL << HW_NAV_RT)) {
			if (c_sel < THRESH) {
				ctl[c_sel].sel = false; ctl[c_sel++].up = true;
				ctl[c_sel].sel = true;  ctl[c_sel].up = true;
			}
		}
		// NAV Down button changes control to next lower option
		if (btns & (1ULL << HW_NAV_DN)) {
			// Handle the currently selected control
			switch (c_sel) {
			case TX:
				if (ctl[TX].u.m.x != TX_SHOT) {
					ctl[TX].u.m.x = TX_SHOT;
					ctl[TX].up = true;
					tx_emit(false);
				}
				break;
			case CH:
				if (ctl[CH].u.i > 0) {
					ctl[CH].u.i--;
					ctl[CH].up = true;
					tx_set_freq(play_freq[ctl[CH].u.i]);
					disable_channel(ctl[CH].u.i);
				}
				break;
			// case VOL:
				// break;
			case THRESH:
				if (ctl[THRESH].u.i > 1) {
					ctl[THRESH].u.i >>= 1;
					ctl[THRESH].up = true;
					detector_setThreshFactor(ctl[THRESH].u.i);
				}
				break;
			}
		// NAV Up button changes control to next higher option
		} else if (btns & (1ULL << HW_NAV_UP)) {
			// Handle the currently selected control
			switch (c_sel) {
			case TX:
				if (ctl[TX].u.m.x != TX_ON) {
					ctl[TX].u.m.x = TX_ON;
					ctl[TX].up = true;
					tx_emit(true);
				}
				break;
			case CH:
				if (ctl[CH].u.i < FILTER_CHANNELS-1) {
					ctl[CH].u.i++;
					ctl[CH].up = true;
					tx_set_freq(play_freq[ctl[CH].u.i]);
					disable_channel(ctl[CH].u.i);
				}
				break;
			// case VOL:
				// break;
			case THRESH:
				if (ctl[THRESH].u.i < MAX_THRESH_FAC) {
					ctl[THRESH].u.i <<= 1;
					ctl[THRESH].up = true;
					detector_setThreshFactor(ctl[THRESH].u.i);
				}
				break;
			}
		}
	} else if (pressed && !btns) {
		pressed = false;
	}
}

// Run tests for shooter mode.
// - Transmit pulse on the current channel when the trigger is pressed.
// - The control option is selected with NAV_LT/RT.
// - The transmit (TX) option (on or shot) is selected with NAV_UP/DN.
// - The channel (CH) is changeable with NAV_UP/DN.
// - The threshold factor (THRESH) is selectable with NAV_UP/DN.
// - 40 shots are initially available. Hits are counted and displayed.
// - Shots and hits can be reset by holding the trigger for 3 sec.
// - Run the receive pipeline (with detector) and display total hits
//   for each channel.
// - This test program can be used to determine the default threshold
//   factor when tag units are separated by 40 ft.
void test_shooter(void)
{
	#ifdef ALT_LOCKOUT
		int64_t tlock = esp_timer_get_time() + EP3(CONFIG_LOCKOUT_PERIOD);
		detector_ignoreAllHits(true);
	#else
		lockoutTimer_start(); // Ignore erroneous hits at startup
	#endif

	trigger_operation(true); // Enable trigger to fire shots
	for (;;) {
		// Run filters, compute energy, run hit-detection
		detector_run();
		if (detector_getHit()) { // Hit detected
			hitLedTimer_start();
			sts[HITS].u.i++;     // Increment the hit count
			sts[HITS].up = true;
			hit_count[detector_getHitChannel()]++;
			histogram_plotInteger(hit_count, FILTER_CHANNELS);
			#ifdef ALT_LOCKOUT
				detector_ignoreAllHits(true);
				tlock = esp_timer_get_time() + EP3(CONFIG_LOCKOUT_PERIOD);
			#else
				lockoutTimer_start(); // Wait for pulse to terminate
			#endif
			detector_clearHit(); // Clear the hit
		}
		#ifdef ALT_LOCKOUT
			else if (tlock > 0 && esp_timer_get_time() > tlock) {
				detector_ignoreAllHits(false);
				tlock = -1;
			}
		#endif
		panel_update(ctl, sizeof(ctl)/sizeof(panel_t));
		panel_update(sts, sizeof(sts)/sizeof(panel_t));
		if (hit_up) { // Set by shot reload callback
			histogram_plotInteger(hit_count, FILTER_CHANNELS);
			hit_up = false;
		}
	}
}
