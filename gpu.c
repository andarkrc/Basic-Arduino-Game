#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "bits.h"
#include "spi.h"
#include "lcd_screen.h"
#include "uart.h"
#include "gpu_commands.h"
#include "buffer.h"

typedef struct __attribute__((packed)) {
	Vec3 v1;
	Vec3 v2;
	Vec3 v3;
	uint16_t color;
	int8_t idx;
	int8_t obj_idx;
} Face;

#define MAX_FACES 40

Buffer command_buffer;

Camera cam;

volatile int8_t face_no = 0;

volatile Face faces[MAX_FACES];

volatile uint16_t display_color = 0x0000;

volatile uint8_t number_to_display = 0;

void init_interrupts(void)
{
	SETB(UCSR0B, RXCIE0);

	sei();
}

void process_command(uint8_t command)
{
	uint32_t timeout = 50000;
	int8_t idx;
	uint8_t skip = 0;
	switch (command) {
		case HELLO_GPU:
			uart_transmit_byte(HELLO_BACK);
			break;

		case CHANGE_DISPLAY_COLOR:
			while (command_buffer.size < 2 && --timeout);
			if (!timeout) {
				buffer_skip_bytes(&command_buffer, command_buffer.size);
				break;
			}
			display_color = buffer_read_word(&command_buffer);
			break;

		case NEW_FACE:
			timeout = 300000;
			while (command_buffer.size < 40 && --timeout);
			if (!timeout) {
				buffer_skip_bytes(&command_buffer, command_buffer.size);
				number_to_display = command_buffer.size;
				break;
			}
			if (face_no >= MAX_FACES) {
				buffer_skip_bytes(&command_buffer, 40);
				break;
			}
			idx = (int8_t)buffer_read_byte(&command_buffer);
			if (idx < 0) {
				buffer_skip_bytes(&command_buffer, 39);
				break;
			}
			for (int8_t i = 0; i < face_no; i++) {
				if (faces[i].idx == idx) {
					buffer_skip_bytes(&command_buffer, 39);
					skip = 1;
				}
			}
			if (skip) break;
			faces[face_no].idx = idx;
			faces[face_no].v1 = buffer_read_vec3(&command_buffer);
			faces[face_no].v2 = buffer_read_vec3(&command_buffer);
			faces[face_no].v3 = buffer_read_vec3(&command_buffer);
			faces[face_no].color = buffer_read_word(&command_buffer);
			faces[face_no].obj_idx = buffer_read_byte(&command_buffer);
			face_no++;
			uart_transmit_byte(READY_FOR_FACE);
			break;

		case MOVE_CAMERA:
			while (command_buffer.size < 3 && --timeout);
			if (!timeout) {
				buffer_skip_bytes(&command_buffer, command_buffer.size);
				break;
			}
			cam.pos.x += (float)(int8_t)buffer_read_byte(&command_buffer);
			cam.pos.y += (float)(int8_t)buffer_read_byte(&command_buffer);
			cam.pos.z += (float)(int8_t)buffer_read_byte(&command_buffer);
			break;

		case ROTATE_OBJECT:{
			while (command_buffer.size < 4 && --timeout);
			if (!timeout) {
				buffer_skip_bytes(&command_buffer, command_buffer.size);
				break;
			}
			int8_t obj_idx = (int8_t)buffer_read_byte(&command_buffer);
			float rx = to_rad((float)(int8_t)buffer_read_byte(&command_buffer));
			float ry = to_rad((float)(int8_t)buffer_read_byte(&command_buffer));
			float rz = to_rad((float)(int8_t)buffer_read_byte(&command_buffer));
			for (int8_t i = 0; i < face_no; i++) {
				if (faces[i].obj_idx == obj_idx) {
					faces[i].v1 = rotate_xyz(faces[i].v1, rx, ry, rz);
					faces[i].v2 = rotate_xyz(faces[i].v2, rx, ry, rz);
					faces[i].v3 = rotate_xyz(faces[i].v3, rx, ry, rz);
				}
			}
			}
			break;

		case MOVE_OBJECT:{
			while (command_buffer.size < 4 && --timeout);
			if (!timeout) {
				buffer_skip_bytes(&command_buffer, command_buffer.size);
				break;
			}
			int8_t obj_idx = (int8_t)buffer_read_byte(&command_buffer);
			int8_t dx = (int8_t)buffer_read_byte(&command_buffer);
			int8_t dy = (int8_t)buffer_read_byte(&command_buffer);
			int8_t dz = (int8_t)buffer_read_byte(&command_buffer);
			for (int8_t i = 0; i < face_no; i++) {
				if (faces[i].obj_idx == obj_idx) {
					faces[i].v1.x += (float)dx;
					faces[i].v2.x += (float)dx;
					faces[i].v3.x += (float)dx;
					faces[i].v1.y += (float)dy;
					faces[i].v2.y += (float)dy;
					faces[i].v3.y += (float)dy;
					faces[i].v1.z += (float)dz;
					faces[i].v2.z += (float)dz;
					faces[i].v3.z += (float)dz;
				}
			}
			}
			break;

		case ROTATE_CAMERA:
			while (command_buffer.size < 2 && --timeout);
			if (!timeout) {
				buffer_skip_bytes(&command_buffer, command_buffer.size);
				break;
			}
			camera_rotate(&cam, (float)(int8_t)buffer_read_byte(&command_buffer) / 4.f, (float)(int8_t)buffer_read_byte(&command_buffer) / 4.f);
			break;

		case RESET_CAMERA:
			cam = camera_look_at(
			    (Vec3){0, 0, -100},
			    (Vec3){0, 0,  0}
			);
			break;

			// NO IDEA WHAT TRASH IS IN THE BUFFER, JUST FLUSH IT ALL OUT. SORRY NOT SORRY
		default:
			number_to_display = command;
			//buffer_skip_bytes(&command_buffer, command_buffer.size);
			break;
	}
}

