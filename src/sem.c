#include "kernel.h"

/*
--------------------------------------------------------------------------------
CreateSem - aloca un semaforo y establece su cuenta inicial
--------------------------------------------------------------------------------
*/

Semaphore_t *CreateSem(char *name, unsigned value)
{
	Semaphore_t *sem = Malloc(sizeof(Semaphore_t));

	sem->queue = CreateQueue(name);
	sem->value = value;
	return sem;
}

/*
--------------------------------------------------------------------------------
DeleteSem - da de baja un semaforo
--------------------------------------------------------------------------------
*/

void
DeleteSem(Semaphore_t *sem)
{
	DeleteQueue(sem->queue);
	Free(sem);
}

/*
--------------------------------------------------------------------------------
WaitSem, WaitSemCond, WaitSemTimed - esperar en un semaforo

WaitSem espera indefinidamente, WaitSemCond retorna inmediatamente y
WaitSemTimed espera con timeout. El valor de retorno indica si se consumio
un evento del semaforo.
--------------------------------------------------------------------------------
*/

bool
WaitSem(Semaphore_t *sem)
{
	return WaitSemTimed(sem, FOREVER);
}

bool
WaitSemCond(Semaphore_t *sem)
{
	return WaitSemTimed(sem, 0);
}

bool
WaitSemTimed(Semaphore_t *sem, unsigned msecs)
{
	bool success;

	DisableInts();
	if ( (success = (sem->value > 0)) )
		sem->value--;
	else
		success = WaitQueueTimed(sem->queue, msecs);
	RestoreInts();

	return success;
}

/*
--------------------------------------------------------------------------------
SignalSem - senaliza un semaforo

Despierta al primer proceso de la cola o incrementa la cuenta si la cola
esta vacia.
--------------------------------------------------------------------------------
*/

void
SignalSem(Semaphore_t *sem)
{
	DisableInts();
	if ( !SignalQueue(sem->queue) )
		sem->value++;
	RestoreInts();
}

/*
--------------------------------------------------------------------------------
ValueSem - informa la cuenta de un semaforo
--------------------------------------------------------------------------------
*/

unsigned	
ValueSem(Semaphore_t *sem)
{
	return sem->value;
}

/*
--------------------------------------------------------------------------------
FlushSem - despierta todos los procesos que esperan en un semaforo

Los procesos completan su WaitSem() con el status que se pasa como argumento.
Deja la cuenta en cero.
--------------------------------------------------------------------------------
*/

void
FlushSem(Semaphore_t *sem, bool wait_ok)
{
	DisableInts();
	sem->value = 0;
	FlushQueue(sem->queue, wait_ok);
	RestoreInts();
}
