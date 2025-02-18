#include <stdbool.h>
#include <string.h> // memset

#include "driver/gpio.h"
#include "esp_timer.h"

#include "config.h" // CONFIG_*
#include "filter.h"
#include "histogram.h"
#include "hitLedTimer.h"
#include "hw.h" // HW_NAV_*, HW_LTAG_*
#include "lcd.h"
#include "rx.h"
#include "tx.h"

// #define DEBUG 1
#define ADC_HALF_SCALE 2048
#define DISPLAY_UPDATE_PERIOD 250000 // microseconds

static const uint16_t play_freq[FILTER_CHANNELS] = CONFIG_PLAY_FREQ;
static uint32_t freq_num;
static bool tx_on;


// Draw control panel
static void control(uint8_t tx, uint8_t ch)
{
	static bool     c_titles = false;
	static uint8_t  c_tx = -1;
	static uint8_t  c_ch = -1;
	coord_t x = 0, y = 0;

	lcd_setFontSize(2);
	lcd_setFontBackground(BLACK);
	if (!c_titles) {
		lcd_fillRect(x, y, LCD_W, LCD_CHAR_H*2*2, BLACK);
		lcd_drawString(x, y, "  TX CH", WHITE);
		c_titles = true;
	}
	y += LCD_CHAR_H*2;
	if (tx != c_tx) {
		lcd_drawString(x, y, (tx) ? "  on" : " off", GREEN);
		c_tx = tx;
	}
	if (ch != c_ch) {
		lcd_drawChar(x+LCD_CHAR_W*2*6, y, '0'+ch, GREEN);
		c_ch = ch;
	}
	lcd_noFontBackground();
}

// Run the DSP stages: FIR filter, IIR filters, energy calculation
static void dsp_run(void)
{
	uint32_t adc_cnt = rx_get_count(); // Save count of elements in ADC buffer

	while (adc_cnt) {
		rx_data_t rawAdcValue;
		filter_data_t scaledAdcValue;

		// Get next ADC value
		rawAdcValue = rx_get_sample();
		adc_cnt--;

		// Scale ADC value
		scaledAdcValue = (filter_data_t)rawAdcValue / ADC_HALF_SCALE - (filter_data_t)1.0;
		filter_addSample(scaledAdcValue); // Process scaled ADC value
	}
}

// Run tests for the continuous module.
// - Transmit on the current channel while the trigger pressed.
// - The channel is changeable with NAV_UP/DN.
// - Run the DSP pipeline and display energy for each channel.
// - End when NAV_LT is pressed.
// Assumptions.
// - Transmit signal is looped back through receiver
//   (optically or electrically).
void test_continuous(void)
{
	int64_t tnext;

	tx_set_freq(play_freq[freq_num]);
	control(tx_on, freq_num);
	tnext = esp_timer_get_time() + DISPLAY_UPDATE_PERIOD;
	while (gpio_get_level(HW_NAV_LT)) {
		static bool pressed = false;
		uint64_t btns = 0;
		btns |= !gpio_get_level(HW_LTAG_TRIGGER) << HW_LTAG_TRIGGER;
		btns |= !gpio_get_level(HW_NAV_RT) << HW_NAV_RT;
		btns |= !gpio_get_level(HW_NAV_DN) << HW_NAV_DN;
		btns |= !gpio_get_level(HW_NAV_UP) << HW_NAV_UP;
		if (!pressed && btns) {
			pressed = true;
			#ifdef DEBUG
			for (uint8_t i = 0; i < sizeof(btns)*8; i++) {
				if (btns & (1ULL << i)) printf(" %d", i);
			}
			putchar('\n');
			#endif // DEBUG
			if (btns & (1ULL << HW_LTAG_TRIGGER)) {
				tx_emit(true); tx_on = true;
			}
			if (btns & (1ULL << HW_NAV_RT)) {
				hitLedTimer_start();
			}
			if (btns & (1ULL << HW_NAV_DN)) {
				if (freq_num > 0) freq_num--;
				tx_set_freq(play_freq[freq_num]);
			} else if (btns & (1ULL << HW_NAV_UP)) {
				if (freq_num < 9) freq_num++;
				tx_set_freq(play_freq[freq_num]);
			}
		} else if (pressed && !btns) {
			if (tx_on) {tx_on = false; tx_emit(false);}
			pressed = false;
		}
		dsp_run();
		control(tx_on, freq_num);
		if (esp_timer_get_time() >= tnext) {
			#ifdef DEBUG
			putchar('t');
			putchar('\n');
			#endif // DEBUG
			tnext += DISPLAY_UPDATE_PERIOD;
			// Graph energy values on LCD
			filter_data_t energyValues[FILTER_CHANNELS];
			filter_getEnergyArray(energyValues);
			filter_data_t max = energyValues[0];
			for (uint16_t i = 1; i < FILTER_CHANNELS; i++) {
				if (energyValues[i] > max) max = energyValues[i];
			}
			// Only plot if energy is 1 or above
			if (max < 1.0f) memset(energyValues, 0, sizeof(energyValues));
			histogram_plotFloat(energyValues, FILTER_CHANNELS);
		}
	}
	tx_emit(false);
}
