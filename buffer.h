#ifndef __BUFFER__H__
#define __BUFFER__H__

#include <stdint.h>

#include "geometry.h"

#define MAX_BUFFER_SIZE 45

typedef struct {
	int8_t write_idx;
	int8_t read_idx;
	uint8_t size;
	uint8_t data[MAX_BUFFER_SIZE];
} Buffer;

void buffer_init(Buffer *buf);

void buffer_write_byte(Buffer *buf, uint8_t data);

void buffer_skip_bytes(Buffer *buf, uint8_t size);

uint8_t buffer_peek_byte(Buffer *buf);

uint8_t buffer_read_byte(Buffer *buf);

uint16_t buffer_read_word(Buffer *buf);

float buffer_read_float(Buffer *buf);

Vec3 buffer_read_vec3(Buffer *buf);

#endif