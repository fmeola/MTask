#include "kernel.h"

void
mt_setup_timer(unsigned msecs)
{
	unsigned count = 1193182 *  msecs;
	count /= 1000;

	outb(0x43, 0x26);
	outb(0x40, count);
	outb(0x40, count >> 8);
}

