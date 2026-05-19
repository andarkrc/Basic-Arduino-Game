#ifndef __UART__H__
#define __UART__H__

#include <avr/io.h>
#include <stdint.h>

#include "bits.h"

#include "geometry.h"

void uart_init(uint16_t baud_rate);

void uart_transmit_byte(uint8_t data);

uint8_t uart_receive_byte(void);

void uart_transmit_word(uint16_t data);

uint16_t uart_receive_word(void);

void uart_transmit_float(float data);

float uart_receive_float(void);

void uart_transmit_vec3(Vec3 data);

Vec3 uart_receive_vec3(void);

void uart_skip_bytes(uint16_t bytes);

#define uart_disable_receive() CLEARB(UCSR0B, RXEN0)

#define uart_enable_receive() SETB(UCSR0B, RXEN0)

#endif