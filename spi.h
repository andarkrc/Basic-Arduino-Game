#ifndef __SPI__H__
#define __SPI__H__

#include <avr/io.h>
#include <stdint.h>
#include "bits.h"

void spi_init(void);

uint8_t spi_transfer_byte(uint8_t byte);

void spi_transfer_data(uint8_t *data, uint16_t len);

#endif