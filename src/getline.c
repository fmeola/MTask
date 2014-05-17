#include "kernel.h"

#define BS 0x08
#define ERASEBACK "\x08 \x08"

unsigned
mt_getline(char *buf, unsigned size)
{
	char *p = buf, *end = buf + size - 1;
	unsigned c;
	unsigned xi, yi, si;
	
	mt_cons_getxy(&xi, &yi);
	si = mt_cons_nscrolls();

	while (p < end)
		if (mt_kbd_getch(&c))
			switch (c)
			{
				case BS:
					if (p == buf)
						break;
					if (*--p == '\t')
					{
						mt_cons_gotoxy(xi, yi - (mt_cons_nscrolls() - si));
						mt_cons_clreom();
						*p = 0;
						mt_cons_puts(buf);
					}
					else
						mt_cons_puts(ERASEBACK);
					break;

				case '\r':
				case '\n':
					mt_cons_puts("\r\n");
					*p++ = '\n';
					*p = 0;
					return p - buf;

				default:
					*p++ = c;
					mt_cons_putc(c);
					break;
			}

	mt_cons_puts("<EOB>\r\n");
	*p = 0;
	return p - buf;
}
