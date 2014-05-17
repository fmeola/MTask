#ifndef KERNEL_H_INCLUDED
#define KERNEL_H_INCLUDED

#include "mtask.h"
#include "segments.h"

/* gdt_idt.c */

void mt_setup_gdt_idt(void);

/* interrupts.asm */

// Excepciones 0-31, interrupciones de HW 32-47
#define INT_STUB_SIZE 16
#define NUM_INTS 48
#define NUM_EXCEPT 32

typedef char int_stub[INT_STUB_SIZE];
extern int_stub mt_int_stubs[NUM_INTS];

/* libasm.asm */

void mt_load_gdt(const region_desc *gdt, unsigned cs, unsigned ds);
void mt_load_idt(const region_desc *idt);
void mt_context_switch(void);
void mt_sti(void);
void mt_cli(void);
void mt_finit(void);
void mt_fsave(void *buf);
void mt_frstor(void *buf);
void mt_stts(void);
void mt_clts(void);

/* kernel.c */

extern Task_t * volatile mt_curr_task;
extern Task_t * volatile mt_last_task;
extern Task_t * volatile mt_fpu_task;
extern unsigned long long volatile mt_ticks;
void mt_main(void);
bool mt_select_task(void);

/* irq.c */

// Registros empujados al stack por una interrupción o excepción.
// Esta estructura debe mantenerse sincronizada con el código de resguardo y
// recuperación de contexto en el manejador de interrupciones (interrupts.asm)
// y con la función mt_context_switch() (libasm.asm).
typedef struct
{
	unsigned ebp;
	unsigned edi;
	unsigned esi;
	unsigned edx;
	unsigned ecx;
	unsigned ebx;
	unsigned eax;
	unsigned eflags;
	unsigned eip;
}
mt_regs_t;

extern unsigned mt_int_level;
void mt_int_handler(unsigned int_num, unsigned except_error, mt_regs_t *regs);

typedef void (*exception_handler)(unsigned except_number, unsigned error, mt_regs_t *regs);
typedef void (*interrupt_handler)(unsigned irq_number);

void mt_setup_interrupts(void);
void mt_set_int_handler(unsigned irq_num, interrupt_handler handler);
void mt_set_exception_handler(unsigned except_num, exception_handler handler);
void mt_enable_irq(unsigned irq);
void mt_disable_irq(unsigned irq);

/* cons.c */

enum COLORS
{
	/* oscuros */
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LIGHTGRAY,

	/* claros */
	DARKGRAY,
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};

void mt_cons_clear(void);
void mt_cons_clreol(void);
void mt_cons_clreom(void);

unsigned mt_cons_nrows(void);
unsigned mt_cons_ncols(void);
unsigned mt_cons_nscrolls(void);
void mt_cons_getxy(unsigned *x, unsigned *y);
void mt_cons_gotoxy(unsigned x, unsigned y);

void mt_cons_setattr(unsigned fg, unsigned bg);
void mt_cons_getattr(unsigned *fg, unsigned *bg);
bool mt_cons_cursor(bool on);

bool mt_cons_raw(bool on);
void mt_cons_putc(char ch);
void mt_cons_puts(const char *str);

void mt_cons_cr(void);
void mt_cons_nl(void);
void mt_cons_tab(void);
void mt_cons_bs(void);

/* keyboard.c */ 

void mt_kbd_init(void);
bool mt_kbd_getch(unsigned *c);
bool mt_kbd_getch_timed(unsigned *c, unsigned timeout);
const char *mt_kbd_getlayout(void);
bool mt_kbd_setlayout(const char *name);
const char **mt_kbd_layouts(void);

/* getline.c */

unsigned mt_getline(char *buf, unsigned size);

/* timer.c */

void mt_setup_timer(unsigned freq);

/* queue.c */

void mt_enqueue(Task_t *task, TaskQueue_t *queue);
void mt_dequeue(Task_t *task);
Task_t *mt_peeklast(TaskQueue_t *queue);
Task_t *mt_getlast(TaskQueue_t *queue);

void mt_enqueue_time(Task_t *task, unsigned ticks);
void mt_dequeue_time(Task_t *task);
Task_t *mt_peekfirst_time(void);
Task_t *mt_getfirst_time(void);

/* math.c */

void mt_setup_math(void);

#endif
