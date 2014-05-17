// Adaptado ligeramente del libro "El lenguaje de programación C" 
// de Kernighan y Ritchie

#include "kernel.h"

typedef union header
{
	struct
	{
		union header *ptr;				/* next block if on free list */
		unsigned size;					/* size of this block */
	};
	double align;						/* forzar alineacion a 8 bytes */
}
Header;

/* Nuestro heap es un buffer estático */
#define HEAPSIZE 0x800000				/* 8 MB, debe ser múltiplo de 8 */
static Header heap[HEAPSIZE / sizeof(Header)];
static Header base;
static Header *freep;

/* free: put block ap in free list */
void
free(void *ap)
{
	Header *bp, *p;

	bp = (Header *) ap - 1;				/* point to block header */
	for (p = freep; !(bp > p && bp < p->ptr); p = p->ptr)
		if (p >= p->ptr && (bp > p || bp < p->ptr))
			break;						/* freed block at start or end of arena */
	if (bp + bp->size == p->ptr) 		/* join to upper nbr */
	{
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	}
	else
		bp->ptr = p->ptr;
	if (p + p->size == bp) 				/* join to lower nbr */
	{
		p->size += bp->size;
		p->ptr = bp->ptr;
	}
	else
		p->ptr = bp;
	freep = p;
}

/* morecore: ask system for more memory */
/* Como nuestro heap es un buffer estático, esta versión aloca todo el heap 
   la primera vez y fracasa en los llamados sucesivos */
static Header *
morecore(unsigned nu)
{
	static Header *up;
	
	if ( up )
		return 0;
	up = heap;
	up->size = HEAPSIZE / sizeof(Header);
	free(up + 1);
	return freep;
}

void *
malloc(unsigned nbytes)
{
	Header *p, *prevp;
	unsigned nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

	if ((prevp = freep) == 0) 			/* no free list yet */
	{
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}
	for (p = prevp->ptr ; ; prevp = p, p = p->ptr)
	{
		if (p->size >= nunits)			/* big enough */
		{
			if (p->size == nunits)		/* exactly */
				prevp->ptr = p->ptr;
			else						/* allocate tail end */
			{
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}
			freep = prevp;
			return (void *)(p + 1);
		}
		if (p == freep)					/* wrapped around free list */
			if ((p = morecore(nunits)) == 0)
				return 0;				/* none left */
	}
}

