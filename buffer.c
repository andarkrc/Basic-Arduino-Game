#include "buffer.h"

void buffer_init(Buffer *buf)
{
	buf->read_idx = 0;
	buf->write_idx = 0;
	buf->size = 0;
}

void buffer_write_byte(Buffer *buf, uint8_t data)
{
	if (buf->size == MAX_BUFFER_SIZE) {
		return;
	}

	buf->data[buf->write_idx] = data;
	buf->write_idx = (buf->write_idx + 1) % MAX_BUFFER_SIZE;
	buf->size++;
}

uint8_t buffer_peek_byte(Buffer *buf)
{
	if (buf->size < 1) {
		return 0;
	}

	return buf->data[buf->read_idx];
}

uint8_t buffer_read_byte(Buffer *buf)
{
	if (buf->size < 1) {
		return 0;
	}
	uint8_t ret = buf->data[buf->read_idx];
	buf->read_idx = (buf->read_idx + 1) % MAX_BUFFER_SIZE;
	buf->size--;

	return ret;
}

uint16_t buffer_read_word(Buffer *buf)
{
	if (buf->size < 2) {
		return 0;
	}
	uint16_t ret = (buf->data[buf->read_idx] << 8);
	buf->read_idx = (buf->read_idx + 1) % MAX_BUFFER_SIZE;
	
	ret |= buf->data[buf->read_idx];
	buf->read_idx = (buf->read_idx + 1) % MAX_BUFFER_SIZE;
	buf->size -= 2;

	return ret;
}

float buffer_read_float(Buffer *buf)
{
	if (buf->size < 4) {
		return 0.f;
	}

	float data;
	uint8_t *p = (uint8_t*)&data;

	for (uint8_t i = 0; i < 4; i++) {
		p[i] = buf->data[buf->read_idx];
		buf->read_idx = (buf->read_idx + 1) % MAX_BUFFER_SIZE;
	}

	buf->size -= 4;

	return data;
}

Vec3 buffer_read_vec3(Buffer *buf)
{
	if (buf->size < 12) {
		return (Vec3){0.f, 0.f, 0.f};
	}

	Vec3 ret;
	ret.x = buffer_read_float(buf);
	ret.y = buffer_read_float(buf);
	ret.z = buffer_read_float(buf);

	return ret;
}

void buffer_skip_bytes(Buffer *buf, uint8_t size)
{
	for (uint8_t i = 0; i < size; i++) {
		buffer_read_byte(buf);
	}
}