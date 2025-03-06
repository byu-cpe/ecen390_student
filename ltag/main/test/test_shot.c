#include <stdio.h>
#include <stdbool.h>

#include "esp_timer.h" // esp_timer_get_time
#include "esp_log.h" // LOG_COLOR_*

#include "config.h" // CONFIG_*
#include "shot.h" // shot_*

#define EP3(x) ((x)*1000)

#define MAX_SHOTS 5
#define RELOAD_TIME_MS 800
#define WAIT_RELOAD (RELOAD_TIME_MS+10)

static uint32_t reload_cnt;

static void reload(void)
{
	reload_cnt++;
}


void test_shot(void)
{
	bool err = false;
	uint32_t scnt;
	int64_t t_end;

	printf("******** test_shot() ********\n");
	// Shot initialization
	printf("shot_init() test\n");
	if (shot_init(MAX_SHOTS, RELOAD_TIME_MS)) {
		printf(" -- error: shot init\n");
		err = true;
		goto tshot_end;
	}
	scnt = shot_remaining();
	if (scnt != MAX_SHOTS) {
		printf(" -- error: shots after init:%lu, shots expected:%d\n",
			scnt, MAX_SHOTS);
		err = true;
		goto tshot_end;
	}

	// Verify shots are decremented
	printf("shot_decrement() test\n");
	for (uint16_t i = 0; i < MAX_SHOTS; i++) {
		shot_decrement();
		scnt = shot_remaining();
		if (scnt != MAX_SHOTS-1-i) {
			printf(" -- error: shots after decrement:%lu, shots expected:%d\n",
				scnt, MAX_SHOTS-1-i);
			err = true;
			break;
		}
	}
	// With zero shots, verify shots remain at zero after decrement
	shot_decrement();
	scnt = shot_remaining();
	if (scnt != 0) {
		printf(" -- error: shots after decrement with none:%lu, shots expected:%d\n",
			scnt, 0);
		err = true;
	}

	// Verify shot timer starts and stops
	printf("shot_timer_start() _stop() test\n");
	if (shot_timer_running()) {
		printf(" -- error: shot timer running\n");
		err = true;
		goto tshot_end;
	}
	shot_timer_start();
	if (!shot_timer_running()) {
		printf(" -- error: shot timer not running\n");
		err = true;
		goto tshot_end;
	}
	shot_timer_stop();
	if (shot_timer_running()) {
		printf(" -- error: shot timer running\n");
		err = true;
		goto tshot_end;
	}
	scnt = shot_remaining();
	if (scnt != 0) {
		printf(" -- error: shots after canceled timer:%lu, shots expected:%d\n",
			scnt, 0);
		err = true;
	}

	// After expired timer, verify reload
	printf("shot_register_reload() and timer test\n");
	reload_cnt = 0;
	shot_register_reload(reload);
	shot_timer_start();
	t_end = esp_timer_get_time() + EP3(WAIT_RELOAD);
	while (esp_timer_get_time() < t_end);
	shot_timer_stop();
	if (reload_cnt != 1) {
		printf(" -- error: reload callback not called\n");
		err = true;
	}
	scnt = shot_remaining();
	if (scnt != MAX_SHOTS) {
		printf(" -- error: shots after reload:%lu, shots expected:%d\n",
			scnt, MAX_SHOTS);
		err = true;
	}

	// After canceled timer, verify no reload
	printf("shot canceled timer test\n");
	shot_decrement();
	reload_cnt = 0;
	shot_timer_start();
	t_end = esp_timer_get_time() + EP3(WAIT_RELOAD/2);
	while (esp_timer_get_time() < t_end);
	shot_timer_stop();
	if (reload_cnt > 0) {
		printf(" -- error: reload callback was called\n");
		err = true;
	}
	scnt = shot_remaining();
	if (scnt != MAX_SHOTS-1) {
		printf(" -- error: shots after canceled timer:%lu, shots expected:%d\n",
			scnt, MAX_SHOTS-1);
		err = true;
	}

tshot_end:
	printf("******** test_shot() %s ********\n\n",
		err ? LOG_COLOR_E "Error" LOG_RESET_COLOR : "Done");
}
