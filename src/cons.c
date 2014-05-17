#include "kernel.h"

#define VIDMEM 0xB8000
#define NUMROWS 25
#define NUMCOLS 80
#define TABSIZE 8

#define CRT_ADDR 0x3D4
#define CRT_DATA 0x3D5
#define CRT_CURSOR_START 0x0A
#define CRT_CURSOR_END 0x0B
#define CRT_CURSOR_HIGH 0x0E
#define CRT_CURSOR_LOW 0x0F

#define DEFATTR ((BLACK << 12) | (LIGHTGRAY << 8))

#define BS 0x08

typedef unsigned short row[NUMCOLS];
static row *vidmem = (row *) VIDMEM;
static unsigned cur_x, cur_y, cur_attr = DEFATTR;
static bool cursor_on = true;
static bool raw;
static unsigned scrolls;

static void
setcursor(void)
{
	if (cursor_on)
	{
		unsigned off = cur_y * NUMCOLS + cur_x;
		outb(CRT_ADDR, CRT_CURSOR_HIGH);
		outb(CRT_DATA, off >> 8);
		outb(CRT_ADDR, CRT_CURSOR_LOW);
		outb(CRT_DATA, off);
	}
}

static void
scroll(void)
{
	int j;

	for (j = 1; j < NUMROWS; j++)
		memcpy(&vidmem[j - 1], &vidmem[j], sizeof(row));
	for (j = 0; j < NUMCOLS; j++)
		vidmem[NUMROWS - 1][j] = DEFATTR;
	scrolls++;
}

static void
put(unsigned char ch)
{
	vidmem[cur_y][cur_x++] = (ch & 0xFF) | cur_attr;
	if (cur_x >= NUMCOLS)
	{
		cur_x = 0;
		if (cur_y == NUMROWS - 1)
			scroll();
		else
			cur_y++;
	}
	setcursor();
}

/* Interfaz pública */

void
mt_cons_clear(void)
{
	unsigned short *p1 = &vidmem[0][0];
	unsigned short *p2 = &vidmem[NUMROWS][0];

	while (p1 < p2)
		*p1++ = DEFATTR;
	mt_cons_gotoxy(0, 0);
}

void
mt_cons_clreol(void)
{
	unsigned short *p1 = &vidmem[cur_y][cur_x];
	unsigned short *p2 = &vidmem[cur_y + 1][0];

	while (p1 < p2)
		*p1++ = DEFATTR;
}

void
mt_cons_clreom(void)
{
	unsigned short *p1 = &vidmem[cur_y][cur_x];
	unsigned short *p2 = &vidmem[NUMROWS][0];

	while (p1 < p2)
		*p1++ = DEFATTR;
}

unsigned
mt_cons_nrows(void)
{
	return NUMROWS;
}

unsigned
mt_cons_ncols(void)
{
	return NUMCOLS;
}

unsigned
mt_cons_nscrolls(void)
{
	return scrolls;
}

void
mt_cons_getxy(unsigned *x, unsigned *y)
{
	*x = cur_x;
	*y = cur_y;
}

void
mt_cons_gotoxy(unsigned x, unsigned y)
{
	if (y < NUMROWS && x < NUMCOLS)
	{
		cur_x = x;
		cur_y = y;
		setcursor();
	}
}

void
mt_cons_setattr(unsigned fg, unsigned bg)
{
	cur_attr = ((fg & 0xF) << 8) | ((bg & 0xF) << 12);
}

void
mt_cons_getattr(unsigned *fg, unsigned *bg)
{
	*fg = (cur_attr >> 8) & 0xF;
	*bg = (cur_attr >> 12) & 0xF;
}

bool
mt_cons_cursor(bool on)
{
	bool prev = cursor_on;
	unsigned start = on ? 14 : 1, end = on ? 15 : 0;

	outb(CRT_ADDR, CRT_CURSOR_START);
	outb(CRT_DATA, start);
	outb(CRT_ADDR, CRT_CURSOR_END);
	outb(CRT_DATA, end);
	cursor_on = on;
	setcursor();
	return prev;
}

void
mt_cons_putc(char ch)
{
	if (raw)
	{
		put(ch);
		return;
	}

	switch (ch)
	{
		case '\t':
			mt_cons_tab();
			break;

		case '\r':
			mt_cons_cr();
			break;

		case '\n':
			mt_cons_nl();
			break;

		case BS:
			mt_cons_bs();
			break;

		default:
			put(ch);
			break;
	}
}

void
mt_cons_puts(const char *str)
{
	while (*str)
		mt_cons_putc(*str++);
}

void
mt_cons_cr(void)
{
	cur_x = 0;
	setcursor();
}

void
mt_cons_nl(void)
{
	if (cur_y == NUMROWS - 1)
		scroll();
	else
		cur_y++;
	setcursor();
}

void
mt_cons_tab(void)
{
	unsigned nspace = TABSIZE - (cur_x % TABSIZE);
	while (nspace--)
		put(' ');
}

void
mt_cons_bs(void)
{
	if (cur_x)
		cur_x--;
	else if (cur_y)
	{
		cur_y--;
		cur_x = NUMCOLS - 1;
	}
	setcursor();
}

bool
mt_cons_raw(bool on)
{
	bool prev = raw;
	raw = on;
	return prev;
}
