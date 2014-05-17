#include "kernel.h"

/*
--------------------------------------------------------------------------------
CreatePipe, DeletePipe - creacion y destruccion de pipes.

El parametro size determina el tamano del buffer interno. En principio puede
usarse cualquier valor; cuanto mas grande sea el buffer, mayor sera el
desacoplamiento entre los procesos que escriben y los que leen en el pipe.
--------------------------------------------------------------------------------
*/

Pipe_t *
CreatePipe(char *name, unsigned size)
{
	char buf[200];
	Pipe_t *p = Malloc(sizeof(Pipe_t));

	p->head = p->tail = p->buf = Malloc(p->size = size);
	p->end = p->buf + size;
	p->monitor = CreateMonitor(name);
	sprintf(buf, "Get %s", name);
	p->cond_get = CreateCondition(buf, p->monitor);
	sprintf(buf, "Put %s", name);
	p->cond_put = CreateCondition(buf, p->monitor);

	return p;
}

void
DeletePipe(Pipe_t *p)
{
	DeleteCondition(p->cond_get);
	DeleteCondition(p->cond_put);
	DeleteMonitor(p->monitor);
	Free(p->buf);
	Free(p);
}

/*
--------------------------------------------------------------------------------
GetPipe, GetPipeCond, GetPipeTimed - lectura de un pipe.

GetPipe intenta leer size bytes de un pipe, bloqueandose si el pipe está vacío.
Retorna cuando puede leer algo, aunque sea una cantidad menor.
GetPipeTimed se comporta igual, pero puede salir prematuramente por timeout.
GetPipeCond lee los bytes que puede y retorna inmediatamente.

Estas funciones retornan la cantidad de bytes leidos.
--------------------------------------------------------------------------------
*/

unsigned
GetPipe(Pipe_t *p, void *data, unsigned size)
{
	return GetPipeTimed(p, data, size, FOREVER);
}

unsigned
GetPipeCond(Pipe_t *p, void *data, unsigned size)
{
	return GetPipeTimed(p, data, size, 0);
}

unsigned
GetPipeTimed(Pipe_t *p, void *data, unsigned size, unsigned msecs)
{
	unsigned i, nbytes;
	char *d;

	if ( !size )
		return 0;

	EnterMonitor(p->monitor);
	// Bloquearse si el pipe está vacío
	while ( !p->avail ) 
		if ( !WaitConditionTimed(p->cond_get, msecs) )
		{
			LeaveMonitor(p->monitor);
			return 0;
		}
	// Leer lo que se pueda
	for ( nbytes = min(size, p->avail), d = data, i = 0 ; i < nbytes ; i++ )
	{
		*d++ = *p->head++;
		if ( p->head == p->end )
			p->head = p->buf;
	}
	if ( p->avail == p->size )		// despertar un eventual escritor bloqueado
		SignalCondition(p->cond_put);
	p->avail -= nbytes;
	LeaveMonitor(p->monitor);
	return nbytes;
}

/*
--------------------------------------------------------------------------------
PutPipe, PutPipeCond, PutPipeTimed - escritura en un pipe.

PutPipe intenta escribir size bytes en el pipe, bloqueandose si el pipe está lleno. Retorna cuando puede escribir algo, aunque sea una cantidad menor.
PutPipeTimed puede salir por timeout.
PutPipeCond escribe los bytes que puede y retorna inmediatamente.

Estas funciones retornan la cantidad de bytes escritos.
--------------------------------------------------------------------------------
*/

unsigned
PutPipe(Pipe_t *p, void *data, unsigned size)
{
	return PutPipeTimed(p, data, size, FOREVER);
}

unsigned
PutPipeCond(Pipe_t *p, void *data, unsigned size)
{
	return PutPipeTimed(p, data, size, 0);
}

unsigned
PutPipeTimed(Pipe_t *p, void *data, unsigned size, unsigned msecs)
{
	unsigned i, nbytes;
	char *d;

	if ( !size )
		return 0;

	EnterMonitor(p->monitor);
	// Bloquearse si el pipe está lleno
	while ( p->avail == p->size )
		if ( !WaitConditionTimed(p->cond_put, msecs) )
		{
			LeaveMonitor(p->monitor);
			return 0;
		}
	// Escribir lo que se pueda
	for ( nbytes = min(size, p->size - p->avail), d = data, i = 0 ; i < nbytes ; i++ )
	{
		*p->tail++ = *d++;
		if ( p->tail == p->end )
			p->tail = p->buf;
	}
	if ( !p->avail )		// despertar un eventual lector bloqueado
		SignalCondition(p->cond_get);
	p->avail += nbytes;
	LeaveMonitor(p->monitor);
	return nbytes;
}

/*
--------------------------------------------------------------------------------
AvailPipe - indica la cantidad de bytes almacenada en el pipe.
--------------------------------------------------------------------------------
*/

unsigned
AvailPipe(Pipe_t *p)
{
	return p->avail;
}
