#include "kernel.h"
#include "apps.h"

#define CLOCKIRQ		0				/* interrupcion de timer */
#define MIN_STACK		4096			/* tamaño de stack mínimo */ 
#define INIFL			0x200			/* flags iniciales, IF=1 */
#define MSPERTICK 		20				/* 50 Hz */
#define QUANTUM			2				/* 40 mseg */

Task_t * volatile mt_curr_task;			/* tarea en ejecucion */
Task_t * volatile mt_last_task;			/* tarea anterior */
Task_t * volatile mt_fpu_task;			/* tarea que tiene el coprocesador */
unsigned long long volatile mt_ticks;	/* ticks ocurridos desde el arranque */

static Task_t main_task;				/* tarea principal */
static volatile unsigned ticks_to_run;	/* ranura de tiempo */
static TaskQueue_t ready_q;				/* cola de tareas ready */
static TaskQueue_t terminated_q;		/* cola de tareas terminadas */
static Switcher_t save_restore;			/* cambio de contexto adicional */

static void scheduler(void);

static void block(Task_t *task, TaskState_t state);
static void ready(Task_t *task, bool success);
static void free_task(Task_t *task);

static unsigned msecs_to_ticks(unsigned msecs);
static unsigned ticks_to_msecs(unsigned ticks);

static void free_terminated(void);		/* libera tareas terminadas */
static void do_nothing(void *arg);		/* funcion de la tarea nula */
static void clockint(unsigned irq);		/* manejador interrupcion de timer */

// Stackframe inicial de una tarea
typedef struct
{
	mt_regs_t		regs;
	void			(*retaddr)(void);
	void *			arg;
}
InitialStack_t;

/*
--------------------------------------------------------------------------------
Malloc, StrDup, Free - manejo de memoria dinamica
--------------------------------------------------------------------------------
*/

void *
Malloc(unsigned size)
{
	void *p;

	Atomic();
	free_terminated();
	if ( !(p = malloc(size)) )
		Panic("Error malloc");
	memset(p, 0, size);
	Unatomic();
	return p;
}

char *
StrDup(char *str)
{
	char *p;

	if ( !str )
		return NULL;
	Atomic();
	free_terminated();
	if ( !(p = malloc(strlen(str) + 1)) )
		Panic("Error strdup");
	strcpy(p, str);
	Unatomic();
	return p;
}

void
Free(void *mem)
{
	if ( !mem )
		return;
	Atomic();
	free(mem);
	Unatomic();
}

/*
--------------------------------------------------------------------------------
msecs_to_ticks, ticks_to_msecs - conversion de milisegundos a ticks y viceversa
--------------------------------------------------------------------------------
*/

static unsigned 
msecs_to_ticks(unsigned msecs)
{
	return (msecs + MSPERTICK - 1) / MSPERTICK;
}

static unsigned 
ticks_to_msecs(unsigned ticks)
{
	return ticks * MSPERTICK;
}

/*
--------------------------------------------------------------------------------
block - bloquea una tarea
--------------------------------------------------------------------------------
*/

static void
block(Task_t *task, TaskState_t state)
{
	mt_dequeue(task);
	mt_dequeue_time(task);
	task->state = state;
}

/*
--------------------------------------------------------------------------------
ready - desbloquea una tarea y la pone en la cola de ready

Si la tarea estaba bloqueado en WaitQueue, Send o Receive, el argumento
success determina el status de retorno de la funcion que la bloqueo.
--------------------------------------------------------------------------------
*/

static void
ready(Task_t *task, bool success)
{
	if ( task->state == TaskReady )
		return;

	mt_dequeue(task);
	mt_dequeue_time(task);
	mt_enqueue(task, &ready_q);
	task->success = success;
	task->state = TaskReady;
}

/*
--------------------------------------------------------------------------------
CreateTask - crea una tarea.

Recibe un puntero a una funcion de tipo void f(void*), tamano del stack,
un puntero para pasar como argumento, nombre, prioridad inicial.
Toma memoria para crear el stack y lo inicializa para que retorne
a Exit(). Una vez creada una tarea, hay que comenzar a ejecutarla llamando 
a Ready().
--------------------------------------------------------------------------------
*/

