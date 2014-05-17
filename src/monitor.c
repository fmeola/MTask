#include "kernel.h"

/*
--------------------------------------------------------------------------------
CreateMonitor - aloca un monitor inicialmente libre
--------------------------------------------------------------------------------
*/

Monitor_t *
CreateMonitor(char *name)
{
	Monitor_t *mon = Malloc(sizeof(Monitor_t));

	mon->sem = CreateSem(name, 1);
	return mon;
}

/*
--------------------------------------------------------------------------------
DeleteMonitor - da de baja un monitor
--------------------------------------------------------------------------------
*/

void 			
DeleteMonitor(Monitor_t *mon)
{
	DeleteSem(mon->sem);
	Free(mon);
}

/*
--------------------------------------------------------------------------------
EnterMonitor, EnterMonitorCond, EnterMonitorTimed - ocupar un monitor.

El valor de retorno indica si la operacion fue exitosa, en cuyo caso el
proceso es dueno del monitor.
--------------------------------------------------------------------------------
*/

bool			
EnterMonitor(Monitor_t *mon)
{
	return EnterMonitorTimed(mon, FOREVER);
}

bool			
EnterMonitorCond(Monitor_t *mon)
{
	return EnterMonitorTimed(mon, 0);
}

bool			
EnterMonitorTimed(Monitor_t *mon, unsigned msecs)
{
	if ( mon->owner == mt_curr_task )
		Panic("EnterMonitorTimed: monitor ya ocupado por este proceso");

	if ( WaitSemTimed(mon->sem, msecs) )
	{
		mon->owner = mt_curr_task;
		return true;
	}
	return false;
}

/*
--------------------------------------------------------------------------------
LeaveMonitor - libera un monitor
--------------------------------------------------------------------------------
*/

void			
LeaveMonitor(Monitor_t *mon)
{
	if ( mon->owner != mt_curr_task )
		Panic("LeaveMonitor: el proceso no posee el monitor");

	mon->owner = NULL;
	SignalSem(mon->sem);
}

/*
--------------------------------------------------------------------------------
CreateCondition - crea una variable de condicion asociada a un monitor
--------------------------------------------------------------------------------
*/

Condition_t *		
CreateCondition(char *name, Monitor_t *mon)
{
	Condition_t *cond = Malloc(sizeof(Condition_t));

	cond->queue = CreateQueue(name);
	cond->monitor = mon;
	return cond;
}

/*
--------------------------------------------------------------------------------
DeleteCondition - da de baja una variable de condicion
--------------------------------------------------------------------------------
*/

void				
DeleteCondition(Condition_t *cond)
{
	DeleteQueue(cond->queue);
	Free(cond);
}

/*
--------------------------------------------------------------------------------
WaitCondition, WaitConditionTimed - esperar una condicion

El proceso que espera en la variable de condicion debe estar dentro del monitor.
Estas funciones atomicamente dejan el monitor, esperan en la cola de procesos
de la condicion y vuelven a tomar el monitor para retornar el resultado de la
espera. 
--------------------------------------------------------------------------------
*/

bool				
WaitCondition(Condition_t *cond)
{
	return WaitConditionTimed(cond, FOREVER);
}

bool				
WaitConditionTimed(Condition_t *cond, unsigned msecs)
{
	bool success;
	Monitor_t *mon = cond->monitor;

	if ( mon->owner != mt_curr_task )
		Panic("WaitConditionTimed: el proceso no posee el monitor");

	Atomic();
	LeaveMonitor(mon);
	success = WaitQueueTimed(cond->queue, msecs);
	while ( !EnterMonitor(mon) )	// Hay que volver a tomar el monitor si o si
		;
	Unatomic();

	return success;
}

/*
--------------------------------------------------------------------------------
SignalCondition - senalizar una condicion

El proceso que senaliza la variable de condicion debe estar dentro del monitor.
Esta funcion despierta un proceso de los que esten esperando en la cola de
procesos de la condicion, si hay alguno. El proceso despertado completa
exitosamente su WaitConditionTimed.
El valor de retorno indica si se ha despertado a un proceso.
--------------------------------------------------------------------------------
*/

bool				
SignalCondition(Condition_t *cond)
{
	if ( cond->monitor->owner != mt_curr_task )
		Panic("SignalCondition: el proceso no posee el monitor");

	return SignalQueue(cond->queue);
}

/*
--------------------------------------------------------------------------------
BroadcastCondition - senalizar en broadcast una condicion

El proceso que senaliza la variable de condicion debe estar dentro del monitor.
Esta funcion despierta a todos los procesos que esten esperando en la cola de
procesos de la condicion, los cuales completan exitosamente sus
WaitConditionTimed.
--------------------------------------------------------------------------------
*/

void				
BroadcastCondition(Condition_t *cond)
{
	if ( cond->monitor->owner != mt_curr_task )
		Panic("BroadcastCondition: el proceso no posee el monitor");

	FlushQueue(cond->queue, true);
}
