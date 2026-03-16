#include <avr/io.h>
#include <util/delay.h>
#include "bits.h"

int main() {
	/* Setăm pinul 7 al portului D ca pin de ieșire. */
	SETB(DDRB, PB5);
 
	CLEARB(PORTB, PB5);
 
	return 0;
}