#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "bits.h"
#include "uart.h"
#include "gpu_commands.h"
#include "geometry.h"
#include "timer.h"
#include "buffer.h"

typedef struct {
	Vec3 pos;
	int8_t idx;
} Object;

#define MAX_OBJECTS 5

volatile uint8_t gpu_found = 0;

volatile uint32_t milis = 0;

Object objects[MAX_OBJECTS];

volatile int8_t buttons[16] = {0};

volatile uint32_t buttons_last[16] = {0};

volatile uint8_t active_row = 0;

volatile uint8_t scene_sent = 0;

volatile uint8_t gpu_buffer_size = 0;

volatile int8_t total_faces_sent = 0;

#define BUTTON_COOLDOWN 50

#define B1 0
#define B4 1
#define B7 2
#define BSTAR 3
#define B2 4
#define B5 5
#define B8 6
#define B0 7
#define B3 8
#define B6 9
#define B9 10
#define BHASH 11
#define BA 12
#define BB 13
#define BC 14
#define BD 15

void init_interrupts(void)
{
	SETB(UCSR0B, RXCIE0);
	SETB(TIMSK0, OCIE0A);
	SETB(PCICR, PCIE2);
	SETB(PCMSK2, PCINT18);
	SETB(PCMSK2, PCINT19);
	SETB(PCMSK2, PCINT20);
	SETB(PCMSK2, PCINT21);

	sei();
}

void init_buttons(void)
{
	SETB(DDRD, PD6);
	CLEARB(PORTD, PD6);
	SETB(DDRD, PD7);
	SETB(PORTD, PD7);

	SETB(DDRB, PB0);
	SETB(DDRB, PB1);
	SETB(PORTB, PB0);
	SETB(PORTB, PB1);

	CLEARB(DDRD, PD2);
	CLEARB(DDRD, PD3);
	CLEARB(DDRD, PD4);
	CLEARB(DDRD, PD5);

	// pullup for buttons
	SETB(PORTD, PD2);
	SETB(PORTD, PD3);
	SETB(PORTD, PD4);
	SETB(PORTD, PD5);
}

void reset_button_cooldowns(void)
{
	buttons_last[0] = 0;
	buttons_last[1] = 0;
	buttons_last[2] = 0;
	buttons_last[3] = 0;
	buttons_last[4] = 0;
	buttons_last[5] = 0;
	buttons_last[6] = 0;
	buttons_last[7] = 0;
	buttons_last[8] = 0;
	buttons_last[9] = 0;
	buttons_last[10] = 0;
	buttons_last[11] = 0;
	buttons_last[12] = 0;
	buttons_last[13] = 0;
	buttons_last[14] = 0;
	buttons_last[15] = 0;
}

void reset_buttons(void)
{
	buttons[0] = 0;
	buttons[1] = 0;
	buttons[2] = 0;
	buttons[3] = 0;
	buttons[4] = 0;
	buttons[5] = 0;
	buttons[6] = 0;
	buttons[7] = 0;
	buttons[8] = 0;
	buttons[9] = 0;
	buttons[10] = 0;
	buttons[11] = 0;
	buttons[12] = 0;
	buttons[13] = 0;
	buttons[14] = 0;
	buttons[15] = 0;
}

ISR (USART_RX_vect)
{
	uint8_t resp = UDR0;
	if (resp == HELLO_BACK) {
		gpu_found = 1;
	} else if (resp == GPU_BUFFER_SIZE) {
		gpu_buffer_size = uart_receive_byte();
	}
}

ISR (TIMER0_COMPA_vect)
{
	milis++;
	if (milis == 0) {
		reset_button_cooldowns();
	}
	if (milis % 2000 == 0 && gpu_found) {
		uart_transmit_byte(REQUEST_BUFFER_SIZE);
		// once every two seconds we should ask for the buffer size to know if we can send more data.
		gpu_buffer_size += 1;
	}
}

