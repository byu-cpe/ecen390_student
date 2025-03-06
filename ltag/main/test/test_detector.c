#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_timer.h" // esp_timer_get_time
#include "esp_log.h" // LOG_COLOR_*

#include "config.h" // CONFIG_*
#include "filter.h" // FILTER_CHANNELS, filter_data_t, filter_init
#include "detector.h" // detector_*
#include "rx.h"
#include "tx.h"

#define GPIO_LOOPBACK 2
#define FILTER_THRESH 2048.0

#define EP3(x) ((x)*1000)
#define EN3(x) ((x)/1000)


// Function to calculate the slope (m) and y-intercept (b)
static void linear_regression(const float x[], const float y[], int n, float *slope, float *intercept) {
	float sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;

	// Calculate necessary sums
	for (int i = 0; i < n; i++) {
		sumX += x[i];
		sumY += y[i];
		sumXY += x[i] * y[i];
		sumX2 += x[i] * x[i];
	}
	// Calculate slope (m)
	*slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
	// Calculate y-intercept (b)
	*intercept = (sumY - (*slope) * sumX) / n;
}

void test_detector(void)
{
	bool err = false;
	bool hit;
	uint16_t hit_ch;
	bool en_chan[] = {true, true, false, true, true, true, true, true, true, true};
	filter_data_t energy[] = {16.0, 11.0, 71.0, 12.0, 14.0, 57.0, 59.0, 10.0, 58.0, 13.0};
	//                        10    11    12    13    14    16    57    58    59    71
	//                                                 ^ median * 4 = 56, * 5 = 70

	printf("******** test_detector() ********\n");
	filter_init();
	detector_init();

	// Verify hit when highest energy channel is ignored
	// Will catch hit detection scanning from low to high energy
	printf("detector_checkHit() phase 1 test\n");
	detector_setChannels(en_chan);
	detector_setThreshFactor(4.0f);
	detector_checkHit(energy);
	hit = detector_getHit();
	hit_ch = detector_getHitChannel();
	// Check for hit on chan 6
	if (!hit) {
		printf(" -- error: hit expected\n");
		err = true;
	} else if (hit_ch != 6) {
		printf(" -- error: hit on chan:%hu, expecting:6\n", hit_ch);
		err = true;
	}

	// Verify previous hit remains when not cleared
	printf("detector_checkHit() phase 2 test\n");
	en_chan[2] = true;
	detector_setChannels(en_chan);
	detector_checkHit(energy);
	hit = detector_getHit();
	hit_ch = detector_getHitChannel();
	// Ignore hit on chan 2 since previous hit on chan 6 not cleared
	if (!hit) {
		printf(" -- error: hit expected\n");
		err = true;
	} else if (hit_ch != 6) {
		printf(" -- error: hit on chan:%hu, expecting:6 when hit not cleared\n", hit_ch);
		err = true;
	}

	// Verify channel 2 was enabled and old hit cleared
	// Verify highest energy channel detected when several are above threshold
	printf("detector_checkHit() phase 3 test\n");
	detector_clearHit(); 
	detector_checkHit(energy);
	hit = detector_getHit();
	hit_ch = detector_getHitChannel();
	// Check for hit on chan 2
	if (!hit) {
		printf(" -- error: hit expected\n");
		err = true;
	} else if (hit_ch != 2) {
		printf(" -- error: hit on chan:%hu, expecting:2\n", hit_ch);
		err = true;
	}

	// Verify no hits occur when ignore all hits is true
	printf("detector_checkHit() phase 4 test\n");
	detector_clearHit();
	// Verify hit was cleared
	if (detector_getHit()) {
		printf(" -- error: hit not cleared\n");
		err = true;
	}
	detector_ignoreAllHits(true);
	detector_checkHit(energy);
	hit = detector_getHit();
	// Check for no hits
	if (hit) {
		printf(" -- error: got hit on chan:%hu when ignoring all hits\n",
			detector_getHitChannel());
		err = true;
	}

	// Verify no hits occur with elevated threshold
	printf("detector_checkHit() phase 5 test\n");
	detector_clearHit();
	en_chan[2] = false;
	detector_setChannels(en_chan);
	detector_setThreshFactor(5.0f);
	detector_checkHit(energy);
	hit = detector_getHit();
	// Check for no hits
	if (hit) {
		printf(" -- error: got hit with elevated threshold on chan:%hu\n",
			detector_getHitChannel());
		err = true;
	}

	// Receiver initialization, must precede tx_init
	if (rx_init(GPIO_LOOPBACK, CONFIG_RX_SAMPLE_RATE)) {
		printf(" -- error: receiver init\n");
		err = true;
		goto tdet_end;
	}

	// Transmitter initialization
	if (tx_init(GPIO_LOOPBACK)) {
		printf(" -- error: transmitter init\n");
		err = true;
		goto tdet_end;
	}

	printf("detector_run() test, threshold:%.1f\n", FILTER_THRESH);
	static const uint16_t play_freq[FILTER_CHANNELS] = CONFIG_PLAY_FREQ;
	float x[FILTER_CHANNELS], y[FILTER_CHANNELS];
	for (uint16_t i = 0; i < FILTER_CHANNELS; i++) en_chan[i] = true;
	detector_setChannels(en_chan);
	detector_setThreshFactor(FILTER_THRESH);
	detector_ignoreAllHits(false);
	for (uint16_t i = 0; i < FILTER_CHANNELS; i++) {
		uint32_t hit_cnt = 0;
		uint32_t rx1_cnt, rx2_cnt, tot_cnt = 0;
		int64_t tbeg, tend, t1, t2, tt = 0; // Used for timing
		y[i] = x[i] = 0.0f;
		rx_clear_buffer();
		filter_reset();
		tend = (tbeg = esp_timer_get_time()) + EP3(CONFIG_TX_PULSE);
		tx_set_freq(play_freq[i]);
		tx_emit(true);
		do {
			// Run filters, compute energy, run hit-detection
			tot_cnt += rx1_cnt = rx_get_count(); // could still miss a count
			t1 = esp_timer_get_time();
			detector_run(); // TODO: return elements processed
			// {while (esp_timer_get_time()-t1 < 625);} // delay for testing
			t2 = esp_timer_get_time();
			rx2_cnt = rx_get_count();
			tt += t2 - t1;
			if (rx1_cnt && rx2_cnt > rx1_cnt) { // Slow detector
				printf(" -- error: slow detector ksamples/sec:%llu pre-cnt:%lu post-cnt:%lu\n",
					EP3(tot_cnt) / tt, rx1_cnt, rx2_cnt);
				err = true;
				break;
			}
			if (detector_getHit()) { // Hit detected
				hit_ch = detector_getHitChannel();
				filter_data_t energy = filter_getEnergyValue(hit_ch);
				hit_cnt++;          // Increment the hit count
				// printf("cnt:%4lu ", tot_cnt);
				printf("hit_ch:%hu energy:%.2e pulse:%2lld ms det:%2lld ms\n",
					hit_ch, energy, EN3(t1-tbeg), EN3(tt));
				x[i] = tot_cnt; y[i] = energy; // for energy/sample calc.
				break;
			}
		} while (esp_timer_get_time() < tend);
		tx_emit(false);
		if (hit_cnt == 0) {
			printf(" -- error: no hit detected\n");
			err = true;
		} else if (hit_ch != i) {
			printf(" -- error: hit on chan:%hu, expecting:%hu\n", hit_ch, i);
			err = true;
		}
		detector_clearHit(); // Clear the hit
	}
	if (!err) {
		float slope, intercept; // for energy/sample calc.
		linear_regression(x, y, FILTER_CHANNELS, &slope, &intercept);
		if (slope < 0.02f || slope > 0.2f) {
			printf(" -- error: energy/sample slope:%.4f intercept:%.4f\n", slope, intercept);
			printf("           possible miscalculation when scaling ADC samples.\n");
			err = true;
		}
	}

tdet_end:
	printf("******** test_detector() %s ********\n\n",
		err ? LOG_COLOR_E "Error" LOG_RESET_COLOR : "Done");
}