ISR (USART_RX_vect)
{
	buffer_write_byte(&command_buffer, UDR0);
}

float face_dist_sq(const volatile Face *f, Camera *c)
{
    float cx = (f->v1.x + f->v2.x + f->v3.x) / 3.f - c->pos.x;
    float cy = (f->v1.y + f->v2.y + f->v3.y) / 3.f - c->pos.y;
    float cz = (f->v1.z + f->v2.z + f->v3.z) / 3.f - c->pos.z;
    return cx*cx + cy*cy + cz*cz;
}

int main()
{
	spi_init();
	
	CLEARB(PORTB, PB0); // select the display
	display_power_on_seq();
	display_madctl(0xC0); // flip x & y axis

	buffer_init(&command_buffer);

	uart_init(9600);
	init_interrupts(); // ready to listen for commands.
	

	cam = camera_look_at(
	    (Vec3){0, 0, -100},
	    (Vec3){0, 0,  0}
	);

	while (1) {
		display_color = 0x0000;
		if (command_buffer.size != 0) {
			//number_to_display = buffer_peek_byte(&command_buffer);
			process_command(buffer_read_byte(&command_buffer));
		}
		if (command_buffer.size <= 1) {
			uart_transmit_byte(READY_FOR_FACE);
		}
		draw_rectangle((ScreenPoint){0,0}, (ScreenPoint){display_width, display_height}, display_color);

		int8_t local_face_no = face_no;

		// Painter's approach: 2 passes of bubble sort xd
		// no depth buffer so we try this: sort by avg distance to camera.
		for (uint8_t pass = 0; pass < 2; pass++) {
	        for (uint8_t i = 0; i < local_face_no - 1; i++) {
	            if (face_dist_sq(&faces[i], &cam) < face_dist_sq(&faces[i+1], &cam)) {
	                Face tmp = faces[i];
	                faces[i] = faces[i+1];
	                faces[i+1] = tmp;
	            }
	        }
	    }

		for (uint8_t i = 0; i < local_face_no; i++) {
			ScreenPoint p1 = project(faces[i].v1, cam);
			ScreenPoint p2 = project(faces[i].v2, cam);
			ScreenPoint p3 = project(faces[i].v3, cam);

			draw_triangle(p1, p2, p3, faces[i].color);
		}
		
		draw_number(number_to_display);
		_delay_ms(10);
	}
	
	return 0;
}