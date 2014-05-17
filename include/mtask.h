#ifndef MTASK_H_INCLUDED
#define MTASK_H_INCLUDED

#include "lib.h"

#define MIN_PRIO		0
#define DEFAULT_PRIO	50
#define FOREVER			-1U

#ifndef NULL
#define NULL 0
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef enum { false, true } bool;

typedef enum 
{ 
	TaskSuspended, 
	TaskReady, 
	TaskCurrent, 
	TaskDelaying, 
	TaskWaiting, 
	TaskSending, 
	TaskReceiving, 
	TaskTerminated 
} 
TaskState_t;

typedef struct Task_t Task_t;

typedef struct
{
	char *			name;
	Task_t *		head;
	Task_t *		tail;
}
TaskQueue_t;

struct Task_t
{
	char *			name;
	TaskState_t		state;
	unsigned		priority;
	unsigned		atomic_level;
	unsigned		disint_level;
	unsigned 		esp;			// offset = 20, sincronizar con interrupts.asm
	char *			stack;
	void *			math_data;
	TaskQueue_t	*	queue;
	Task_t *		prev;
	Task_t *		next;
	bool			success;
	bool			in_time_q;
	Task_t *		time_prev;
	Task_t *		time_next;
	unsigned		ticks;
	void *			data;
	Task_t *		from;
	void *			msg;
	unsigned 		size;
	TaskQueue_t 	send_queue;
};

typedef void (*TaskFunc_t)(void *arg);
typedef void (*Switcher_t)(Task_t *save, Task_t *restore);

/* API principal */

Task_t *			CreateTask(TaskFunc_t func, unsigned stacksize, void *arg, char *name, unsigned priority);
Task_t *			CurrentTask(void);
void				DeleteTask(Task_t *task);
	
unsigned			GetPriority(Task_t *task);
void				SetPriority(Task_t *task, unsigned priority);
void				Suspend(Task_t *task);
void				Ready(Task_t *task);

TaskQueue_t *		CreateQueue(char *name);
void				DeleteQueue(TaskQueue_t *queue);
bool				WaitQueue(TaskQueue_t *queue);
bool				WaitQueueTimed(TaskQueue_t *queue, unsigned msecs);
bool				SignalQueue(TaskQueue_t *queue);
void				FlushQueue(TaskQueue_t *queue, bool success);

bool				Send(Task_t *to, void *msg, unsigned size);
bool				SendCond(Task_t *to, void *msg, unsigned size);
bool				SendTimed(Task_t *to, void *msg, unsigned size, unsigned msecs);
bool				Receive(Task_t **from, void *msg, unsigned *size);
bool				ReceiveCond(Task_t **from, void *msg, unsigned *size);
bool				ReceiveTimed(Task_t **from, void *msg, unsigned *size, unsigned msecs);

void				Pause(void);
void				Yield(void);
void				Delay(unsigned msecs);
void				Exit(void);

void				Atomic(void);
void				Unatomic(void);
void	 			DisableInts(void);
void				RestoreInts(void);

void *				Malloc(unsigned size);
char *				StrDup(char *str);
void 				Free(void *mem);

void				SetData(Task_t *task, void *data);
void				SetSwitcher(Switcher_t switcher);

void				Panic(char *msg);

/* Semáforos */

typedef struct
{
	unsigned		value;
	TaskQueue_t *	queue;
}
Semaphore_t;

Semaphore_t *		CreateSem(char *name, unsigned value);
void 				DeleteSem(Semaphore_t *sem);
bool 				WaitSem(Semaphore_t *sem);
bool 				WaitSemCond(Semaphore_t *sem);
bool 				WaitSemTimed(Semaphore_t *sem, unsigned msecs);
void 				SignalSem(Semaphore_t *sem);
unsigned			ValueSem(Semaphore_t *sem);
void 				FlushSem(Semaphore_t *sem, bool wait_ok);

/* Mutexes */

typedef struct Mutex_t
{
	unsigned		use_count;
	Task_t *		owner;
	Semaphore_t *	sem;
}
Mutex_t;

Mutex_t *			CreateMutex(char *name);
void 				DeleteMutex(Mutex_t *mut);
bool				EnterMutex(Mutex_t *mut);
bool				EnterMutexCond(Mutex_t *mut);
bool				EnterMutexTimed(Mutex_t *mut, unsigned msecs);
void				LeaveMutex(Mutex_t *mut);

/* Monitores y variables de condición */

typedef struct
{
	Task_t *		owner;
	Semaphore_t *	sem;
}
Monitor_t;

Monitor_t *			CreateMonitor(char *name);
void 				DeleteMonitor(Monitor_t *mon);
bool				EnterMonitor(Monitor_t *mon);
bool				EnterMonitorCond(Monitor_t *mon);
bool				EnterMonitorTimed(Monitor_t *mon, unsigned msecs);
void				LeaveMonitor(Monitor_t *mon);

typedef struct
{
	Monitor_t *		monitor;
	TaskQueue_t *	queue;
}
Condition_t;

Condition_t *		CreateCondition(char *name, Monitor_t *mon);
void				DeleteCondition(Condition_t *cond);
bool				WaitCondition(Condition_t *cond);
bool				WaitConditionTimed(Condition_t *cond, unsigned msecs);
bool				SignalCondition(Condition_t *cond);
void				BroadcastCondition(Condition_t *cond);

/* Pipes */

typedef struct
{
	Monitor_t *		monitor;
	Condition_t *	cond_get;
	Condition_t *	cond_put;
	unsigned		size;
	unsigned		avail;
	char *			buf;
	char *			head;
	char *			tail;
	char *			end;
}
Pipe_t;

Pipe_t *			CreatePipe(char *name, unsigned size);
void				DeletePipe(Pipe_t *p);
unsigned			GetPipe(Pipe_t *p, void *data, unsigned size);
unsigned			GetPipeCond(Pipe_t *p, void *data, unsigned size);
unsigned			GetPipeTimed(Pipe_t *p, void *data, unsigned size, unsigned msecs);
unsigned			PutPipe(Pipe_t *p, void *data, unsigned size);
unsigned			PutPipeCond(Pipe_t *p, void *data, unsigned size);
unsigned			PutPipeTimed(Pipe_t *p, void *data, unsigned size, unsigned msecs);
unsigned			AvailPipe(Pipe_t *p);

/* Colas de mensajes */

typedef struct
{
	Mutex_t *		mutex_get;
	Mutex_t *		mutex_put;
	Semaphore_t *	sem_get;
	Semaphore_t *	sem_put;
	unsigned		msg_size;
	char *			buf;
	char *			head;
	char *			tail;
	char *			end;
}
MsgQueue_t;

MsgQueue_t *		CreateMsgQueue(char *name, unsigned msg_max, unsigned msg_size, bool serialized_get, bool serialized_put);
void				DeleteMsgQueue(MsgQueue_t *mq);
bool				GetMsgQueue(MsgQueue_t *mq, void *msg);
bool				GetMsgQueueCond(MsgQueue_t *mq, void *msg);
bool				GetMsgQueueTimed(MsgQueue_t *mq, void *msg, unsigned msecs);
bool				PutMsgQueue(MsgQueue_t *mq, void *msg);
bool				PutMsgQueueCond(MsgQueue_t *mq, void *msg);
bool				PutMsgQueueTimed(MsgQueue_t *mq, void *msg, unsigned msecs);
unsigned			AvailMsgQueue(MsgQueue_t *mq);

#endif
