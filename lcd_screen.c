#include "lcd_screen.h"

void display_power_on_seq(void)
{
	display_sleep_out();
	display_set_color_mode(0x05); // 16 bit
	display_on();
}

void display_sleep_out(void)
{
	display_send_command(0x11);
	_delay_ms(150);
}

void display_on(void)
{
	display_send_command(0x29);
	_delay_ms(150);
}

void display_madctl(uint8_t flags)
{
	display_send_command(0x36);
	display_send_data(&flags, 1);
}

void display_reset(void)
{
	display_send_command(0x01);
	_delay_ms(200);
}

void display_set_color_mode(uint8_t mode)
{
	display_send_command(0x3A);
	display_send_data(&mode, 1);
}

void display_set_draw_area(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	display_send_command(0x2A);
	uint8_t cols[] = {0, x1, 0, x2};
	display_send_data(cols, 4);

	display_send_command(0x2B);
	uint8_t rows[] = {0, y1, 0, y2};
	display_send_data(rows, 4);
}

void display_send_command(uint8_t command)
{
	CLEARB(PORTB, PB2);
	spi_transfer_byte(command);
}

void display_send_data(uint8_t *data, uint16_t len)
{
	SETB(PORTB, PB2);
	spi_transfer_data(data, len);
}

void draw_rectangle(ScreenPoint p1, ScreenPoint p2, uint16_t color)
{
	if (p2.x < p1.x) {
		int16_t t = p2.x;
		p2.x = p1.x;
		p1.x = t;
	}

	if (p2.y < p1.y) {
		int16_t t = p2.y;
		p2.y = p1.y;
		p1.y = t;
	}

	if (p1.x < 0) {
		p1.x = 0;
	}

	if (p1.y < 0) {
		p1.y = 0;
	}

	if (p2.x >= display_width) {
		p2.x = display_width - 1;
	}

	if (p2.y >= display_height) {
		p2.y = display_height - 1;
	}

	display_send_command(0x2A);
	uint8_t cols[] = {0, (uint8_t)p1.x, 0, (uint8_t)p2.x};
	display_send_data(cols, 4);

	display_send_command(0x2B);
	uint8_t rows[] = {0, (uint8_t)p1.y, 0, (uint8_t)p2.y};
	display_send_data(rows, 4);

	display_send_command(0x2C);
	SETB(PORTB, PB2);

	for (uint16_t i = p1.y; i <= p2.y; i++) {
		for (uint16_t j = p1.x; j <= p2.x; j++) {
			spi_transfer_byte((uint8_t)(color >> 8));
			spi_transfer_byte((uint8_t)color);
		}
	}
}

void draw_pixel(ScreenPoint p, uint16_t color)
{
	display_set_draw_area(p.x, p.y, p.x, p.y);
	display_send_command(0x2C);
	SETB(PORTB, PB2);
	spi_transfer_byte((uint8_t)(color >> 8));
	spi_transfer_byte((uint8_t)color);
}

void draw_number(uint8_t number)
{
	uint16_t hundreds = (uint16_t)number / 100;
	uint16_t tens = ((uint16_t)number % 100) / 10;
	uint16_t units = ((uint16_t)number % 10);

	for (uint16_t i = 0; i < hundreds; i++) {
		draw_pixel((ScreenPoint){3 * i, 0}, 0xffff);
	}

	for (uint16_t i = 0; i < tens; i++) {
		draw_pixel((ScreenPoint){3 * i, 10}, 0xffff);
	}

	for (uint16_t i = 0; i < units; i++) {
		draw_pixel((ScreenPoint){3 * i, 20}, 0xffff);
	}
}

