#include "kernel.h"

#define isspace(c) ((c) == ' ' || (c) == '\t')
#define isdigit(c) ((c) >= '0' && (c) <= '9')

int
atoi(const char *s)
{
	int n = 0, neg = 0;
	while (isspace(*s))
		s++;
	switch (*s)
	{
		case '-':
			neg = 1;
		case '+':
			s++;
	}
	while (isdigit(*s))
		n = 10 * n - (*s++ - '0');
	return neg ? n : -n;
}
