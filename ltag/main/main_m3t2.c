#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // gpio_*
#include "esp_log.h" // ESP_LOG*

#include "hw.h" // HW_LTAG_*
#include "lcd.h" // lcd_init
#include "neo.h"
#include "test_trigger.h"
#include "test_tx.h"
#include "test_hitLedTimer.h"
#include "test_shot.h"

static const char *TAG = "m3t2";

// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "app_main");

	// Initialization
	// Make sure LTAG TX is low to start
	gpio_set_pull_mode(HW_LTAG_TX, GPIO_PULLDOWN_ONLY);
	lcd_init(); // Clears display
	neo_init(HW_LTAG_LED); // used by test_trigger

	test_trigger();
	test_tx();
	test_hitLedTimer();
	test_shot();

	return;
}
