// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/rmt.html
// https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip
// https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip_simple_encoder

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_check.h"

#include "neo.h"
#include "led_strip_encoder.h"

static const char *TAG = "neo";

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define NEO_RESOLUTION_HZ 10000000

static rmt_channel_handle_t led_chan;
static rmt_encoder_handle_t led_encoder;
static rmt_transmit_config_t tx_config; // .loop_count = 0, no looping
static bool rmt_live;


// Initialize the NeoPixel driver.
// gpio_num: GPIO pin number.
// Return zero if successful, or non-zero otherwise.
int32_t neo_init(int32_t gpio_num)
{
	if (rmt_live) return 0;
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

	rmt_live = true;
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
