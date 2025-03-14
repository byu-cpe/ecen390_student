#ifndef PANEL_H_
#define PANEL_H_

#include <stdbool.h>
#include <stdint.h>

// Panels consist of a region on the display with a heading on the
// first row of characters and a value on the second row. Values
// can be integers, floating point values, or strings. Headings and
// values are right justified in the panel region specified by
// x, y, and w. The height is always 2.

#define PANEL_STR_SZ 8

typedef struct {
	char *head; // heading
	bool up; // update flag
	bool sel; // select flag
	int16_t x, y, w; // location in character coordinates
	enum {P_INT, P_FLOAT, P_STRING, P_MAP} t; // data type
	union {
		int32_t i; // integer value
		float f; // float value
		char s[PANEL_STR_SZ]; // string value
		struct { // map
			int16_t x; // array index
			char **v; // array of string values
		} m;
	} u;
} panel_t;

// Clear each panel region, draw headings
void panel_init(panel_t *p, uint16_t n);

// Update values in each panel if needed
void panel_update(panel_t *p, uint16_t n);

#endif // PANEL_H_
