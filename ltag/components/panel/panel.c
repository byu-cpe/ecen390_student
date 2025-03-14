#include <stdio.h>

#include "lcd.h"
#include "panel.h"

#define C_HEAD WHITE
#define C_VAL  GREEN
#define C_SEL  YELLOW
#define C_BACK BLACK

#define FONT_SZ 2
#define SBUF_SZ 12


// Clear each panel region, draw headings
void panel_init(panel_t *p, uint16_t n)
{
	char s[SBUF_SZ];

	lcd_setFontSize(2);
	lcd_setFontBackground(C_BACK);
	for (uint16_t i = 0; i < n; i++, p++) {
		coord_t x = p->x * (LCD_CHAR_W * FONT_SZ);
		coord_t y = p->y * (LCD_CHAR_H * FONT_SZ); // 1st row
		coord_t w = p->w * (LCD_CHAR_W * FONT_SZ);
		lcd_fillRect(x, y, w, LCD_CHAR_H*FONT_SZ*2, C_BACK);
		snprintf(s, SBUF_SZ, "%*s", p->w, p->head);
		lcd_drawString(x, y, s, C_HEAD);
		p->up = true; // panel_update() will draw values
	}
	lcd_noFontBackground();
}

// Update values in each panel if needed
void panel_update(panel_t *p, uint16_t n)
{
	char s[SBUF_SZ];

	lcd_setFontSize(2);
	lcd_setFontBackground(C_BACK);
	for (uint16_t i = 0; i < n; i++, p++) {
		if (!p->up) continue;
		coord_t x = p->x * (LCD_CHAR_W * FONT_SZ);
		coord_t y = (p->y+1) * (LCD_CHAR_H * FONT_SZ); // 2nd row
		switch (p->t) {
		case P_INT:
			snprintf(s, SBUF_SZ, "%*ld", p->w, p->u.i);
			break;
		case P_FLOAT:
			snprintf(s, SBUF_SZ, "%*.2e", p->w, p->u.f);
			break;
		case P_STRING:
			snprintf(s, SBUF_SZ, "%*s", p->w, p->u.s);
			break;
		case P_MAP:
			snprintf(s, SBUF_SZ, "%*s", p->w, p->u.m.v[p->u.m.x]);
			break;
		}
		lcd_drawString(x, y, s, p->sel ? C_SEL : C_VAL);
		p->up = false;
	}
	lcd_noFontBackground();
}