ISR (PCINT2_vect)
{
	uint8_t col = 0;

	if (!CHECKB(PIND, PD2)) {
		col = 0;
		if (milis - buttons_last[4 * active_row + col] >= BUTTON_COOLDOWN) {
			buttons[4 * active_row + col] = 1;
			buttons_last[4 * active_row + col] = milis;
		}
	}

	if (!CHECKB(PIND, PD3)) {
		col = 1;
		if (milis - buttons_last[4 * active_row + col] >= BUTTON_COOLDOWN) {
			buttons[4 * active_row + col] = 1;
			buttons_last[4 * active_row + col] = milis;
		}
	}

	if (!CHECKB(PIND, PD4)) {
		col = 2;
		if (milis - buttons_last[4 * active_row + col] >= BUTTON_COOLDOWN) {
			buttons[4 * active_row + col] = 1;
			buttons_last[4 * active_row + col] = milis;
		}
	}

	if (!CHECKB(PIND, PD5)) {
		col = 3;
		if (milis - buttons_last[4 * active_row + col] >= BUTTON_COOLDOWN) {
			buttons[4 * active_row + col] = 1;
			buttons_last[4 * active_row + col] = milis;
		}
	}	
}

// BLOCKING
void send_face_to_gpu(int8_t idx, int8_t obj_idx, Vec3 v1, Vec3 v2, Vec3 v3, uint16_t color)
{
	while(gpu_buffer_size >= MAX_BUFFER_SIZE - 41);
	uart_transmit_byte(NEW_FACE);
	uart_transmit_byte(idx);
	uart_transmit_vec3(v1);
	uart_transmit_vec3(v2);
	uart_transmit_vec3(v3);
	uart_transmit_word(color);
	uart_transmit_byte(obj_idx);
	gpu_buffer_size += 41;
}

void send_dummy_face_to_gpu()
{
	// send face id as -1
	send_face_to_gpu(-1, -1, (Vec3){0.f, 0.f, 0.f}, (Vec3){0.f, 0.f, 0.f}, (Vec3){0.f, 0.f, 0.f}, 0x0000);
}

void send_cube(int8_t obj_idx)
{
	float s = 10.f;

	// Fata din fata (z = +s)
	send_face_to_gpu(total_faces_sent + 0, obj_idx, (Vec3){-s, -s,  s}, (Vec3){ s, -s,  s}, (Vec3){ s,  s,  s}, 0xF800);
	send_face_to_gpu(total_faces_sent + 1, obj_idx, (Vec3){-s, -s,  s}, (Vec3){ s,  s,  s}, (Vec3){-s,  s,  s}, 0xF800);

	// Fata din spate (z = -s)
	send_face_to_gpu(total_faces_sent + 2, obj_idx, (Vec3){ s, -s, -s}, (Vec3){-s, -s, -s}, (Vec3){-s,  s, -s}, 0x07E0);
	send_face_to_gpu(total_faces_sent + 3, obj_idx, (Vec3){ s, -s, -s}, (Vec3){-s,  s, -s}, (Vec3){ s,  s, -s}, 0x07E0);

	// Fata stanga (x = -s)
	send_face_to_gpu(total_faces_sent + 4, obj_idx, (Vec3){-s, -s, -s}, (Vec3){-s, -s,  s}, (Vec3){-s,  s,  s}, 0x001F);
	send_face_to_gpu(total_faces_sent + 5, obj_idx, (Vec3){-s, -s, -s}, (Vec3){-s,  s,  s}, (Vec3){-s,  s, -s}, 0x001F);

	// Fata dreapta (x = +s)
	send_face_to_gpu(total_faces_sent + 6, obj_idx, (Vec3){ s, -s,  s}, (Vec3){ s, -s, -s}, (Vec3){ s,  s, -s}, 0xFFE0);
	send_face_to_gpu(total_faces_sent + 7, obj_idx, (Vec3){ s, -s,  s}, (Vec3){ s,  s, -s}, (Vec3){ s,  s,  s}, 0xFFE0);

	// Fata de sus (y = +s)
	send_face_to_gpu(total_faces_sent + 8, obj_idx, (Vec3){-s,  s,  s}, (Vec3){ s,  s,  s}, (Vec3){ s,  s, -s}, 0xF81F);
	send_face_to_gpu(total_faces_sent + 9, obj_idx, (Vec3){-s,  s,  s}, (Vec3){ s,  s, -s}, (Vec3){-s,  s, -s}, 0xF81F);

	// Fata de jos (y = -s)
	send_face_to_gpu(total_faces_sent + 10, obj_idx, (Vec3){-s, -s, -s}, (Vec3){ s, -s, -s}, (Vec3){ s, -s,  s}, 0x07FF);
	send_face_to_gpu(total_faces_sent + 11, obj_idx, (Vec3){-s, -s, -s}, (Vec3){ s, -s,  s}, (Vec3){-s, -s,  s}, 0x07FF);

	total_faces_sent += 12;
}

