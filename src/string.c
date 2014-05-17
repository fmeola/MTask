/* Adaptado de los fuentes de uClinux, adaptado a su vez de Linux. */

/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		NO Copyright (C) 1991, 1992 Linus Torvalds,
 *		consider these trivial functions to be PD.
 *
 * Modified for uClibc by Erik Andersen <andersen@codepoet.org>
 * These make no attempt to use nifty things like mmx/3dnow/etc.
 * These are not inline, and will therefore not be as fast as
 * modifying the headers to use inlines (and cannot therefore
 * do tricky things when dealing with const memory).  But they
 * should (I hope!) be faster than their generic equivalents....
 *
 * More importantly, these should provide a good example for
 * others to follow when adding arch specific optimizations.
 *  -Erik
 */

#include "kernel.h"

char *strcpy(char *dest, const char *src)
{
    int d0, d1, d2;
    __asm__ __volatile__(
	    "1:\tlodsb\n\t"
	    "stosb\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b"
	    : "=&S" (d0), "=&D" (d1), "=&a" (d2)
	    :"0" (src),"1" (dest) : "memory");
    return dest;
}

char *strncpy(char *dest, const char *src, unsigned count)
{
    int d0, d1, d2, d3;
    __asm__ __volatile__(
	    "1:\tdecl %2\n\t"
	    "js 2f\n\t"
	    "lodsb\n\t"
	    "stosb\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b\n\t"
	    "rep\n\t"
	    "stosb\n"
	    "2:"
	    : "=&S" (d0), "=&D" (d1), "=&c" (d2), "=&a" (d3)
	    :"0" (src),"1" (dest),"2" (count) : "memory");
    return dest;
}

char *strcat(char *dest, const char *src)
{
    int d0, d1, d2, d3;
    __asm__ __volatile__(
	    "repne\n\t"
	    "scasb\n\t"
	    "decl %1\n"
	    "1:\tlodsb\n\t"
	    "stosb\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b"
	    : "=&S" (d0), "=&D" (d1), "=&a" (d2), "=&c" (d3)
	    : "0" (src), "1" (dest), "2" (0), "3" (0xffffffff):"memory");
    return dest;
}

char *strncat(char *dest, const char *src, unsigned count)
{
    int d0, d1, d2, d3;
    __asm__ __volatile__(
	    "repne\n\t"
	    "scasb\n\t"
	    "decl %1\n\t"
	    "movl %8,%3\n"
	    "1:\tdecl %3\n\t"
	    "js 2f\n\t"
	    "lodsb\n\t"
	    "stosb\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b\n"
	    "2:\txorl %2,%2\n\t"
	    "stosb"
	    : "=&S" (d0), "=&D" (d1), "=&a" (d2), "=&c" (d3)
	    : "0" (src),"1" (dest),"2" (0),"3" (0xffffffff), "g" (count)
	    : "memory");
    return dest;
}

int strcmp(const char *cs, const char *ct)
{
    int d0, d1;
    register int __res;
    __asm__ __volatile__(
	    "1:\tlodsb\n\t"
	    "scasb\n\t"
	    "jne 2f\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b\n\t"
	    "xorl %%eax,%%eax\n\t"
	    "jmp 3f\n"
	    "2:\tsbbl %%eax,%%eax\n\t"
	    "orb $1,%%al\n"
	    "3:"
	    :"=a" (__res), "=&S" (d0), "=&D" (d1)
	    :"1" (cs),"2" (ct));
    return __res;
}

int strncmp(const char *cs, const char *ct, unsigned count)
{
    register int __res;
    int d0, d1, d2;
    __asm__ __volatile__(
	    "1:\tdecl %3\n\t"
	    "js 2f\n\t"
	    "lodsb\n\t"
	    "scasb\n\t"
	    "jne 3f\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b\n"
	    "2:\txorl %%eax,%%eax\n\t"
	    "jmp 4f\n"
	    "3:\tsbbl %%eax,%%eax\n\t"
	    "orb $1,%%al\n"
	    "4:"
	    :"=a" (__res), "=&S" (d0), "=&D" (d1), "=&c" (d2)
	    :"1" (cs),"2" (ct),"3" (count));
    return __res;
}

