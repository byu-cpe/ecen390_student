#include <stdio.h> // printf

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // gpio_*
#include "esp_log.h" // ESP_LOG*

#include "config.h" // HW_LTAG_TX
#include "lcd.h" // lcd_init
#include "test_delay.h"
#include "test_filter.h"

static const char *TAG = "m3t1";

// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "m3t1 main");

	// Initialization
	// Make sure LTAG TX is low to start
	gpio_set_pull_mode(HW_LTAG_TX, GPIO_PULLDOWN_ONLY);
	lcd_init();

	test_delay();
	test_filter();
	return;
}
