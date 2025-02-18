#include <stdio.h>
#include <math.h> // fabsf

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // gpio_*
#include "esp_timer.h"

#include "config.h" // CONFIG_*
#include "hw.h" // HW_LTAG_*
#include "lcd.h" // lcd_*
#include "filter.h" // FILTER_CHANNELS
#include "rx.h"
#include "tx.h"

#define GPIO_LOOPBACK 2
#define MID 2048
#define TARGET_DUTY 50.0f

#define PERCENT(f) ((f)*100)
#define EP3(x) ((x)*1000)
#define EN3(x) ((x)/1000)

#define T_EMIT_MS 100
#define T_PULSE_MS CONFIG_TX_PULSE
#define T_MARGIN_MS 2

#define ERROR_PULSE 10.0f // ms
#define ERROR_DUTY 0.5f   // percent
#define ERROR_FREQ 25.0f  // Hz

static const uint16_t play_freq[FILTER_CHANNELS] = CONFIG_PLAY_FREQ;
static rx_data_t rx_buf[CONFIG_RX_SAMPLE_RATE/10*3];


bool check_samples(uint16_t freq_hz, float pulse_ms, rx_data_t *buf, uint32_t len)
{
	uint32_t i, high, edges;

	// Check length of receive buffer
	uint32_t expect_len = EN3(CONFIG_RX_SAMPLE_RATE*pulse_ms);
	if (len < expect_len) {
		printf(" -- error: samples:%lu, expecting:%lu+\n", len, expect_len);
		return true;
	}

	// Signal should start and end low
	if (buf[0] >= MID) {
		printf(" -- error: signal did not start low\n");
		return true;
	}
	if (buf[len-1] >= MID) {
		printf(" -- error: signal did not end low\n");
		return true;
	}

	// Find first and last positive edges
	uint32_t first, last;
	for (i = 0; i < len && buf[i] >= MID; i++); // find low
	for (; i < len && buf[i] < MID; i++); // find high
	first = i;
	for (i = len-1; i >= first && buf[i] < MID; i--); // find high
	for (; i >= first && buf[i] >= MID; i--); // find low
	last = i+1;
	if (first == len) {
		printf(" -- error: no signal\n");
		return true;
	}

#if 0
	printf("fidx:%lu lidx:%lu diff:%lu len:%lu\n", first, last, last-first, len);
	if (freq_hz == 4000) {
		for (uint32_t j = 0; j < first+(CONFIG_RX_SAMPLE_RATE/freq_hz)+2; j++) printf(" %hu", buf[j]);
		putchar('\n');
		for (uint32_t j = last-(CONFIG_RX_SAMPLE_RATE/freq_hz)-2; j < len; j++) printf(" %hu", buf[j]);
		putchar('\n');
	}
#endif

	// Calculate pulse time
	float actual_pulse_ms = EP3((float)(last-first)/CONFIG_RX_SAMPLE_RATE);
	actual_pulse_ms += EP3(1.0f/freq_hz); // add last cycle
	if (fabsf(actual_pulse_ms-pulse_ms) > ERROR_PULSE) {
		printf(" -- error: pulse time:%.1f ms, expecting:%.1f ms\n", actual_pulse_ms, pulse_ms);
		return true;
	}

	// Calculate duty cycle
	for (i = first, high = 0; i < last; i++)
		if (buf[i] >= MID) high++;
	float duty_cycle = PERCENT((float)high/(last-first));
	if (fabsf(duty_cycle-TARGET_DUTY) > ERROR_DUTY) {
		printf(" -- error: duty cycle:%.1f%%, expecting:%.1f%%\n", duty_cycle, TARGET_DUTY);
		return true;
	}

	// Calculate positive edges
	for (i = first, edges = 0; i < last; i++)
		if (buf[i-1] < MID && buf[i] >= MID) edges++;
	float actual_freq_hz = EP3(edges+1)/actual_pulse_ms; // add last cycle
	if (fabsf(actual_freq_hz-freq_hz) > ERROR_FREQ) {
		printf(" -- error: frequency:%.1f Hz, expecting:%hu Hz\n", actual_freq_hz, freq_hz);
		return true;
	}

	// printf("target:%hu Hz diff:%+5.1f Hz ", freq_hz, actual_freq_hz-freq_hz);
	printf("freq:%0.1f Hz, duty:%0.1f%%, pulse:%0.1f ms\n",
		actual_freq_hz, duty_cycle, actual_pulse_ms);

	return false;
}

