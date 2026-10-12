#ifdef __WASM__

#include "../qcommon/qcommon.h"
#include "../qcommon/sys_threads.h"

#include <stdlib.h>
#include <string.h>

/*
 * First-cut WASM build is single-threaded: mutexes are no-ops and
 * "threads" are handles that never execute in parallel. Callers that
 * Join() after doing the work themselves (pak loading) keep working.
 * Infinite loops such as the HTTP server thread are not started — see
 * SV_Web_Init. Enabling real pthreads requires SharedArrayBuffer + COOP/COEP.
 */

struct qthread_s {
	void *( *routine )( void * );
	void *param;
};

struct qmutex_s {
	int dummy;
};

struct qcondvar_s {
	int dummy;
};

int Sys_Mutex_Create( qmutex_t **pmutex )
{
	qmutex_t *mutex;

	if( !pmutex )
		return -1;
	mutex = (qmutex_t *)calloc( 1, sizeof( *mutex ) );
	if( !mutex )
		return -1;
	*pmutex = mutex;
	return 0;
}

void Sys_Mutex_Destroy( qmutex_t *mutex )
{
	free( mutex );
}

void Sys_Mutex_Lock( qmutex_t *mutex )
{
	(void)mutex;
}

void Sys_Mutex_Unlock( qmutex_t *mutex )
{
	(void)mutex;
}

int Sys_Thread_Create( qthread_t **pthread, void *( *routine )( void * ), void *param )
{
	qthread_t *thread;

	if( !pthread || !routine )
		return -1;
	thread = (qthread_t *)calloc( 1, sizeof( *thread ) );
	if( !thread )
		return -1;
	thread->routine = routine;
	thread->param = param;
	routine( param );
	*pthread = thread;
	return 0;
}

void Sys_Thread_Join( qthread_t *thread )
{
	free( thread );
}

void Sys_Thread_Yield( void )
{
}

int Sys_Atomic_Add( volatile int *value, int add, qmutex_t *mutex )
{
	(void)mutex;
	*value += add;
	return *value - add;
}

bool Sys_Atomic_CAS( volatile int *value, int oldval, int newval, qmutex_t *mutex )
{
	(void)mutex;
	if( *value != oldval )
		return false;
	*value = newval;
	return true;
}

int Sys_CondVar_Create( qcondvar_t **pcond )
{
	qcondvar_t *cond;

	if( !pcond )
		return -1;
	cond = (qcondvar_t *)calloc( 1, sizeof( *cond ) );
	if( !cond )
		return -1;
	*pcond = cond;
	return 0;
}

void Sys_CondVar_Destroy( qcondvar_t *cond )
{
	free( cond );
}

bool Sys_CondVar_Wait( qcondvar_t *cond, qmutex_t *mutex, unsigned int timeout_msec )
{
	(void)cond;
	(void)mutex;
	(void)timeout_msec;
	return true;
}

void Sys_CondVar_Wake( qcondvar_t *cond )
{
	(void)cond;
}

#endif // __WASM__
