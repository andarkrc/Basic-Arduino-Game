#include <avr/io.h>
#include <util/delay.h>
#include "bits.h"

int main() {
	SETB(DDRB, PB5);
	CLEARB(PORTB, PB5);
	return 0;
}