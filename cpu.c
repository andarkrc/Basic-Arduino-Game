#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "bits.h"
#include "uart.h"
#include "gpu_commands.h"
#include "geometry.h"
#include "timer.h"

volatile uint8_t gpu_found = 0;

volatile uint32_t milis = 0;

volatile int8_t buttons[16] = {0};

volatile uint32_t buttons_last[16] = {0};

volatile uint8_t active_row = 0;

volatile uint8_t ready_for_face = 1;

volatile uint8_t scene_sent = 0;

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
	uint8_t resp = uart_receive_byte();
	if (resp == HELLO_BACK) {
		gpu_found = 1;
	} else if (resp == READY_FOR_FACE) {
		ready_for_face = 1;
	}
}

ISR (TIMER0_COMPA_vect)
{
	milis++;
	if (milis == 0) {
		reset_button_cooldowns();
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

void send_face_to_gpu(int8_t idx, int8_t obj_idx, Vec3 v1, Vec3 v2, Vec3 v3, uint16_t color)
{
	while(!ready_for_face);
	uart_transmit_byte(NEW_FACE);
	uart_transmit_byte(idx);
	uart_transmit_vec3(v1);
	uart_transmit_vec3(v2);
	uart_transmit_vec3(v3);
	uart_transmit_word(color);
	uart_transmit_byte(obj_idx);
	ready_for_face = 0;
}

void send_dummy_face_to_gpu()
{
	// intentionally empty so we don't lose actual face info
	while(!ready_for_face);
	uart_transmit_byte(NEW_FACE);
	uart_transmit_byte(-1);
	uart_transmit_vec3((Vec3){0.f, 0.f, 0.f});
	uart_transmit_vec3((Vec3){0.f, 0.f, 0.f});
	uart_transmit_vec3((Vec3){0.f, 0.f, 0.f});
	uart_transmit_word(0x0000);
	uart_transmit_byte(-1);
	ready_for_face = 0;
}

void send_scene_data(void)
{
	float s = 10.f;

	send_dummy_face_to_gpu();

	// Fata din fata (z = +s)
	send_face_to_gpu(0,  0, (Vec3){-s, -s,  s}, (Vec3){ s, -s,  s}, (Vec3){ s,  s,  s}, 0xF800);

	send_face_to_gpu(1,  0, (Vec3){-s, -s,  s}, (Vec3){ s,  s,  s}, (Vec3){-s,  s,  s}, 0xF800);

	// Fata din spate (z = -s)
	send_face_to_gpu(2,  0, (Vec3){ s, -s, -s}, (Vec3){-s, -s, -s}, (Vec3){-s,  s, -s}, 0x07E0);

	send_face_to_gpu(3,  0, (Vec3){ s, -s, -s}, (Vec3){-s,  s, -s}, (Vec3){ s,  s, -s}, 0x07E0);

	// Fata stanga (x = -s)
	send_face_to_gpu(4,  0, (Vec3){-s, -s, -s}, (Vec3){-s, -s,  s}, (Vec3){-s,  s,  s}, 0x001F);

	send_face_to_gpu(5,  0, (Vec3){-s, -s, -s}, (Vec3){-s,  s,  s}, (Vec3){-s,  s, -s}, 0x001F);


	// Fata dreapta (x = +s)
	send_face_to_gpu(6,  0, (Vec3){ s, -s,  s}, (Vec3){ s, -s, -s}, (Vec3){ s,  s, -s}, 0xFFE0);

	send_face_to_gpu(7,  0, (Vec3){ s, -s,  s}, (Vec3){ s,  s, -s}, (Vec3){ s,  s,  s}, 0xFFE0);


	// Fata de sus (y = +s)
	send_face_to_gpu(8,  0, (Vec3){-s,  s,  s}, (Vec3){ s,  s,  s}, (Vec3){ s,  s, -s}, 0xF81F);

	send_face_to_gpu(9,  0, (Vec3){-s,  s,  s}, (Vec3){ s,  s, -s}, (Vec3){-s,  s, -s}, 0xF81F);

	// Fata de jos (y = -s)
	send_face_to_gpu(10, 0, (Vec3){-s, -s, -s}, (Vec3){ s, -s, -s}, (Vec3){ s, -s,  s}, 0x07FF);

	send_face_to_gpu(11, 0, (Vec3){-s, -s, -s}, (Vec3){ s, -s,  s}, (Vec3){-s, -s,  s}, 0x07FF);

	scene_sent = 1;
}

void rotate_object(int8_t obj_idx, int8_t rx, int8_t ry, int8_t rz)
{
	uart_transmit_byte(ROTATE_OBJECT);
	uart_transmit_byte(obj_idx);
	uart_transmit_byte(rx);
	uart_transmit_byte(ry);
	uart_transmit_byte(rz);
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
	int8_t inputx = (buttons[B9] - buttons[B7]) * 8;
	int8_t inputz = (buttons[B0] - buttons[B8]) * 8;
	int8_t inputy = (buttons[BSTAR] - buttons[BHASH]) * 8;
	if (inputx || inputy || inputz) {
		uart_transmit_byte(MOVE_CAMERA);
		uart_transmit_byte(inputx);
		uart_transmit_byte(inputy);
		uart_transmit_byte(inputz);

		buttons[B9] = 0;
		buttons[B7] = 0;
		buttons[B0] = 0;
		buttons[B8] = 0;
		buttons[BSTAR] = 0;
		buttons[BHASH] = 0;
	}

	inputx = (buttons[B3] - buttons[B1]);
	inputy = (buttons[BA] - buttons[BC]);

	if (inputx || inputy) {
		uart_transmit_byte(ROTATE_CAMERA);
		uart_transmit_byte(inputy);
		uart_transmit_byte(inputx);

		buttons[B3] = 0;
		buttons[B1] = 0;
		buttons[BA] = 0;
		buttons[BC] = 0;
	}

	if (buttons[B2]) {
		uart_transmit_byte(RESET_CAMERA);
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
	uart_init(9600);
	init_interrupts();
	timer0_init();

	while (!gpu_found) {
		uart_transmit_byte(HELLO_GPU);
		_delay_ms(1000);
	}
	CLEARB(PORTB, PB5);

	uint16_t colors[] = {0xF800, 0xA321, 0x00F8, 0xABCD, 0x0065, 0x6767};

	uint8_t i = 0;

	while (1) {
		handle_input();
		if (scene_sent && 0) {
			if (milis % 500 == 0) {
				rotate_object(0, 0, 15, 0);
			}
		}
		//_delay_ms(10);
	}

	return 0;
}