#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED

#include <sys/types.h>
#include <stdarg.h>

/* io.asm */

unsigned inb(unsigned ioaddr);
void outb(unsigned ioaddr, unsigned data);

/* string.c */

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, unsigned count);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, unsigned count);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, unsigned count);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
unsigned strlen(const char *s);
unsigned strnlen(const char *s, unsigned count);
void *memcpy(void *to, const void *from, unsigned n);
void *memmove(void *dest, const void *src, unsigned n);
void *memchr(const void *cs, int c, unsigned count);
void *memset(void *s, int c, unsigned count);

/* rand.c */

int rand(void);
void srand(unsigned seed);

/* sprintf.c */

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

/* printk.c */

int vprintk(const char *fmt, va_list args);
int printk(const char *fmt, ...);
void cprintk(unsigned fg, unsigned bg, char *fmt, ...);

/* malloc.c */

void *malloc(unsigned nbytes);
void free(void *ap);

/* split.c */

const char *setfs(const char *fs);
unsigned split(char *s, char *field[], unsigned nfields);
unsigned separate(char *s, char *field[], unsigned nfields);

/* atoi.c **/

int atoi(const char *s);


#endif