void send_tetrahedron(int8_t obj_idx)
{
    float s = 10.f;

    Vec3 top   = { 0.f,       s,        0.f      };
    Vec3 front = { 0.f,      -s * 0.333f, s * 0.943f};
    Vec3 left  = {-s * 0.816f,-s * 0.333f,-s * 0.471f};
    Vec3 right = { s * 0.816f,-s * 0.333f,-s * 0.471f};

    // Fata din fata
    send_face_to_gpu(total_faces_sent + 0, obj_idx, top,   front, right, 0xF800);
    // Fata stanga
    send_face_to_gpu(total_faces_sent + 1, obj_idx, top,   left,  front, 0x07E0);
    // Fata dreapta
    send_face_to_gpu(total_faces_sent + 2, obj_idx, top,   right, left,  0x001F);
    // Baza
    send_face_to_gpu(total_faces_sent + 3, obj_idx, front, left,  right, 0xFFE0);

    total_faces_sent += 4;
}

// NON BLOCKING
void rotate_object(Object *obj, int8_t rx, int8_t ry, int8_t rz)
{
	if (gpu_buffer_size < MAX_BUFFER_SIZE - 17) {
		uart_transmit_byte(ROTATE_OBJECT);
		uart_transmit_byte(obj->idx);
		uart_transmit_vec3(obj->pos);
		uart_transmit_byte(rx);
		uart_transmit_byte(ry);
		uart_transmit_byte(rz);
		gpu_buffer_size += 17;
	}
}

void move_object(Object *obj, int8_t dx, int8_t dy, int8_t dz)
{
	if (gpu_buffer_size < MAX_BUFFER_SIZE - 5) {
		uart_transmit_byte(MOVE_OBJECT);
		uart_transmit_byte(obj->idx);
		uart_transmit_byte(dx);
		uart_transmit_byte(dy);
		uart_transmit_byte(dz);
		gpu_buffer_size += 5;

		obj->pos.x += (float)dx;
		obj->pos.y += (float)dy;
		obj->pos.z += (float)dz;
	}
}

void move_object_forced(Object *obj, int8_t dx, int8_t dy, int8_t dz)
{
	while (gpu_buffer_size >= MAX_BUFFER_SIZE - 5);

	uart_transmit_byte(MOVE_OBJECT);
	uart_transmit_byte(obj->idx);
	uart_transmit_byte(dx);
	uart_transmit_byte(dy);
	uart_transmit_byte(dz);
	gpu_buffer_size += 5;

	obj->pos.x += (float)dx;
	obj->pos.y += (float)dy;
	obj->pos.z += (float)dz;
}

// NON BLOCKING
void handle_camera_movement(void)
{
	int8_t inputx = (buttons[B7] - buttons[B9]) * 8;
	int8_t inputz = (buttons[B0] - buttons[B8]) * 8;
	int8_t inputy = (buttons[BSTAR] - buttons[BHASH]) * 8;
	if (inputx || inputy || inputz) {
		if (gpu_buffer_size < MAX_BUFFER_SIZE - 4) {
			uart_transmit_byte(MOVE_CAMERA);
			uart_transmit_byte(inputx);
			uart_transmit_byte(inputz);
			uart_transmit_byte(inputy);
			gpu_buffer_size += 4;
		}

		buttons[B9] = 0;
		buttons[B7] = 0;
		buttons[B0] = 0;
		buttons[B8] = 0;
		buttons[BSTAR] = 0;
		buttons[BHASH] = 0;
	}
}

