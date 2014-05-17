#include "kernel.h"

static bool
get_msg(MsgQueue_t *mq, void *msg, unsigned msecs)
{
	if ( !WaitSemTimed(mq->sem_get, msecs) )
		return false;
	memcpy(msg, mq->head, mq->msg_size);
	mq->head += mq->msg_size;
	if ( mq->head == mq->end )
		mq->head = mq->buf;
	SignalSem(mq->sem_put);
	return true;
}

static bool
put_msg(MsgQueue_t *mq, void *msg, unsigned msecs)
{
	if ( !WaitSemTimed(mq->sem_put, msecs) )
		return false;
	memcpy(mq->tail, msg, mq->msg_size);
	mq->tail += mq->msg_size;
	if ( mq->tail == mq->end )
		mq->tail = mq->buf;
	SignalSem(mq->sem_get);
	return true;
}

/*
--------------------------------------------------------------------------------
CreateMsgQueue, DeleteMsgQueue - creacion y destruccion de colas de mensajes.

El parametro msg_max indica la maxima cantidad de mensajes a almacenar, y
msg_size el tamano de cada uno. Los otros parametros determinan si se desea 
tener mutexes de lectura y escritura en la cola, para permitir la existencia de
varios consumidores y/o productores de mensajes.
--------------------------------------------------------------------------------
*/

MsgQueue_t *
CreateMsgQueue(char *name, unsigned msg_max, unsigned msg_size, bool serialized_get, bool serialized_put)
{
	char buf[200];
	MsgQueue_t *mq;
	unsigned size = msg_max * msg_size;

	if ( size / msg_size != msg_max )	
		Panic("CreateMsgQueue: excede capacidad");

	mq = Malloc(sizeof(MsgQueue_t));
	mq->msg_size = msg_size;
	mq->head = mq->tail = mq->buf = Malloc(size);
	mq->end = mq->buf + size;
	sprintf(buf, "Get %s", name);
	mq->sem_get = CreateSem(buf, 0);
	if ( serialized_get )
		mq->mutex_get = CreateMutex(buf);
	sprintf(buf, "Put %s", name);
	mq->sem_put = CreateSem(buf, msg_max);
	if ( serialized_put )
		mq->mutex_put = CreateMutex(buf);

	return mq;
}

void
DeleteMsgQueue(MsgQueue_t *mq)
{
	DeleteSem(mq->sem_get);
	if ( mq->mutex_get )
		DeleteMutex(mq->mutex_get);
	DeleteSem(mq->sem_put);
	if ( mq->mutex_put )
		DeleteMutex(mq->mutex_put);
	Free(mq->buf);
	Free(mq);
}

/*
--------------------------------------------------------------------------------
GetMsgQueue, GetMsgQueueCond, GetMsgQueueTimed - lectura de un mensaje
--------------------------------------------------------------------------------
*/

bool
GetMsgQueue(MsgQueue_t *mq, void *msg)
{
	return GetMsgQueueTimed(mq, msg, FOREVER);
}

bool
GetMsgQueueCond(MsgQueue_t *mq, void *msg)
{
	return GetMsgQueueTimed(mq, msg, 0);
}

bool
GetMsgQueueTimed(MsgQueue_t *mq, void *msg, unsigned msecs)
{
	bool result;

	if ( mq->mutex_get && !EnterMutexTimed(mq->mutex_get, msecs) )
		return false;
	result = get_msg(mq, msg, msecs);
	if ( mq->mutex_get )
		LeaveMutex(mq->mutex_get);

	return result;
}

/*
--------------------------------------------------------------------------------
PutMsgQueue, PutMsgQueueCond, PutMsgQueueTimed - escritura de un mensaje
--------------------------------------------------------------------------------
*/

bool
PutMsgQueue(MsgQueue_t *mq, void *msg)
{
	return PutMsgQueueTimed(mq, msg, FOREVER);
}

bool
PutMsgQueueCond(MsgQueue_t *mq, void *msg)
{
	return PutMsgQueueTimed(mq, msg, 0);
}

bool
PutMsgQueueTimed(MsgQueue_t *mq, void *msg, unsigned msecs)
{
	bool result;

	if ( mq->mutex_put && !EnterMutexTimed(mq->mutex_put, msecs) )
		return false;
	result = put_msg(mq, msg, msecs);
	if ( mq->mutex_put )
		LeaveMutex(mq->mutex_put);

	return result;
}

/*
--------------------------------------------------------------------------------
AvailMsgQueue - indica la cantidad de mensajes almacenada en la cola
--------------------------------------------------------------------------------
*/

unsigned
AvailMsgQueue(MsgQueue_t *mq)
{
	return ValueSem(mq->sem_get);
}
