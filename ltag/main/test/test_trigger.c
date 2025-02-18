#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h" // CONFIG_*
#include "hw.h" // HW_LTAG_*
#include "lcd.h" // lcd_*
#include "neo.h" // neo_write
#include "trigger.h"

#define TPERIOD CONFIG_MAIN_TICK_PERIOD // timer period in ms
#define TICKS_SEC (1000/TPERIOD) // update timer ticks per second
#define DISABLE_SEC 3 // trigger disable time period in seconds
#define MAX_PRESSES 10
#define PIXEL_CNT 4

static TimerHandle_t update_timer; // Declare timer handle for update callback
static const uint8_t pixels_whi[3*PIXEL_CNT] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8_t pixels_blk[3*PIXEL_CNT] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t pixels_red[3*PIXEL_CNT] = {0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00};

static volatile uint16_t trigger_press_cnt;
static volatile uint16_t trigger_release_cnt;
static volatile uint32_t update_cnt;

// Called when timer alarms
static void update(TimerHandle_t pt)
{
	trigger_tick();
	update_cnt++;
}

// Trigger pressed callback
static void trig_pressed(void)
{
	neo_write(pixels_whi, sizeof(pixels_whi), false);
	fputc('D', stderr);
	trigger_press_cnt++;
}

// Trigger released callback
static void trig_released(void)
{
	neo_write(pixels_blk, sizeof(pixels_blk), false);
	fputc('U', stderr);
	trigger_release_cnt++;
}

// Run tests for the trigger module.
void test_trigger(void)
{
	bool err = false;
	uint32_t t1; // Used for time
	uint16_t tpress, trelease;

	printf("******** test_trigger() ********\n");
	neo_write(pixels_blk, sizeof(pixels_blk), false);
	lcd_fillScreen(BLACK);
	lcd_setFontSize(2);
	lcd_setFontBackground(BLACK);
	if (trigger_init(TPERIOD, HW_LTAG_TRIGGER)) {
		printf(" -- error: trigger init\n");
		err = true;
		goto ttr_end;
	}
	trigger_register_pressed(trig_pressed);
	trigger_register_released(trig_released);
	trigger_operation(true); // Enable trigger.

	// Initialize update timer
	update_timer = xTimerCreate(
		"update_timer",         // Text name for the timer.
		pdMS_TO_TICKS(TPERIOD), // The timer period in ticks.
		pdTRUE,                 // Auto-reload the timer when it expires.
		NULL,                   // No need for a timer ID.
		update                  // Function called when timer expires.
	);
	if (update_timer == NULL) {
		printf(" -- error: creating update timer\n");
		err = true;
		goto ttr_end;
	}
	if (xTimerStart(update_timer,
		pdMS_TO_TICKS(CONFIG_TIME_OUT_PERIOD)) != pdPASS) {
		printf(" -- error: starting update timer\n");
		err = true;
		goto ttr_end;
	}
	lcd_drawString(0, 0, "Press trigger", WHITE);
	lcd_drawString(0, 1*2*LCD_CHAR_H, "10 times", WHITE);
	// Wait for trigger operation.
	while (trigger_release_cnt < MAX_PRESSES/2);

	printf("\ntrigger disable\n");
	trigger_operation(false);
	tpress = trigger_press_cnt;
	trelease = trigger_release_cnt;
	neo_write(pixels_red, sizeof(pixels_red), false);
	t1 = update_cnt + TICKS_SEC*DISABLE_SEC;
	while (update_cnt < t1); // Wait DISABLE_SEC.
	neo_write(pixels_blk, sizeof(pixels_blk), false);
	if (tpress != trigger_press_cnt || trelease != trigger_release_cnt) {
		printf(" -- error: trigger was not disabled\n");
		err = true;
	}

	printf("trigger enable\n");
	trigger_operation(true);
	// Wait for trigger operation.
	while (trigger_release_cnt < MAX_PRESSES);
	fputc('\n', stderr);

	if (trigger_press_cnt != trigger_release_cnt) {
		printf(" -- error: press count (%d) not equal to release count (%d)\n",
			trigger_press_cnt, trigger_release_cnt);
		err = true;
	}
	if (xTimerDelete(update_timer, CONFIG_TIME_OUT_PERIOD) != pdPASS) {
		printf(" -- error: deleting update timer\n");
		err = true;
		goto ttr_end;
	}

ttr_end:
	lcd_drawString(0, 2*2*LCD_CHAR_H, err?"fail":"pass", err?RED:GREEN);
	printf("******** test_trigger() %s ********\n\n", err ? "Error" : "Done");
}
