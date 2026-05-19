#ifndef __TIMER__H__
#define __TIMER__H__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "bits.h"

void timer0_init(void);

#endif