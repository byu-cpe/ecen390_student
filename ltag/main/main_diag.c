#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // gpio_*
#include "esp_log.h" // ESP_LOG*

#include "config.h" // CONFIG_*
#include "filter.h"
#include "histogram.h"
#include "hitLedTimer.h"
#include "hw.h" // HW_NAV_*, HW_LTAG_*
#include "lcd.h"
#include "rx.h"
#include "tx.h"
#include "sound.h"
#include "gameBoyStartup.h"
#include "test_continuous.h"

static const char *TAG = "diag";

// Main application
void app_main(void)
{
	static const uint8_t pixels_def[] = {  1,  1,  1}; // dim white
	static const uint8_t pixels_hit[] = {255,  0,  0}; // red
	ESP_LOGI(TAG, "app_main");
	// Initialization
	// Make sure LTAG TX is low to start
	gpio_set_pull_mode(HW_LTAG_TX, GPIO_PULLDOWN_ONLY);
	lcd_init(); // Clears display
	histogram_init(FILTER_CHANNELS); // Clears portion of display
	gpio_reset_pin(HW_LTAG_TRIGGER); // TODO: use trigger.c instead?
	gpio_set_direction(HW_LTAG_TRIGGER, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_LT);
	gpio_set_direction(HW_NAV_LT, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_RT);
	gpio_set_direction(HW_NAV_RT, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_DN);
	gpio_set_direction(HW_NAV_DN, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_NAV_UP);
	gpio_set_direction(HW_NAV_UP, GPIO_MODE_INPUT);
	filter_init();
	rx_init(HW_LTAG_RX, CONFIG_RX_SAMPLE_RATE);
	tx_init(HW_LTAG_TX);
	hitLedTimer_init(CONFIG_HITLED_PERIOD, HW_LTAG_LED);
	hitLedTimer_setColor(pixels_def);
	hitLedTimer_setHitColor(pixels_hit);
	sound_init(CONFIG_SND_SAMPLE_RATE);

	test_continuous();

	sound_deinit();
	tx_emit(false);
	rx_deinit();
	return;
}
