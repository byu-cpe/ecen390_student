#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "buffer.h"

#define MAX_ERROR_CNT 5
#define MARK(n) (n^0x8000)
#define LOW_SZ 8192
#define HIGH_SZ 32768

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
	bool err = false;
	uint32_t i, bsize, start;

	printf("******** test_buffer() ********\n");
	printf("initialization test\n");
	buffer_init();
	bsize = buffer_size();
	if (bsize < LOW_SZ || bsize > HIGH_SZ) {
		printf(" -- error: buffer size (%lu) out of range (%u, %u)\n",
			bsize, LOW_SZ, HIGH_SZ);
		error_cnt++;
	}
	err = err || error_cnt;
	if (err) goto tb_end;
	check_value(0);
	err = err || error_cnt;
	if (err) goto tb_end;


	printf("half-fill and drain test\n");
	start = 0x10;
	error_cnt = 0;
	for (i = start; i < start+bsize/2; i++) buffer_pushover(MARK(i));
	for (i = start; i < start+bsize/2; i++) check_value(MARK(i));
	if (error_cnt) printf("errors: %lu\n", error_cnt);
	err = err || error_cnt;

	printf("fill and drain test\n");
	start = 0x20;
	error_cnt = 0;
	for (i = start; i < start+bsize; i++) buffer_pushover(MARK(i));
	for (i = start; i < start+bsize; i++) check_value(MARK(i));
	if (error_cnt) printf("errors: %lu\n", error_cnt);
	err = err || error_cnt;

	printf("push, pop, push, pop test\n");
	start = 0x30;
	error_cnt = 0;
	for (i = start;         i < start+bsize/2; i++) buffer_pushover(MARK(i));
	for (i = start;         i < start+bsize/4; i++) check_value(MARK(i));
	for (i = start+bsize/2; i < start+bsize;   i++) buffer_pushover(MARK(i));
	for (i = start+bsize/4; i < start+bsize;   i++) check_value(MARK(i));
	if (error_cnt) printf("errors: %lu\n", error_cnt);
	err = err || error_cnt;

	printf("over-fill and drain test\n");
	start = 0x40;
	error_cnt = 0;
	for (i = start;   i < start+bsize+2; i++) buffer_pushover(MARK(i));
	for (i = start+2; i < start+bsize+2; i++) check_value(MARK(i));
	if (error_cnt) printf("errors: %lu\n", error_cnt);
	err = err || error_cnt;

	printf("push and over-drain test\n");
	start = 0x50;
	error_cnt = 0;
	for (i = start; i < start+bsize/4; i++) buffer_pushover(MARK(i));
	for (i = start; i < start+bsize/4; i++) check_value(MARK(i));
	check_value(0);
	check_value(0);
	if (error_cnt) printf("errors: %lu\n", error_cnt);
	err = err || error_cnt;

tb_end:
	printf("******** test_buffer() %s ********\n\n", err ? "Error" : "Done");
}
