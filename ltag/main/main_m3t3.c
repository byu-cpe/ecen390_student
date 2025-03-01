#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // gpio_*
#include "esp_log.h" // ESP_LOG*

#include "hw.h" // HW_LTAG_*
#include "lcd.h"
#include "test_buffer.h"
#include "test_detector.h"
#include "test_shot.h"

static const char *TAG = "m3t3";

// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "app_main");

	// Initialization
	// Make sure LTAG TX is low to start
	gpio_set_pull_mode(HW_LTAG_TX, GPIO_PULLDOWN_ONLY);
	lcd_init();
	lcd_fillScreen(BLACK);

	test_buffer();
	test_detector();
	test_shot();

	return;
}
