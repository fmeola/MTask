#include "kernel.h"

/*
	Trabajamos en modo flat y en ring 0.
	Utilizamos una GDT con dos segmentos, uno de código (CS = 0x8) y 
	otro de	datos (DS, ES, FS, GS y SS = 0x10). No usamos LDT.
	Ambos segmentos empiezan en 0 y abarcan toda la memoria (4 GB).
	La idea es inicializar los registros de segmento una vez y después no 
	tocarlos nunca más.
*/

static segment_desc gdt[] = 
{
	{
		/* Primer descriptor nulo */
	},
	{
		/* Segmento de código, selector 0x8 */
		.type = DESC_MEMER, .dpl = 0, .present = 1, .bits32 = 1, .gran = 1,
		.base_low = 0, .base_high = 0,
		.limit_low = 0xFFFF, .limit_high = 0xF
	},
	{
		/* Segmento de datos, selector 0x10 */
		.type = DESC_MEMRW, .dpl = 0, .present = 1, .bits32 = 1, .gran = 1,
		.base_low = 0, .base_high = 0,
		.limit_low = 0xFFFF, .limit_high = 0xF
	}
};

/* Inicializar la GDT */
static void
setup_gdt(void)
{
	region_desc gdtr;

	gdtr.base = (unsigned) gdt;
	gdtr.limit = sizeof gdt - 1;

	/* Cargar GDTR e inicializar los registros de segmentos */
	mt_load_gdt(&gdtr, 0x8, 0x10);
}

static gate_desc idt[NUM_INTS];

static void setup_idt(void)
{
	unsigned i;
	int_stub *sptr;
	gate_desc *dptr;
	region_desc idtr;

	/* Inicializar las entradas de la IDT con los stubs de interrupción */
	for ( i = 0, sptr = mt_int_stubs, dptr = idt ; i < NUM_INTS ; i++, sptr++, dptr++ )
	{
		dptr->type = DESC_INTGT;
		dptr->present = 1;
		dptr->selector = 0x8;
		dptr->offset_low = ((unsigned) sptr) & 0xFFFF;
		dptr->offset_high = ((unsigned) sptr) >> 16;
	}

	idtr.base = (unsigned) idt;
	idtr.limit = sizeof idt - 1;

	/* Cargar IDTR */
	mt_load_idt(&idtr);
}

void mt_setup_gdt_idt(void)
{
	setup_gdt();
	setup_idt();
}
