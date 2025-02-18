#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "hw.h"
#include "lcd.h"
#include "histogram.h"
#include "filter.h"
#include "detector.h"
#include "hitLedTimer.h"
#include "invincibilityTimer.h"
#include "lockoutTimer.h"
#include "shot.h"
#include "trigger.h"
#include "tx.h"
#include "rx.h"
#include "sound.h"
#include "game.h"
#include "config.h"

#define TPERIOD CONFIG_MAIN_TICK_PERIOD // timer period in ms

static const char *TAG = "ltag";
static TimerHandle_t update_timer; // Declare timer handle for update callback


static void update(TimerHandle_t pt)
{
	trigger_tick();
	game_tick();
}

// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "app_main");

	// Basic initialization
	// Make sure LTAG TX is low to start
	gpio_set_pull_mode(HW_LTAG_TX, GPIO_PULLDOWN_ONLY);
	lcd_init();
	histogram_init(FILTER_CHANNELS); // Clears screen

	// GPIO initialization for navigation buttons
	// gpio_set_pull_mode(<gpio>, GPIO_PULLUP_ONLY); set by default
	gpio_reset_pin(HW_NAV_LT);
	gpio_set_direction(HW_NAV_LT, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_RT);
	gpio_set_direction(HW_NAV_RT, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_DN);
	gpio_set_direction(HW_NAV_DN, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_UP);
	gpio_set_direction(HW_NAV_UP, GPIO_MODE_INPUT);

	// Application specific initialization
	hitLedTimer_init(CONFIG_HITLED_PERIOD, HW_LTAG_LED);
	invincibilityTimer_init(CONFIG_INVINCIBILITY_PERIOD);
	lockoutTimer_init(CONFIG_LOCKOUT_PERIOD);
	shot_init(CONFIG_SHOT_COUNT, CONFIG_SHOT_RELOAD_PERIOD);
	filter_init();
	detector_init();
	trigger_init(TPERIOD, HW_LTAG_TRIGGER);
	tx_init(HW_LTAG_TX);
	rx_init(HW_LTAG_RX, CONFIG_RX_SAMPLE_RATE);
	sound_init(CONFIG_SND_SAMPLE_RATE);
	game_init(TPERIOD); // last

	// Initialize update timer
	update_timer = xTimerCreate(
		"update_timer",         // Text name for the timer.
		pdMS_TO_TICKS(TPERIOD), // The timer period in ticks.
		pdTRUE,                 // Auto-reload the timer when it expires.
		NULL,                   // No need for a timer ID.
		update                  // Function called when timer expires.
	);
	if (update_timer == NULL) {
		ESP_LOGE(TAG, "Error creating update timer");
		return;
	}
	if (xTimerStart(update_timer,
		pdMS_TO_TICKS(CONFIG_TIME_OUT_PERIOD)) != pdPASS) {
		ESP_LOGE(TAG, "Error starting update timer");
		return;
	}

	game_twoTeamTag();

	// Turn off and release resources
	tx_emit(false);
	sound_deinit();
	rx_deinit();
}
