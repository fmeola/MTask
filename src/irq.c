#include "kernel.h"

#define MASTER		0x20				// PIC maestro, registro base
#define SLAVE		0xA0				// PIC esclavo, registro base
#define CTL(pic)	((pic)+1)			// Registro de control del PIC
#define BIT(n)		(1 << ((n) & 0x7))	// Máscara para IRQ n

#define ICW1        0x11				// 4 ICWs, modo cascada, por flanco
#define ICW2_MASTER NUM_EXCEPT			// Primeras 8 interrupciones (32-39)
#define ICW2_SLAVE  (NUM_EXCEPT+8)		// Segundas 8 interrupciones (40-47)
#define ICW3_MASTER 0x04 				// Esclavo en IRQ2 del maestro
#define ICW3_SLAVE  0x02 				// Esclavo en IRQ2 del maestro
#define ICW4        0x01 				// Modo 8086

unsigned mt_int_level;

static void 
setup_pics(void)
{
	// Maestro
	outb(MASTER, ICW1);
	outb(CTL(MASTER), ICW2_MASTER);
	outb(CTL(MASTER), ICW3_MASTER);
	outb(CTL(MASTER), ICW4);
	outb(CTL(MASTER), 0xFB);			// Deshabilitar todas menos la 2

	// Esclavo
	outb(SLAVE, ICW1);
	outb(CTL(SLAVE), ICW2_SLAVE);
	outb(CTL(SLAVE), ICW3_SLAVE);
	outb(CTL(SLAVE), ICW4);
	outb(CTL(SLAVE), 0xFF);				// Deshabilitar todas
}

static void 
eoi(unsigned irq)
{
	unsigned command = 0x60 | (irq & 0x7);

	if (irq < 8)
		outb(MASTER, command);
	else
	{
		outb(SLAVE, command);
		outb(MASTER, 0x62);
	}
}

static exception_handler exception[NUM_EXCEPT];
static interrupt_handler interrupt[NUM_INTS-NUM_EXCEPT];

static void 
unhandled_exception(unsigned num, unsigned error, mt_regs_t *regs)
{
	mt_cons_setattr(RED, LIGHTGRAY);
	printk("\nExcepcion no manejada %d, error %d", num, error);
	printk("\nProceso actual: %s", mt_curr_task->name);
	while ( true )
		;
}

static void 
unhandled_interrupt(unsigned num)
{
	mt_cons_setattr(RED, LIGHTGRAY);
	printk("\nInterrupcion %d no manejada", num);
	while ( true )
		;
}

void
mt_int_handler(unsigned int_number, unsigned except_error, mt_regs_t *regs)
{
	if ( int_number < NUM_EXCEPT )	// Excepción
		exception[int_number](int_number, except_error, regs);
	else							// Interrupción	de HW
	{
		int_number -= NUM_EXCEPT;	// Nro. de irq
		interrupt[int_number](int_number);
		eoi(int_number);
	}
}

void
mt_setup_interrupts(void)
{	
	unsigned i;

	for ( i = 0 ; i < NUM_EXCEPT ; i++ )
		exception[i] = unhandled_exception;

	for ( i = 0 ; i < NUM_INTS-NUM_EXCEPT ; i++ )
		interrupt[i] = unhandled_interrupt;

	setup_pics();
}

void
mt_set_int_handler(unsigned irq_num, interrupt_handler handler)
{
	interrupt[irq_num] = handler ? handler : unhandled_interrupt;
}

void 
mt_set_exception_handler(unsigned except_num, exception_handler handler)
{
	exception[except_num] = handler ? handler : unhandled_exception;
}

void
mt_disable_irq(unsigned irq)
{
	DisableInts();
	if ( irq <= 7 )
		outb(CTL(MASTER), inb(CTL(MASTER)) | BIT(irq));
	else
		outb(CTL(SLAVE), inb(CTL(SLAVE)) | BIT(irq));
	RestoreInts();
}

void
mt_enable_irq(unsigned irq)
{
	DisableInts();
	if ( irq <= 7 )
		outb(CTL(MASTER), inb(CTL(MASTER)) & ~BIT(irq));
	else
		outb(CTL(SLAVE), inb(CTL(SLAVE)) & ~BIT(irq));
	RestoreInts();
}


