#ifndef __LCD__SCREEN__DRIVER__H__
#define __LCD__SCREEN__DRIVER__H__

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "spi.h"
#include "bits.h"
#include "geometry.h"

#define display_width 128
#define display_height 160


void display_power_on_seq(void);
void display_sleep_out(void);
void display_on(void);
void display_madctl(uint8_t flags);
void display_reset(void);
void display_set_color_mode(uint8_t mode);
void display_set_draw_area(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

void display_send_command(uint8_t command);

void display_send_data(uint8_t *data, uint16_t len);

void draw_rectangle(ScreenPoint p1, ScreenPoint p2, uint16_t color);

void draw_triangle(ScreenPoint p1, ScreenPoint p2, ScreenPoint p3, uint16_t color);

void draw_pixel(ScreenPoint p, uint16_t color);

void draw_number(uint8_t number);

#endif