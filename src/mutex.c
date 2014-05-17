#include "kernel.h"

/*
--------------------------------------------------------------------------------
CreateMutex - aloca un mutex inicialmente libre
--------------------------------------------------------------------------------
*/

Mutex_t *
CreateMutex(char *name)
{
	Mutex_t *mut = Malloc(sizeof(Mutex_t));

	mut->sem = CreateSem(name, 1);
	return mut;
}

/*
--------------------------------------------------------------------------------
DeleteMutex - da de baja un mutex
--------------------------------------------------------------------------------
*/

void 			
DeleteMutex(Mutex_t *mut)
{
	DeleteSem(mut->sem);
	Free(mut);
}

/*
--------------------------------------------------------------------------------
EnterMutex, EnterMutexCond, EnterMutexTimed - ocupar un mutex.

El valor de retorno indica si la operacion fue exitosa, en cuyo caso el
proceso es dueno del mutex.
El mutex puede tomarse anidadamente, para liberarlo debe llamarse tantas
veces a LeaveMutex como las que se lo ocupo exitosamente.
--------------------------------------------------------------------------------
*/

bool			
EnterMutex(Mutex_t *mut)
{
	return EnterMutexTimed(mut, FOREVER);
}

bool			
EnterMutexCond(Mutex_t *mut)
{
	return EnterMutexTimed(mut, 0);
}

bool			
EnterMutexTimed(Mutex_t *mut, unsigned msecs)
{
	if ( mut->owner == mt_curr_task )
	{
		mut->use_count++;
		return true;
	}
	if ( WaitSemTimed(mut->sem, msecs) )
	{
		mut->owner = mt_curr_task;
		mut->use_count = 1;
		return true;
	}
	return false;
}

/*
--------------------------------------------------------------------------------
LeaveMutex - libera un mutex

Para liberar un mutex, debe llamarse tantas veces como se lo ocupo. Produce
un error fatal si el proceso actual no es dueno del mutex.
--------------------------------------------------------------------------------
*/

void			
LeaveMutex(Mutex_t *mut)
{
	if ( mut->owner != mt_curr_task )
		Panic("LeaveMutex: el proceso no posee el mutex");

	if ( !--mut->use_count )
	{
		mut->owner = NULL;
		SignalSem(mut->sem);
	}
}

