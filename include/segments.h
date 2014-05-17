#ifndef SEGMENTS_H_INCLUDED
#define SEGMENTS_H_INCLUDED

/*
	Descriptores de segmentos y compuertas para i386 
*/

#pragma pack(push, 1)

/* Descriptores de segmentos de memoria */
typedef struct
{
	unsigned limit_low:16;			/* limite del segmento (parte baja) */
	unsigned base_low:24;			/* direccion base (parte baja) */
	unsigned type:5;				/* tipo */
	unsigned dpl:2;					/* nivel de prioridad */
	unsigned present:1;					/* segmento presente */
	unsigned limit_high:4;			/* limite del segmento (parte alta) */
	unsigned reserved:2;			/* sin usar */
	unsigned bits32:1;				/* 32 bits en vez de 16 */
	unsigned gran:1;				/* granularidad (limite en paginas) */
	unsigned base_high:8;			/* direccion base (parte alta) */
} 
segment_desc;

/* Tipos */
#define	DESC_MEMRO	16				/* memory read only */
#define	DESC_MEMRW	18				/* memory read write */
#define	DESC_MEMROD	20				/* memory read only grow down */
#define	DESC_MEMRWD	22				/* memory read write grow down */
#define	DESC_MEME	24				/* memory execute only */
#define	DESC_MEMER	26				/* memory execute read */
#define	DESC_MEMEC	28				/* memory execute only conforming */
#define	DESC_MEMERC	30				/* memory execute read conforming */

/* Descriptores de segmentos de sistema y compuertas */
typedef struct
{
	unsigned offset_low:16;			/* offset(parte baja) */ 
	unsigned selector:16;			/* selector de segmento */
	unsigned copy_words:5;			/* words del stack a copiar */
	unsigned reserved:3;			/* sin usar */
	unsigned type:5;				/* tipo */
	unsigned dpl:2;					/* nivel de privilegio */
	unsigned present:1;				/* segmento presente */
	unsigned offset_high:16;		/* offset (parte alta) */
}
gate_desc;

/* Tipos */
#define	DESC_LDT	 2				/* local descriptor table */
#define	DESC_TASKGT	 5				/* task gate */
#define	DESC_TSS	 9				/* TSS */
#define	DESC_CALLGT	12				/* call gate */
#define	DESC_INTGT	14				/* interrupt gate */
#define	DESC_TRAPGT	15				/* trap gate */

/* Descriptores de GDT e IDT */
typedef struct
{
	unsigned limit:16;				/* limite del segmento */
	unsigned base:32;				/* direccion base */
}
region_desc;

#pragma pack(pop)

#endif