Task_t *
CreateTask(TaskFunc_t func, unsigned stacksize, void *arg, char *name, unsigned priority)
{
	Task_t *task;
	InitialStack_t *s;

	/* alocar bloque de control */
	task = Malloc(sizeof(Task_t));
	task->name = task->send_queue.name = StrDup(name);
	task->priority = priority;

	/* alocar stack */
	stacksize &= ~3;					// redondear a multiplos de 4
	if ( stacksize < MIN_STACK )		// garantizar tamaño mínimo
		stacksize = MIN_STACK;
	task->stack = Malloc(stacksize);	// malloc alinea adecuadamente

	/* inicializar stack */
	s = (InitialStack_t *)(task->stack + stacksize) - 1;
	s->arg = arg;
	s->retaddr = Exit;				/* direccion de retorno de func() */
	s->regs.eflags = INIFL;
	s->regs.eip = (unsigned) func;
	task->esp = (unsigned) s;

	return task;
}

/*
--------------------------------------------------------------------------------
DeleteTask - elimina una tarea creada con CreateTask

Si es la actual, se bloquea en la cola de tareas terminadas.
--------------------------------------------------------------------------------
*/

static void
free_task(Task_t *task)
{
	if ( task->name )
		free(task->name);
	free(task->stack);
	if ( task->math_data )
		free(task->math_data);
	free(task);
}