// Run tests for the transmitter module
void test_tx(void)
{
	bool err = false;

	printf("******** test_tx() ********\n");
	lcd_setFontSize(2);
	lcd_setFontBackground(BLACK);

	// If rx_init precedes tx_init, internal loopback & AUTO_CLK has less frequency error
	// If rx_init precedes tx_init, internal loopback & RC_FAST_CLK shows zero samples on rx
	// If tx_init precedes rx_init, external loopback & RC_FAST_CLK has more frequency error
	// If tx_init precedes rx_init, internal loopback shows no signal on rx

	// gpio_set_pull_mode(GPIO_LOOPBACK, GPIO_PULLDOWN_ONLY);
	// gpio_set_direction(GPIO_LOOPBACK, GPIO_MODE_INPUT_OUTPUT);

	// Receiver initialization, must precede tx_init
	if (rx_init(GPIO_LOOPBACK, CONFIG_RX_SAMPLE_RATE)) {
	// if (rx_init(HW_LTAG_RX, CONFIG_RX_SAMPLE_RATE)) {
		printf(" -- error: receiver init\n");
		err = true;
		goto ttx_end;
	}

	// Transmitter initialization
	if (tx_init(GPIO_LOOPBACK)) {
	// if (tx_init(HW_LTAG_TX)) {
		printf(" -- error: transmitter init\n");
		err = true;
		goto ttx_end;
	}

	printf("tx_emit() test\n");
	for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
		int64_t t_end;
		uint32_t rx_idx = 0;
		uint32_t cnt;
		char s[14];
		sprintf(s, "chan %hu emit", i);
		lcd_drawString(0, 3*2*LCD_CHAR_H, s, WHITE);
		tx_set_freq(play_freq[i]);

		rx_clear_buffer();
		t_end = esp_timer_get_time() + EP3(T_MARGIN_MS);
		while (esp_timer_get_time() < t_end);

		tx_emit(true);
		t_end = esp_timer_get_time() + EP3(T_EMIT_MS);
		while (esp_timer_get_time() < t_end) {
			cnt = rx_get_count();
			while (cnt--) rx_buf[rx_idx++] = rx_get_sample();
		}
		tx_emit(false);

		t_end = esp_timer_get_time() + EP3(T_MARGIN_MS);
		while (esp_timer_get_time() < t_end);
		cnt = rx_get_count();
		while (cnt--) rx_buf[rx_idx++] = rx_get_sample();

		err |= check_samples(play_freq[i], T_EMIT_MS, rx_buf, rx_idx);
	}

	printf("tx_pulse() test\n");
	for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
		int64_t t_end;
		uint32_t rx_idx = 0;
		uint32_t cnt;
		char s[14];
		sprintf(s, "chan %hu pulse", i);
		lcd_drawString(0, 3*2*LCD_CHAR_H, s, WHITE);
		tx_set_freq(play_freq[i]);

		rx_clear_buffer();
		t_end = esp_timer_get_time() + EP3(T_MARGIN_MS);
		while (esp_timer_get_time() < t_end);

		tx_pulse(T_PULSE_MS);
		t_end = esp_timer_get_time() + EP3(T_PULSE_MS);
		while (esp_timer_get_time() < t_end) {
			cnt = rx_get_count();
			while (cnt--) rx_buf[rx_idx++] = rx_get_sample();
		}

		t_end = esp_timer_get_time() + EP3(T_MARGIN_MS);
		while (esp_timer_get_time() < t_end);
		cnt = rx_get_count();
		while (cnt--) rx_buf[rx_idx++] = rx_get_sample();

		err |= check_samples(play_freq[i], T_PULSE_MS, rx_buf, rx_idx);
	}

ttx_end:
	lcd_drawString(0, 4*2*LCD_CHAR_H, err?"fail":"pass", err?RED:GREEN);
	printf("******** test_tx() %s ********\n\n", err ? "Error" : "Done");
}

// setup internal loopback on gpio pin on esp32
// https://esp32.com/viewtopic.php?t=1369 - loopback question
// https://github.com/DavidAntliff/esp32-freqcount - frequency counter component
// https://github.com/DavidAntliff/esp32-freqcount-example - frequency counter example
// https://github.com/espressif/esp-idf/tree/master/examples/peripherals/gpio/generic_gpio

// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/pcnt.html
// https://github.com/espressif/esp-idf/tree/master/examples/peripherals/pcnt/rotary_encoder

// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/rmt.html
// https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip
// https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip_simple_encoder

// https://github.com/astrorafael/esp32_freq_gen
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gptimer.html
