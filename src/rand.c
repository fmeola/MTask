#include "kernel.h"

static int
do_rand(unsigned *ctx)
{
	int hi, lo, x;

	if (*ctx == 0)
		*ctx = 123459876;
	hi = *ctx / 127773;
	lo = *ctx % 127773;
	x = 16807 * lo - 2836 * hi;
	if (x < 0)
		x += 0x7fffffff;
	return *ctx = x;
}

static unsigned next = 1;

int
rand()
{
	return do_rand(&next);
}

void
srand(unsigned seed)
{
	next = seed;
}
