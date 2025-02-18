#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h" // CONFIG_*
#include "hw.h" // HW_LTAG_*
#include "hitLedTimer.h"

#define TPERIOD 1000 // timer period in ms
#define MAX_BLINK 5

static TimerHandle_t update_timer; // Declare timer handle for update callback
static bool err = false;

static const uint8_t pixels_def[] = {  1,  1,  1}; // dim white
static const uint8_t pixels_hit[] = {255,  0,  0}; // red

static volatile uint32_t update_cnt;


// Called when timer alarms
static void update(TimerHandle_t pt)
{
	if (hitLedTimer_running()) {
		printf(" -- error: hit LEC timer running\n");
		err = true;
	}
	hitLedTimer_start();
	taskYIELD();
	if (!hitLedTimer_running()) {
		printf(" -- error: hit LEC timer not running\n");
		err = true;
	}
	update_cnt++;
}

// Run tests for the hit LED timer module.
void test_hitLedTimer(void)
{
	printf("******** test_hitLedTimer() ********\n");
	if (hitLedTimer_init(CONFIG_HITLED_PERIOD, HW_LTAG_LED)) {
		printf(" -- error: hit LEC timer init\n");
		err = true;
		goto thlt_end;
	}
	hitLedTimer_setColor(pixels_def);
	hitLedTimer_setHitColor(pixels_hit);

	// Initialize update timer
	update_timer = xTimerCreate(
		"update_timer",         // Text name for the timer.
		pdMS_TO_TICKS(TPERIOD), // The timer period in ticks.
		pdTRUE,                 // Auto-reload the timer when it expires.
		NULL,                   // No need for a timer ID.
		update                  // Function called when timer expires.
	);
	if (update_timer == NULL) {
		printf(" -- error: creating update timer\n");
		err = true;
		goto thlt_end;
	}
	if (xTimerStart(update_timer,
		pdMS_TO_TICKS(CONFIG_TIME_OUT_PERIOD)) != pdPASS) {
		printf(" -- error: starting update timer\n");
		err = true;
		goto thlt_end;
	}

	while (update_cnt < MAX_BLINK);

	if (xTimerDelete(update_timer, CONFIG_TIME_OUT_PERIOD) != pdPASS) {
		printf(" -- error: deleting update timer\n");
		err = true;
		goto thlt_end;
	}

thlt_end:
	printf("******** test_hitLedTimer() %s ********\n\n", err ? "Error" : "Done");
}
