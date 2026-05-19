#include "spi.h"

void spi_init(void)
{
	SETB(DDRB, PB0);
	SETB(DDRB, PB5);
	SETB(DDRB, PB3);
	SETB(DDRB, PB2);
	SETB(DDRB, PB1);
	CLEARB(PORTB, PB1);
	SETB(PORTB, PB2);
	SETB(PORTB, PB0);
	SPCR = (1 << SPE) | (1 << MSTR) | (0 << SPR0);
	SPSR = (1 << SPI2X);
}

uint8_t spi_transfer_byte(uint8_t byte)
{
	SPDR = byte;
	while (!CHECKB(SPSR, SPIF));

	return SPDR;
}

void spi_transfer_data(uint8_t *data, uint16_t len)
{
	SPDR = data[0];
	for (uint16_t i = 1; i < len; i++) {
		while (!CHECKB(SPSR, SPIF));
		SPDR = data[i];
	}
	while (!CHECKB(SPSR, SPIF));
}