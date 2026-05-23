#include "uart.h"

#define TO_UBRR(baud_rate) ((F_CPU / (16UL * (baud_rate))) - 1)

void uart_init(uint32_t baud_rate)
{
	baud_rate = TO_UBRR(baud_rate);

	UBRR0H = (uint8_t)(baud_rate >> 8);
	UBRR0L = (uint8_t)baud_rate;

	SETB(UCSR0C, UCSZ01);
	SETB(UCSR0C, UCSZ00);

	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
}

void uart_transmit_byte(uint8_t data)
{
	while (!CHECKB(UCSR0A, UDRE0));

	UDR0 = data;
}

uint8_t uart_receive_byte(void)
{
	while (!CHECKB(UCSR0A, RXC0));

	return UDR0;
}

void uart_transmit_word(uint16_t data)
{
	while (!CHECKB(UCSR0A, UDRE0));

	UDR0 = (uint8_t)(data >> 8); // high

	while (!CHECKB(UCSR0A, UDRE0));

	UDR0 = (uint8_t)(data); // low
}

uint16_t uart_receive_word(void)
{
	uint8_t high;
	uint8_t low;

	while (!CHECKB(UCSR0A, RXC0));

	high = UDR0;

	while (!CHECKB(UCSR0A, RXC0));

	low = UDR0;

	return (uint16_t)high << 8 | (uint16_t)low;
}

void uart_transmit_float(float data)
{
	uint8_t *p = (uint8_t*)&data;

	for (uint8_t i = 0; i < 4; i++) {
		while (!CHECKB(UCSR0A, UDRE0));
		UDR0 = p[i];
	}
}

float uart_receive_float(void)
{
	float data;
	uint8_t *p = (uint8_t*)&data;

	for (uint8_t i = 0; i < 4; i++) {
		while (!CHECKB(UCSR0A, RXC0));
		p[i] = UDR0;
	}

	return data;
}

void uart_transmit_vec3(Vec3 data)
{
	uart_transmit_float(data.x);
	uart_transmit_float(data.y);
	uart_transmit_float(data.z);
}

Vec3 uart_receive_vec3(void)
{
	Vec3 ret;
	ret.x = uart_receive_float();
	ret.y = uart_receive_float();
	ret.z = uart_receive_float();

	return ret;
}

void uart_skip_bytes(uint16_t bytes)
{
	for (uint16_t i = 0; i < bytes; i++) {
		uart_receive_byte();
	}
}