static void draw_horizontal_line(ScreenPoint p1, ScreenPoint p2, uint16_t color)
{
	if (p2.x < p1.x) {
		ScreenPoint t = p1;
		p1 = p2;
		p2 = t;
	}

	if (p1.x < 0) {
		p1.x = 0;
	}

	if (p1.y < 0) {
		p1.y = 0;
	}

	if (p2.x >= display_width) {
		p2.x = display_width - 1;
	}

	if (p2.y >= display_height) {
		p2.y = display_height - 1;
	}

	display_send_command(0x2A);
	uint8_t cols[] = {0, (uint8_t)p1.x, 0, (uint8_t)p2.x};
	display_send_data(cols, 4);

	display_send_command(0x2B);
	uint8_t rows[] = {0, (uint8_t)p1.y, 0, (uint8_t)p2.y};
	display_send_data(rows, 4);
	
	

	display_send_command(0x2C);
	SETB(PORTB, PB2);
	for (int16_t i = p1.x; i <= p2.x; i++) {
		spi_transfer_byte((uint8_t)(color >> 8));
		spi_transfer_byte((uint8_t)color);
	}
}


static void fillBottomFlatTriangle(ScreenPoint p1, ScreenPoint p2, ScreenPoint p3, uint16_t color)
{
	float invslope1 = (float)(p2.x - p1.x) / (float)(p2.y - p1.y);
	float invslope2 = (float)(p3.x - p1.x) / (float)(p3.y - p1.y);

	float curx1 = p1.x;
	float curx2 = p1.x;

	for (int16_t scanlineY = p1.y; scanlineY <= p2.y; scanlineY++)
	{
		if (scanlineY < 0 || scanlineY >= display_height) continue;

		float clipx1 = curx1;
		float clipx2 = curx2;
		if (clipx1 < 0.0f) {
			clipx1 = 0.0f;
		}
		if (clipx2 >= (float)display_width) {
			clipx2 = (float)(display_width - 1);
		}

		draw_horizontal_line((ScreenPoint){clipx1, scanlineY}, (ScreenPoint){clipx2, scanlineY}, color);
		curx1 += invslope1;
		curx2 += invslope2;
	}
}

 	

static void fillTopFlatTriangle(ScreenPoint p1, ScreenPoint p2, ScreenPoint p3, uint16_t color)
{
	float invslope1 = (float)(p3.x - p1.x) / (float)(p3.y - p1.y);
	float invslope2 = (float)(p3.x - p2.x) / (float)(p3.y - p2.y);

	float curx1 = p3.x;
	float curx2 = p3.x;

	for (int16_t scanlineY = p3.y; scanlineY > p1.y; scanlineY--)
	{
		if (scanlineY < 0 || scanlineY >= display_height) continue;

		float clipx1 = curx1;
		float clipx2 = curx2;
		if (clipx1 < 0.0f) {
			clipx1 = 0.0f;
		}
		if (clipx2 >= (float)display_width) {
			clipx2 = (float)(display_width - 1);
		}
		draw_horizontal_line((ScreenPoint){clipx1, scanlineY}, (ScreenPoint){clipx2, scanlineY}, color);
		curx1 -= invslope1;
		curx2 -= invslope2;
	}
}

void draw_triangle(ScreenPoint p1, ScreenPoint p2, ScreenPoint p3, uint16_t color)
{
	// y sorting so that p1.y < p2.y < p3.y
	if (p2.y < p1.y) {
		ScreenPoint t = p1;
		p1 = p2;
		p2 = t; 
	}
    if (p3.y < p1.y) {
    	ScreenPoint t = p1;
    	p1 = p3;
    	p3 = t;
    }
    if (p3.y < p2.y) {
    	ScreenPoint t = p2;
    	p2 = p3;
    	p3 = t;
    }
    if (p2.y == p3.y) {
    	fillBottomFlatTriangle(p1, p2, p3, color);
    } else if (p1.y == p2.y){
    	fillTopFlatTriangle(p1, p2, p3, color);
    } else {
    	ScreenPoint p4;
    	p4.y = p2.y;
    	p4.x = (int16_t)(p1.x + ((float)(p2.y - p1.y) / (float)(p3.y - p1.y)) * (p3.x - p1.x));
		fillBottomFlatTriangle(p1, p2, p4, color);
		fillTopFlatTriangle(p2, p4, p3, color);
    }
}