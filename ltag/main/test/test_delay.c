#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // rand

#include "delay.h"

#define MAX_ERROR_CNT 5
#define MARK(n) ((n)+(delay_data_t)1)
#define SIZE_TESTS 100
#define DELAY_SIZE 50

static uint32_t error_cnt;


static void check_value(delay_t *d, delay_size_t index, delay_data_t expected)
{
	delay_data_t found = delay_read(d, index);

	if (expected != found) {
		if (error_cnt < MAX_ERROR_CNT)
			printf(" -- error: at index: %lu, expected: %2.0f, found: %2.0f\n", index, expected, found);
		error_cnt++;
	}
}

void test_delay(void)
{
	bool err = false;
	uint32_t i, j;
	delay_data_t v;
	delay_t d = {(delay_size_t)-1, (delay_size_t)-1, (delay_data_t *)-1};

	printf("******** test_delay() ********\n");
	printf("initialization test\n");
	error_cnt = 0;
	delay_init(&d, DELAY_SIZE);
	if (d.pos > DELAY_SIZE) {
		printf(" -- error: d.pos (%lu) > %d\n", d.pos, DELAY_SIZE);
		error_cnt++;
	}
	if (d.size != DELAY_SIZE) {
		printf(" -- error: d.size (%lu) != %d\n", d.size, DELAY_SIZE);
		error_cnt++;
	}
	if (d.data == NULL || d.data == (delay_data_t *)-1) {
		printf(" -- error: d.data (%p) invalid\n", d.data);
		error_cnt++;
	}
	err = err || error_cnt;
	if (err) goto td_end;
	for (i = 0; i < DELAY_SIZE; i++) check_value(&d, i, 0);
	err = err || error_cnt;

	printf("save and read test\n");
	error_cnt = 0;
	for (j = 0; j < SIZE_TESTS; j++) {
		delay_size_t elem = rand() % DELAY_SIZE + 1;
		for (i = 0; i < elem; i++) delay_save(&d, MARK(i));
		// d.data[(d.pos+5)%d.size] = (delay_data_t)(DELAY_SIZE+1); // inject error
		for (i = 0; i < elem; i++) check_value(&d, i, MARK(elem-1-i));
	}
	err = err || error_cnt;

	printf("read out-of-bound test\n");
	error_cnt = 0;
	for (i = 0; i < DELAY_SIZE; i++) delay_save(&d, MARK(1));
	if ((v = delay_read(&d, DELAY_SIZE)) != (delay_data_t)0) {
		printf(" -- error: return value (%.0f) non-zero\n", v);
	}
	err = err || error_cnt;

	printf("reset test\n");
	error_cnt = 0;
	delay_reset(&d);
	for (i = 0; i < DELAY_SIZE; i++) check_value(&d, i, 0);
	err = err || error_cnt;

td_end:
	printf("******** test_delay() %s ********\n\n", err ? "Error" : "Done");
}
