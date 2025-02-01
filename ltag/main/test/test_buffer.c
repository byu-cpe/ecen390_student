#include <stdint.h>
#include <stdio.h>

#include "buffer.h"

#define MAX_ERROR_CNT 5
#define MARK(n) (n^0x8000)

static uint32_t error_cnt;

static void check_value(buffer_data_t expected)
{
	buffer_data_t found = buffer_pop();

	if (expected != found) {
		if (error_cnt < MAX_ERROR_CNT)
			printf(" -- error: expected: 0x%08X, found: 0x%08X\n", expected, found);
		error_cnt++;
	}
}

void test_buffer(void)
{
	uint32_t i, bsize, start;

	buffer_init();
	bsize = buffer_size();

	printf("half-fill and drain test\n");
	start = 0x10;
	error_cnt = 0;
	for (i = start; i < start+bsize/2; i++) buffer_pushover(MARK(i));
	for (i = start; i < start+bsize/2; i++) check_value(MARK(i));
	printf("errors: %ld\n", error_cnt);

	printf("fill and drain test\n");
	start = 0x20;
	error_cnt = 0;
	for (i = start; i < start+bsize; i++) buffer_pushover(MARK(i));
	for (i = start; i < start+bsize; i++) check_value(MARK(i));
	printf("errors: %ld\n", error_cnt);

	printf("push, pop, push, pop test\n");
	start = 0x30;
	error_cnt = 0;
	for (i = start;         i < start+bsize/2; i++) buffer_pushover(MARK(i));
	for (i = start;         i < start+bsize/4; i++) check_value(MARK(i));
	for (i = start+bsize/2; i < start+bsize;   i++) buffer_pushover(MARK(i));
	for (i = start+bsize/4; i < start+bsize;   i++) check_value(MARK(i));
	printf("errors: %ld\n", error_cnt);

	printf("over-fill and drain test\n");
	start = 0x40;
	error_cnt = 0;
	for (i = start;   i < start+bsize+2; i++) buffer_pushover(MARK(i));
	for (i = start+2; i < start+bsize+2; i++) check_value(MARK(i));
	printf("errors: %ld\n", error_cnt);

	printf("push and over-drain test\n");
	start = 0x50;
	error_cnt = 0;
	for (i = start; i < start+bsize/4; i++) buffer_pushover(MARK(i));
	for (i = start; i < start+bsize/4; i++) check_value(MARK(i));
	check_value(0);
	check_value(0);
	printf("errors: %ld\n", error_cnt);
}