void handle_camera_rotation(void)
{
	int8_t inputx = (buttons[B3] - buttons[B1]);
	int8_t inputy = (buttons[BC] - buttons[BA]);

	if (inputx || inputy) {
		if (gpu_buffer_size < MAX_BUFFER_SIZE - 3) {
			uart_transmit_byte(ROTATE_CAMERA);
			uart_transmit_byte(inputy);
			uart_transmit_byte(inputx);
			gpu_buffer_size += 3;
		}

		buttons[B3] = 0;
		buttons[B1] = 0;
		buttons[BA] = 0;
		buttons[BC] = 0;
	}
}

void send_scene_data(void)
{
	send_cube(0);

	send_cube(1);
	move_object_forced(&objects[1], 100, 0, 0);

	send_tetrahedron(2);
	move_object_forced(&objects[2], -100, 0, 0);

	scene_sent = 1;
}

void init_object(Object *obj, int8_t idx)
{
	obj->pos = (Vec3){0.f, 0.f, 0.f};
	obj->idx = idx;
}

void handle_input(void)
{
	switch (active_row) {
		case 0:
			active_row = 1;
			SETB(PORTD, PD6);
			CLEARB(PORTD, PD7);		
			break;

		case 1:
			active_row = 2;
			SETB(PORTD, PD7);
			CLEARB(PORTB, PB0);
			break;

		case 2:
			active_row = 3;
			SETB(PORTB, PB0);
			CLEARB(PORTB, PB1);
			break;

		case 3:
			active_row = 0;
			SETB(PORTB, PB1);
			CLEARB(PORTD, PD6);
			break;

		default:
			active_row = 0;
			SETB(PORTD, PD7);
			SETB(PORTB, PB0);
			SETB(PORTB, PB1);
			CLEARB(PORTD, PD6);
			break;
	}
	handle_camera_movement();
	handle_camera_rotation();

	if (buttons[B2]) {
		if (gpu_buffer_size < MAX_BUFFER_SIZE - 1) {
			uart_transmit_byte(RESET_CAMERA);
			gpu_buffer_size += 1;
		}
		buttons[B2] = 0;
	}

	if (buttons[BD]) {
		send_scene_data();
		buttons[BD] = 0;
	}
}

int main()
{
	SETB(DDRB, PB5);
	SETB(PORTB, PB5);
	init_buttons();
	uart_init(115200);
	init_interrupts();
	timer0_init();

	while (!gpu_found) {
		uart_transmit_byte(HELLO_GPU);
		_delay_ms(1000);
	}
	CLEARB(PORTB, PB5);

	for (int8_t i = 0; i < MAX_OBJECTS; i++) init_object(&objects[i], i);


	uint16_t colors[] = {0xF800, 0xA321, 0x00F8, 0xABCD, 0x0065, 0x6767};

	uint8_t i = 0;

	uint32_t last_rotation = 0;

	uint32_t last_movement = 100;

	int8_t movement_direction = 1;

	int8_t dynamic_scene = 1;

	while (1) {
		handle_input();
		if (scene_sent) {
			if (milis - last_rotation >= 300 && dynamic_scene) {
				rotate_object(&objects[0], 0, 15, 0);
				last_rotation = milis;
			}

			if (milis - last_movement >= 500 && dynamic_scene) {
				move_object(&objects[1], 0, movement_direction, 0);

				if (objects[1].pos.y < -10 || objects[1].pos.y > 10) {
					movement_direction = -movement_direction;
				}
				last_movement = 0;
			}
		}
		//_delay_ms(10);
	}

	return 0;
}