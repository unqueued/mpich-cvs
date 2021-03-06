/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Threads
 */

/* MPE_Thread_create() defined in mpe_thread_solaris.c */

#define MPE_Thread_exit()			\
{						\
    thr_exit(NULL);				\
}

#define MPE_Thread_self(id_ptr_)		\
{						\
    *(id_ptr_) = thr_self();			\
}

#define MPE_Thread_same(id1_ptr_, id2_ptr_, same_ptr_)	\
{							\
    *(same_ptr_) = (*(id1_ptr_) == *(id2_ptr_)) ? TRUE : FALSE;		\
}

#define MPE_Thread_yield()			\
{						\
    thr_yield();				\
}


/*
 *    Mutexes
 */

#define MPE_Thread_mutex_create(mutex_ptr_, err_ptr_)	\
{							\
    *(mutex_ptr_) = DEFAULTMUTEX;			\
    if ((err_ptr_) == NULL)				\
    { 							\
	*(err_ptr_) = MPE_THREAD_SUCCESS;		\
    }							\
}

#define MPE_Thread_mutex_destroy(mutex_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	mutex_destroy(mutex_ptr_);				\
    }								\
    else							\
    {								\
	*(err_ptr_) = mutex_destroy(mutex_ptr_);		\
	/* FIXME: convert error to an MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_mutex_lock(mutex_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	mutex_lock(mutex_ptr_);					\
    }								\
    else							\
    {								\
	*(err_ptr_) = mutex_lock(mutex_ptr_);			\
	/* FIXME: convert error to an MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_mutex_unlock(mutex_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	mutex_unlock(mutex_ptr_);				\
    }								\
    else							\
    {								\
	*(err_ptr_) = mutex_unlock(mutex_ptr_);			\
	/* FIXME: convert error to an MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_mutex_trylock(mutex_ptr_, flag_ptr_, err_ptr_)	\
{									\
    int err__;								\
    									\
    err__ = mutex_trylock(mutex_ptr_);					\
    *(flag_ptr_) = (err__ == 0) ? TRUE : FALSE;				\
    if ((err_ptr_) != NULL)						\
    {									\
	*(err_ptr_) = (err__ == EBUSY) : MPE_THREAD_SUCCESS ? err__;	\
	/* FIXME: convert error to an MPE_THREAD_ERR value */		\
    }									\
}


/*
 * Condition Variables
 */

#define MPE_Thread_cond_create(cond_ptr_, err_ptr_)	\
{							\
    *(cond_ptr_) = DEFAULTCV;				\
    if ((err_ptr_) == NULL)				\
    { 							\
	*(err_ptr_) = MPE_THREAD_SUCCESS;		\
    }							\
}

#define MPE_Thread_cond_destroy(cond_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	cond_destroy(cond_ptr_);				\
    }								\
    else							\
    {								\
	*(err_ptr_) = cond_destroy(cond_ptr_);			\
	/* FIXME: convert error to a MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)	\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	cond_wait((cond_ptr_), (mutex_ptr_));			\
    }								\
    else							\
    {								\
	*(err_ptr_) = cond_wait((cond_ptr_), (mutex_ptr_));	\
	/* FIXME: convert error to a MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_cond_broadcast(cond_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	cond_broadcast(cond_ptr_);				\
    }								\
    else							\
    {								\
	*(err_ptr_) = cond_broadcast(cond_ptr_);		\
	/* FIXME: convert error to a MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_cond_signal(cond_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) == NULL)					\
    { 								\
	cond_signal(cond_ptr_);					\
    }								\
    else							\
    {								\
	*(err_ptr_) = cond_signal(cond_ptr_);			\
	/* FIXME: convert error to a MPE_THREAD_ERR value */	\
    }								\
}


/*
 * Thread Local Storage
 */
typedef void (*MPE_Thread_tls_exit_func_t)(void * value);


#define MPE_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
{									\
    if ((err_ptr_) == NULL)						\
    {									\
	thr_keycreate((tls_ptr), (exit_func_ptr));			\
    }									\
    else								\
    {									\
	*(err_ptr_) = thr_keycreate((tls_ptr), (exit_func_ptr));	\
	/* FIXME: convert error to a MPE_THREAD_ERR value */		\
    }									\
}

#define MPE_Thread_tls_destroy(tls_ptr_, err_ptr_)										  \
{																  \
    /*																  \
     * FIXME: Solaris threads does not have a key destroy.  We need to create equivalent functionality to prevent a callback from \
     * occuring when a thread exits after the TLS is destroyed.  This is the only way to prevent subsystems that have shutdown	  \
     * from continuing to receive callbacks.											  \
     */																  \
    if ((err_ptr_) != NULL)													  \
    {																  \
	*(err_ptr) = MPE_THREAD_SUCCESS;											  \
    }																  \
}

#define MPE_Thread_tls_set(tls_ptr, value_)			\
{								\
    if ((err_ptr_) == NULL)					\
    {								\
	thr_setspecific(*(tls_ptr), (value_));			\
    }								\
    else							\
    {								\
	*(err_ptr_) = thr_setspecific(*(tls_ptr), (value_));	\
	/* FIXME: convert error to a MPE_THREAD_ERR value */	\
    }								\
}

#define MPE_Thread_tls_get(tls_ptr, value_ptr_)				\
{									\
    if ((err_ptr_) == NULL)						\
    {									\
	thr_setspecific(*(tls_ptr), (value_ptr_));			\
    }									\
    else								\
    {									\
	*(err_ptr_) = thr_setspecific(*(tls_ptr), (value_ptr_));	\
	/* FIXME: convert error to a MPE_THREAD_ERR value */		\
    }									\
}
