/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED)
#define MPICH_MPIDI_CH3_POST_H_INCLUDED

/* FIXME: These sizes need to be documented at least, and it should be 
   easier to change them, at least at configure/build time */
/* #define MPIDI_CH3_EAGER_MAX_MSG_SIZE (1500 - sizeof(MPIDI_CH3_Pkt_t)) */
#define MPIDI_CH3_EAGER_MAX_MSG_SIZE 128000

/*
 * Channel level request management macros
 */
#define MPIDI_CH3_Request_add_ref(req_)					\
{									\
    MPIU_Assert(HANDLE_GET_MPI_KIND((req_)->handle) == MPID_REQUEST);	\
    MPIU_Object_add_ref(req_);						\
}

#define MPIDI_CH3_Request_release_ref(req_, req_ref_count_)		\
{									\
    MPIU_Assert(HANDLE_GET_MPI_KIND((req_)->handle) == MPID_REQUEST);	\
    MPIU_Object_release_ref((req_), (req_ref_count_));			\
    MPIU_Assert((req_)->ref_count >= 0);				\
}

/*
 * MPIDI_CH3_Progress_signal_completion() is used to notify the progress
 * engine that a completion has occurred.  The multi-threaded version will need
 * to wake up any (and all) threads blocking in MPIDI_CH3_Progress().
 */
extern volatile unsigned int MPIDI_CH3I_progress_completion_count;
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    extern volatile int MPIDI_CH3I_progress_blocked;
    extern volatile int MPIDI_CH3I_progress_wakeup_signalled;

    void MPIDI_CH3I_Progress_wakeup(void);
#endif

#if (MPICH_THREAD_LEVEL != MPI_THREAD_MULTIPLE)
#   define MPIDI_CH3_Progress_signal_completion()	\
    {							\
        MPIDI_CH3I_progress_completion_count++;		\
    }
#else
#   define MPIDI_CH3_Progress_signal_completion()			\
    {									\
	MPIDI_CH3I_progress_completion_count++;				\
	if (MPIDI_CH3I_progress_blocked == TRUE && MPIDI_CH3I_progress_wakeup_signalled == FALSE)\
	{								\
	    MPIDI_CH3I_progress_wakeup_signalled = TRUE;		\
	    MPIDI_CH3I_Progress_wakeup();				\
	}								\
    }
#endif


/*
 * CH3 Progress routines (implemented as macros for performanace)
 */

#define MPIDI_CH3_Progress_start(progress_state_)			\
{									\
    (progress_state_)->ch.completion_count = MPIDI_CH3I_progress_completion_count;\
}
#define MPIDI_CH3_Progress_end(progress_state_)
#define MPIDI_CH3_Progress_poke() (MPIDI_CH3_Progress_test())

/* FIXME: Are these the same definitions used by all channels? If so, they
 should be defined in the same place (and it would make sense for the 
 progress routines in the ch3 device to have the same interface independent of 
 channel) */
#if defined(USE_FIXED_SPIN_WAITS) || !defined(MPID_CPU_TICK) || defined(USE_FIXED_ACTIVE_PROGRESS)
int MPIDI_CH3I_Progress(int blocking, MPID_Progress_state *state);
#define MPIDI_CH3_Progress_test() MPIDI_CH3I_Progress(FALSE, NULL)
#define MPIDI_CH3_Progress_wait(state) MPIDI_CH3I_Progress(TRUE, state)
#endif

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */
