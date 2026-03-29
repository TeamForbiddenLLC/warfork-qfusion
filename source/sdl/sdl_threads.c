/*
Copyright (C) 2013 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <SDL.h>
#include "../qcommon/qcommon.h"
#include "../qcommon/sys_threads.h"

struct qthread_s {
	SDL_Thread *t;
};

struct qmutex_s {
	SDL_mutex *m;
};

struct qcondvar_s {
	SDL_cond *c;
};

/*
* Sys_Mutex_Create
*/
int Sys_Mutex_Create( qmutex_t **pmutex )
{
	if(!pmutex) 
		return -1;

	qmutex_t *mutex = (qmutex_t *)malloc(sizeof(*mutex));
	
	if(!mutex) 
		return -1;
	
	mutex->m = SDL_CreateMutex();
	if(!mutex->m) {
		free(mutex);
		return -1;
	}

	*pmutex = mutex;
	return 0;
}

/*
* Sys_Mutex_Destroy
*/
void Sys_Mutex_Destroy( qmutex_t *mutex )
{
	if (!mutex) {
		return;
	}

	SDL_DestroyMutex(mutex->m);
	free(mutex);
}

/*
* Sys_Mutex_Lock
*/
void Sys_Mutex_Lock( qmutex_t *mutex )
{
	SDL_LockMutex(mutex->m);
}

/*
* Sys_Mutex_Unlock
*/
void Sys_Mutex_Unlock( qmutex_t *mutex )
{
	SDL_UnlockMutex(mutex->m);
}

/*
* Sys_Thread_Create
*/
int Sys_Thread_Create( qthread_t **pthread, void *(*routine) (void*), void *param )
{
	if(!pthread) 
		return -1;
	
	qthread_t *thread = (qthread_t *)malloc( sizeof( *thread ) );
	if( !thread ) 
		return -1;
	

	thread->t = SDL_CreateThread( (SDL_ThreadFunction)routine, NULL, param );
	if( !thread->t ) {
		free(thread);
		return -1;
	}

	*pthread = thread;
	return 0;
}

/*
* Sys_Thread_Join
*/
void Sys_Thread_Join( qthread_t *thread )
{
	int status = 0;

	if (thread) {
		SDL_WaitThread( thread->t, &status );
		free( thread );
	}
}

/*
* Sys_Thread_Yield
*/
void Sys_Thread_Yield( void )
{
	Sys_Sleep(0);
}

/*
* Sys_Atomic_Add
*/
int Sys_Atomic_Add( volatile int *value, int add, qmutex_t *mutex )
{
	return SDL_AtomicAdd( ( SDL_atomic_t * )value, add ) + add;
}

/*
* Sys_Atomic_CAS
*/
bool Sys_Atomic_CAS( volatile int *value, int oldval, int newval, qmutex_t *mutex )
{
	return SDL_AtomicCAS( ( SDL_atomic_t * )value, newval, oldval ) == SDL_TRUE;
}

/*
* Sys_CondVar_Create
*/
int Sys_CondVar_Create( qcondvar_t **pcond )
{
	if( !pcond )
		return -1;
	

	qcondvar_t *const cond = (qcondvar_t *)malloc( sizeof( *cond ) );
	if(!cond)
		return -1;
	cond->c = SDL_CreateCond();
	if( !cond->c ) {
		free( cond );
		return -1;
	}

	*pcond = cond;
	return 0;
}

/*
* Sys_CondVar_Destroy
*/
void Sys_CondVar_Destroy( qcondvar_t *cond )
{
	if( !cond ) {
		return;
	}

	SDL_DestroyCond( cond->c );
	free( cond );
}

/*
* Sys_CondVar_Wait
*/
bool Sys_CondVar_Wait( qcondvar_t *cond, qmutex_t *mutex, unsigned int timeout_msec )
{
	if( !cond || !mutex ) {
		return false;
	}

	return SDL_CondWaitTimeout( cond->c, mutex->m, timeout_msec ) == 0;
}

/*
* Sys_CondVar_Wake
*/
void Sys_CondVar_Wake( qcondvar_t *cond )
{
	if( !cond ) {
		return;
	}

	SDL_CondSignal( cond->c );
}