void
DeleteTask(Task_t *task)
{
	if ( task == &main_task )
		Panic("Imposible eliminar la tarea principal");

	FlushQueue(&task->send_queue, false);
	if ( mt_fpu_task == task )
		mt_fpu_task = NULL;
	DisableInts();
	if ( task == mt_curr_task )
	{
		mt_curr_task->state = TaskTerminated;
		mt_enqueue(mt_curr_task, &terminated_q);
		scheduler();
	}
	else
	{
		block(task, TaskTerminated);
		free_task(task);
	}
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
free_terminated - elimina las tareas terminadas.
--------------------------------------------------------------------------------
*/

void
free_terminated(void)
{
	Task_t *task;

	while ( (task = mt_getlast(&terminated_q)) )
		free_task(task);
}

/*
--------------------------------------------------------------------------------
CurrentTask - retorna un puntero a la tarea actual.
--------------------------------------------------------------------------------
*/

Task_t *
CurrentTask(void)
{
	return mt_curr_task;
}


/*
--------------------------------------------------------------------------------
Panic - error fatal del sistema
--------------------------------------------------------------------------------
*/

void
Panic(char *msg)
{
	DisableInts();
	cprintk(RED, LIGHTGRAY, "\nPANIC [%s]: %s", mt_curr_task->name, msg);
	while ( true )
		;
}

/*
--------------------------------------------------------------------------------
Pause - suspende la tarea actual
--------------------------------------------------------------------------------
*/

void
Pause(void)
{
	Suspend(mt_curr_task);
}

/*
--------------------------------------------------------------------------------
Yield - cede voluntariamente la CPU
--------------------------------------------------------------------------------
*/

void
Yield(void)
{
	Ready(mt_curr_task);
}

/*
--------------------------------------------------------------------------------
Delay - pone a la tarea actual a dormir durante una cantidad de milisegundos
--------------------------------------------------------------------------------
*/

void
Delay(unsigned msecs)
{
	DisableInts();
	if ( msecs )
	{
		block(mt_curr_task, TaskDelaying);
		if ( msecs != FOREVER )
			mt_enqueue_time(mt_curr_task, msecs_to_ticks(msecs));
	}
	else
		ready(mt_curr_task, false);
	scheduler();
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
Exit - finaliza la tarea actual

Todas las tareas creadas con CreateTask retornan a esta funcion que las mata.
Esta funcion nunca retorna.
--------------------------------------------------------------------------------
*/

void
Exit(void)
{
	DeleteTask(mt_curr_task);
}

/*
--------------------------------------------------------------------------------
Suspend - suspende una tarea
--------------------------------------------------------------------------------
*/

void
Suspend(Task_t *task)
{
	DisableInts();
	block(task, TaskSuspended);
	if ( task == mt_curr_task )
		scheduler();
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
Ready - pone una tarea en la cola ready
--------------------------------------------------------------------------------
*/

void
Ready(Task_t *task)
{
	DisableInts();
	ready(task, false);
	scheduler();
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
GetPriority - retorna la prioridad de una tarea
--------------------------------------------------------------------------------
*/

unsigned
GetPriority(Task_t *task)
{
	return task->priority;
}

/*
--------------------------------------------------------------------------------
SetPriority - establece la prioridad de una tarea

Si la tarea estaba en una cola, la desencola y la vuelve a encolar para
reflejar el cambio de prioridad en su posición en la cola.
Si se le ha cambiado la prioridad a la tarea actual o a una que esta ready se
llama al scheduler.
--------------------------------------------------------------------------------
*/

void		
SetPriority(Task_t *task, unsigned priority)
{
	TaskQueue_t *queue;

	DisableInts();
	task->priority = priority;
	if ( (queue = task->queue) )
	{
		mt_dequeue(task);
		mt_enqueue(task, queue);
	}
	if ( task == mt_curr_task || task->state == TaskReady )
		scheduler();
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
SetData - establece un puntero a datos privados de una tarea
--------------------------------------------------------------------------------
*/

void		
SetData(Task_t *task, void *data)
{
	DisableInts();
	task->data = data;
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
SetSwitcher - establece un manejador global de cambio de contexto

Si existe este manejador, esta funcion se llamara inmediatamente antes de cada
cambio de contexto. Recibe un puntero a la tarea que deja la CPU y otro a la
tarea que la recibe. Se ejecuta con interrupciones deshabilitadas y no debe
habilitarlas.
--------------------------------------------------------------------------------
*/

void		
SetSwitcher(Switcher_t switcher)
{
	DisableInts();
	save_restore = switcher;
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
CreateQueue - crea una cola de tareas
--------------------------------------------------------------------------------
*/

TaskQueue_t *	
CreateQueue(char *name)
{
	TaskQueue_t *queue = Malloc(sizeof(TaskQueue_t));

	queue->name = StrDup(name);
	return queue;
}

/*
--------------------------------------------------------------------------------
DeleteQueue - destruye una cola de tareas
--------------------------------------------------------------------------------
*/

void
DeleteQueue(TaskQueue_t *queue)
{
	FlushQueue(queue, false);
	Free(queue->name);
	Free(queue);
}

/*
--------------------------------------------------------------------------------
WaitQueue, WaitQueueCond, WaitQueueTimed - esperar en una cola de tareas

El valor de retorno es true si la tarea fue despertada por SignalQueue
o el valor pasado a FlushQueue.
Si msecs es FOREVER, espera indefinidamente. Si msecs es cero, retorna false.
--------------------------------------------------------------------------------
*/

bool			
WaitQueue(TaskQueue_t *queue)
{
	return WaitQueueTimed(queue, FOREVER);
}

bool			
WaitQueueTimed(TaskQueue_t *queue, unsigned msecs)
{
	bool success;

	if ( !msecs )
		return false;

	DisableInts();
	block(mt_curr_task, TaskWaiting);
	mt_enqueue(mt_curr_task, queue);
	if ( msecs != FOREVER )
		mt_enqueue_time(mt_curr_task, msecs_to_ticks(msecs));
	scheduler();
	success = mt_curr_task->success;
	RestoreInts();

	return success;
}

/*
--------------------------------------------------------------------------------
SignalQueue, FlushQueue - funciones para despertar tareas en una cola

SignalQueue despierta la última tarea de la cola (lal de mayor prioridad o
la que llego primero entre dos de la misma prioridad), el valor de retorno 
es true si desperto a una tarea. Esta tarea completa su WaitQueue() 
exitosamente.
FlushQueue despierta a todas las tareas de la cola, que completan su
WaitQueue() con el resultado que se pasa como argumento.
--------------------------------------------------------------------------------
*/

bool		
SignalQueue(TaskQueue_t *queue)
{
	Task_t *task;

	DisableInts();
	if ( (task = mt_getlast(queue)) )
	{
		ready(task, true);
		scheduler();
	}
	RestoreInts();

	return task != NULL;
}

void			
FlushQueue(TaskQueue_t *queue, bool success)
{
	Task_t *task;

	DisableInts();
	if ( mt_peeklast(queue) )
	{
		while ( (task = mt_getlast(queue)) )
			ready(task, success);
		scheduler();
	}
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
Send, SendCond, SendTimed - enviar un mensaje
--------------------------------------------------------------------------------
*/

bool			
Send(Task_t *to, void *msg, unsigned size)
{
	return SendTimed(to, msg, size, FOREVER);
}

bool			
SendCond(Task_t *to, void *msg, unsigned size)
{
	return SendTimed(to, msg, size, 0);
}

bool			
SendTimed(Task_t *to, void *msg, unsigned size, unsigned msecs)
{
	bool success;

	DisableInts();

	if ( to->state == TaskReceiving && (!to->from || to->from == mt_curr_task) )
	{
		to->from = mt_curr_task;
		if ( to->msg && msg )
		{
			if ( size > to->size )
				Panic("Buffer insuficiente para transmitir mensaje");
			to->size = size;
			memcpy(to->msg, msg, size);
		}
		else
			to->size = 0;
		ready(to, true);
		scheduler();
		RestoreInts();
		return true;
	}

	if ( !msecs )
	{
		RestoreInts();
		return false;
	}

	mt_curr_task->msg = msg;
	mt_curr_task->size = size;
	mt_curr_task->state = TaskSending;
	mt_enqueue(mt_curr_task, &to->send_queue);
	if ( msecs != FOREVER )
		mt_enqueue_time(mt_curr_task, msecs_to_ticks(msecs));
	scheduler();
	success = mt_curr_task->success;

	RestoreInts();
	return success;
}


/*
--------------------------------------------------------------------------------
Receive, ReceiveCond, ReceiveTimed - recibir un mensaje
--------------------------------------------------------------------------------
*/

bool			
Receive(Task_t **from, void *msg, unsigned *size)
{
	return ReceiveTimed(from, msg, size, FOREVER);
}

bool			
ReceiveCond(Task_t **from, void *msg, unsigned *size)
{
	return ReceiveTimed(from, msg, size, 0);
}

bool			
ReceiveTimed(Task_t **from, void *msg, unsigned *size, unsigned msecs)
{
	bool success;
	Task_t *sender;

	DisableInts();

	if ( from && *from )
		sender = (*from)->queue == &mt_curr_task->send_queue ? *from : NULL;
	else
		sender = mt_peeklast(&mt_curr_task->send_queue);

	if ( sender )
	{
		if ( from ) 
			*from = sender;
		if ( sender->msg && msg )
		{
			if ( size )
			{
				if ( sender->size > *size )
					Panic("Buffer insuficiente para recibir mensaje");
				memcpy(msg, sender->msg, *size = sender->size);
			}
		}
		else if ( size )
			*size = 0;
		ready(sender, true);
		scheduler();
		RestoreInts();
		return true;
	}

	if ( !msecs )
	{
		RestoreInts();
		return false;
	}

	mt_curr_task->from = from ? *from : NULL;
	mt_curr_task->msg = msg;
	mt_curr_task->size = size ? *size : 0;
	mt_curr_task->state = TaskReceiving;
	if ( msecs != FOREVER )
		mt_enqueue_time(mt_curr_task, msecs_to_ticks(msecs));
	scheduler();
	if ( (success = mt_curr_task->success) )
	{
		if ( size )
			*size = mt_curr_task->size;
		if ( from )
			*from = mt_curr_task->from;
	}

	RestoreInts();
	return success;
}

/*
--------------------------------------------------------------------------------
mt_select_task - determina la próxima tarea a ejecutar.

Retorna true si ha cambiado la tarea en ejecucion.
Llamada desde scheduler() y cuanto retorna una interrupcion de primer nivel.
Si la tarea actual no es dueña del coprocesador, levanta el bit TS en CR0 para que 
se genere la excepción 7 la próxima vez que se ejecute una instrucción de 
coprocesador.
Guarda y restaura el contexto propio del usuario, si existe. 
--------------------------------------------------------------------------------
*/

bool 
mt_select_task(void)
{
	Task_t *ready_task;

	/* Ver si la tarea actual puede conservar la CPU */
	if ( mt_curr_task->state == TaskCurrent )
	{
		if ( mt_curr_task->atomic_level )		/* No molestar */
			return false;

		/* Analizar prioridades y ranura de tiempo */
		ready_task = mt_peeklast(&ready_q);
		if ( !ready_task || ready_task->priority < mt_curr_task->priority ||
			(ticks_to_run && ready_task->priority == mt_curr_task->priority) )
			return false; 

		/* La tarea actual pierde la CPU */
		ready(mt_curr_task, false);
	}

	/* Obtener la próxima tarea */
	mt_last_task = mt_curr_task;
	mt_curr_task = mt_getlast(&ready_q);
	mt_curr_task->state = TaskCurrent;

	/* Si es la misma de antes, no hay nada mas que hacer */
	if ( mt_curr_task == mt_last_task )
		return false;

	/* Si la tarea actual es dueña del coprocesador aritmético,
	   bajar el bit TS en CR0. En caso contrario, levantarlo para que
	   la próxima instrucción de coprocesador genere una excepción 7 */
	if ( mt_curr_task == mt_fpu_task )
		mt_clts();
	else
		mt_stts();

	/* Guardar/reponer contexto propio del usuario */
	if ( save_restore )
		save_restore(mt_last_task, mt_curr_task);

	/* Inicializar ranura de tiempo */
	ticks_to_run = QUANTUM;
	return true;
}

/*
--------------------------------------------------------------------------------
scheduler - selecciona la próxima tarea a ejecutar.

Se llama cuando se bloquea la tarea actual o se despierta cualquier tarea.
No hace nada si se llama desde una interrupcion, porque las interrupciones
pueden despertar tareas pero recien se cambia contexto al retornar de la
interrupcion de primer nivel.
--------------------------------------------------------------------------------
*/

static void
scheduler(void)
{
	if ( !mt_int_level && mt_select_task() )
		mt_context_switch();
}

/*
--------------------------------------------------------------------------------
clockint - interrupcion de tiempo real

Despierta a las tareas de la cola de tiempo que tengan su cuenta de ticks
agotada, y decrementa la cuenta de la primera que quede en la cola.
Decrementa la ranura de tiempo de la tarea actual.
--------------------------------------------------------------------------------
*/

static void 
clockint(unsigned irq)
{
	Task_t *task;

	++mt_ticks;

	if ( ticks_to_run )
		ticks_to_run--;
	while ( (task = mt_peekfirst_time()) && !task->ticks )
	{
		mt_getfirst_time();
		ready(task, false);
	}
	if ( task )
		task->ticks--;
}

/*
--------------------------------------------------------------------------------
Atomic - deshabilita el modo preemptivo para la tarea actual (anidable)
--------------------------------------------------------------------------------
*/

void
Atomic(void)
{
	++mt_curr_task->atomic_level;
}

/*
--------------------------------------------------------------------------------
Unatomic - habilita el modo preemptivo para la tarea actual (anidable)
--------------------------------------------------------------------------------
*/

void
Unatomic(void)
{
	if ( mt_curr_task->atomic_level && !--mt_curr_task->atomic_level )
	{
		DisableInts();
		scheduler();
		RestoreInts();
	}
}

/*
--------------------------------------------------------------------------------
DisableInts - deshabilita interrupciones para la tarea actual (anidable)
--------------------------------------------------------------------------------
*/

void
DisableInts(void)
{
	if ( !mt_curr_task->disint_level++ )
		mt_cli();
}

/*
--------------------------------------------------------------------------------
RestoreInts - habilita interrupciones para la tarea actual (anidable)
--------------------------------------------------------------------------------
*/

void
RestoreInts(void)
{
	if ( mt_curr_task->disint_level && !--mt_curr_task->disint_level )
		mt_sti();
}

/*
--------------------------------------------------------------------------------
do_nothing - Tarea nula

Corre con prioridad 0 y toma la CPU cuando ninguna otra tarea pueda ejecutar.
--------------------------------------------------------------------------------
*/

static void
do_nothing(void *arg)
{
	while ( true )
		;
}

/*
--------------------------------------------------------------------------------
	mt_main - Inicializacion del kernel

	El control viene aquí inmediatamente después del arranque en kstart.asm.
	Las interrupciones están deshabilitadas, puede usarse el stack y los
	registros de segmento CS y DS apuntan a memoria plana con base 0, pueden 
	usarse pero	no cargarse, ni siquiera con los mismos valores que tienen. 
	Lo primero que hay que hacer es poner una GDT e inicializar los registros 
	de segmento.
--------------------------------------------------------------------------------
*/

void
mt_main(void)
{
	// Inicializar GDT e IDT
	mt_setup_gdt_idt();

	// Inicializar sistema de interrupciones
	mt_setup_interrupts();

	// Configurar el timer, colocar el manejador de interrupción
	// correspondiente y habilitar la interrupción
	mt_setup_timer(MSPERTICK);
	mt_set_int_handler(CLOCKIRQ, clockint);
	mt_enable_irq(CLOCKIRQ);

	// Inicializar el sistema de manejo del coprocesador aritmético
	mt_setup_math();

	// Inicializar tarea principal
	main_task.name = "Main Task";
	main_task.state = TaskCurrent;
	main_task.priority = DEFAULT_PRIO;
	main_task.send_queue.name = main_task.name;
	mt_curr_task = &main_task;
	ticks_to_run = QUANTUM;

	// Crear tarea nula y ponerla ready 
	ready(CreateTask(do_nothing, 0, NULL, "Null Task", MIN_PRIO), false);

	// Habilitar interrupciones
	mt_sti();

	// Borrar la pantalla
	mt_cons_clear();
	mt_cons_cursor(true);

	// Inicializar driver de teclado
	mt_kbd_init();

	// Ejecutar primera tarea
	while ( true )
	{
		cprintk(LIGHTCYAN, BLACK, "MTask inicializado.\n");
		char *arg[] = { "shell", NULL };
		shell_main(1, arg);
	}
}
