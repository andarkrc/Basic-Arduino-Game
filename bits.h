#ifndef __BITS_H__
#define __BITS_H__

#define SETB(container, bit) (container |= (1 << (bit)))
#define CLEARB(container, bit) (container &= ~(1 << (bit)))
#define CHECKB(container, bit) (container & (1 << (bit)))
#define TOGGLEB(container, bit) (container ^= (1 << (bit)))


#define LED_ON(container, bit) CLEARB(container, bit)
#define LED_OFF(container, bit) SETB(container, bit)

#endif