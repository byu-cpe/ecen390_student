
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_check.h"

#include "neo.h"
#include "led_strip_encoder.h"

static const char *TAG = "neo";

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define NEO_RESOLUTION_HZ 10000000

rmt_channel_handle_t led_chan;
rmt_encoder_handle_t led_encoder;
rmt_transmit_config_t tx_config; // .loop_count = 0, no looping


// Initialize the NeoPixel driver.
// gpio_num: GPIO pin number.
// Return zero if successful, or non-zero otherwise.
int32_t neo_init(int32_t gpio_num)
{
	rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
		.gpio_num = gpio_num,
		.mem_block_symbols = 64, // increasing the block size can reduce flickering
		.resolution_hz = NEO_RESOLUTION_HZ,
		.trans_queue_depth = 4, // number of pending transactions in the background
	};
	ESP_LOGI(TAG, "Create RMT TX channel");
	ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

	led_strip_encoder_config_t encoder_config = {
		.resolution = NEO_RESOLUTION_HZ,
	};
	ESP_LOGI(TAG, "Install led strip encoder");
	ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

	ESP_LOGI(TAG, "Enable RMT TX channel");
	ESP_ERROR_CHECK(rmt_enable(led_chan));

	return 0;
}

// Free resources used by the NeoPixel driver.
// Return zero if successful, or non-zero otherwise.
int32_t neo_deinit(void)
{
	// TODO: implement neo_deinit(void);
	return 0;
}

// Write the pixel data to the LED strip.
// pixels: array of pixel data. Each pixel is represented by 3 bytes (GRB).
// size: the size of the array in bytes.
// wait: if true, block until done writing, otherwise return straight away.
void neo_write(const uint8_t *pixels, uint32_t size, bool wait)
{
	ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, pixels, size, &tx_config));
	if (wait) ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, -1));
}
