#include "timer.h"

void timer0_init(void)
{
	//SETB(TCCR0A, WGM00); this gotta be 0
	SETB(TCCR0A, WGM01);

	OCR0A = 250;

	TCCR0B |= (1 << CS01) | (1 << CS00);

	TCNT0 = 0;
}