char *strchr(const char *s, int c)
{
    int d0;
    register char * __res;
    __asm__ __volatile__(
	    "movb %%al,%%ah\n"
	    "1:\tlodsb\n\t"
	    "cmpb %%ah,%%al\n\t"
	    "je 2f\n\t"
	    "testb %%al,%%al\n\t"
	    "jne 1b\n\t"
	    "movl $1,%1\n"
	    "2:\tmovl %1,%0\n\t"
	    "decl %0"
	    :"=a" (__res), "=&S" (d0) : "1" (s),"0" (c));
    return __res;
}

char *strrchr(const char *s, int c)
{
    int d0, d1;
    register char * __res;
    __asm__ __volatile__(
	    "movb %%al,%%ah\n"
	    "1:\tlodsb\n\t"
	    "cmpb %%ah,%%al\n\t"
	    "jne 2f\n\t"
	    "leal -1(%%esi),%0\n"
	    "2:\ttestb %%al,%%al\n\t"
	    "jne 1b"
	    :"=g" (__res), "=&S" (d0), "=&a" (d1) :"0" (0),"1" (s),"2" (c));
    return __res;
}

unsigned strlen(const char *s)
{
    int d0;
    register int __res;
    __asm__ __volatile__(
	    "repne\n\t"
	    "scasb\n\t"
	    "notl %0\n\t"
	    "decl %0"
	    :"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffff));
    return __res;
}

unsigned strnlen(const char *s, unsigned count)
{
    int d0;
    register int __res;
    __asm__ __volatile__(
	    "movl %2,%0\n\t"
	    "jmp 2f\n"
	    "1:\tcmpb $0,(%0)\n\t"
	    "je 3f\n\t"
	    "incl %0\n"
	    "2:\tdecl %1\n\t"
	    "cmpl $-1,%1\n\t"
	    "jne 1b\n"
	    "3:\tsubl %2,%0"
	    :"=a" (__res), "=&d" (d0)
	    :"c" (s),"1" (count));
    return __res;
}

void *memcpy(void *to, const void *from, unsigned n)
{
    int d0, d1, d2;
    __asm__ __volatile__(
	    "rep ; movsl\n\t"
	    "testb $2,%b4\n\t"
	    "je 1f\n\t"
	    "movsw\n"
	    "1:\ttestb $1,%b4\n\t"
	    "je 2f\n\t"
	    "movsb\n"
	    "2:"
	    : "=&c" (d0), "=&D" (d1), "=&S" (d2)
	    :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
	    : "memory");
    return (to);
}

void *memmove(void *dest, const void *src, unsigned n)
{
    int d0, d1, d2;
    if (dest<src)
	__asm__ __volatile__(
		"rep\n\t"
		"movsb"
		: "=&c" (d0), "=&S" (d1), "=&D" (d2)
		:"0" (n),"1" (src),"2" (dest)
		: "memory");
    else
	__asm__ __volatile__(
		"std\n\t"
		"rep\n\t"
		"movsb\n\t"
		"cld"
		: "=&c" (d0), "=&S" (d1), "=&D" (d2)
		:"0" (n),
		"1" (n-1+(const char *)src),
		"2" (n-1+(char *)dest)
		:"memory");
    return dest;
}

void *memchr(const void *cs, int c, unsigned count)
{
    int d0;
    register void * __res;
    if (!count)
	return 0;
    __asm__ __volatile__(
	    "repne\n\t"
	    "scasb\n\t"
	    "je 1f\n\t"
	    "movl $1,%0\n"
	    "1:\tdecl %0"
	    :"=D" (__res), "=&c" (d0) : "a" (c),"0" (cs),"1" (count));
    return __res;
}

void *memset(void *s, int c, unsigned count)
{
    int d0, d1;
    __asm__ __volatile__(
	    "rep\n\t"
	    "stosb"
	    : "=&c" (d0), "=&D" (d1)
	    :"a" (c),"1" (s),"0" (count)
	    :"memory");
    return s;
}
