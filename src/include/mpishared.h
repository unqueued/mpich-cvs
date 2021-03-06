/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file provides the very basic definitions for use in a component
 * that is to work with MPICH2.  This defines (by inclusion) the 
 * MPI error reporting routines, memory access routines, and common types.
 * Using this include (instead of the comprehensive mpiimpl.h file) is 
 * appropriate for modules that perform more basic functions, such as
 * basic socket communications (see, for example, src/mpid/common/sock/poll)
 */

#ifndef MPISHARED_H_INCLUDED
#define MPISHARED_H_INCLUDED

/* Make sure that we have the basic definitions */
#ifndef MPICHCONF_H_INCLUDED
#include "mpichconf.h"
#endif

/* The most common MPI error classes */
#ifndef MPI_SUCCESS
#define MPI_SUCCESS 0
#define MPI_ERR_ARG         12      /* Invalid argument */
#define MPI_ERR_UNKNOWN     13      /* Unknown error */
#define MPI_ERR_OTHER       15      /* Other error; use Error_string */
#define MPI_ERR_INTERN      16      /* Internal error code    */
#endif

/* For supported thread levels */
#ifndef MPI_THREAD_SINGLE
#define MPI_THREAD_SINGLE 0
#define MPI_THREAD_FUNNELED 1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE 3
#endif

/* For thread support */
#include "mpiimplthread.h"

/* Error reporting routines */
#include "mpierror.h"
#include "mpierrs.h"

#define MPIU_QUOTE(A) MPIU_QUOTE2(A)
#define MPIU_QUOTE2(A) #A

/* FIXME: This is extracted from mpi.h.in, where it may not be appropriate */
#define MPICH_ERR_LAST_CLASS 53     /* It is also helpful to know the
				       last valid class */

/* Add support for the states and function enter/exit macros */
/* #include "mpitimerimpl.h" */
#if defined(MPICH_DEBUG_FINE_GRAIN_NESTING)
#   include "mpidu_func_nesting.h"
#elif defined(MPICH_DEBUG_MEMARENA)
#   include "mpifuncmem.h"
#elif defined(USE_DBG_LOGGING)
#   include "mpifunclog.h"
#elif !defined(NEEDS_FUNC_ENTER_EXIT_DEFS)
    /* If no timing choice is selected, this sets the entry/exit macros 
       to empty */
#   include "mpitimerimpl.h"
#endif
#ifdef NEEDS_FUNC_ENTER_EXIT_DEFS
/* mpich layer definitions */
#define MPID_MPI_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPID_MPI_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)

/* device layer definitions */
#define MPIDI_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_FRONT(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_COLL_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_COLL_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_INIT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_INIT_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_FINALIZE_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_FINALIZE_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)

/* evaporate the timing macros since timing is not selected */
#define MPIU_Timer_init(rank, size)
#define MPIU_Timer_finalize()
#endif /* NEEDS_FUNC_ENTER_EXIT_DEFS */

/* Add support for the memory allocation routines */
#define MPIU_MEM_NOSTDIO
#include "mpimem.h"

/* Add support for the debugging macros */
#include "mpidbg.h"

/* Add support for the assert and strerror routines */
#include "mpiutil.h"

/* Prototypes for the functions to provide uniform access to the environment */
int MPIU_GetEnvInt( const char *envName, int *val );
int MPIU_GetEnvRange( const char *envName, int *lowPtr, int *highPtr );
int MPIU_GetEnvBool( const char *envName, int *val );

/* Use this macro for each parameter to a function that is not referenced in
   the body of the function */
#ifdef HAVE_WINDOWS_H
#define MPIU_UNREFERENCED_ARG(a) a
#else
#define MPIU_UNREFERENCED_ARG(a)
#endif

#endif
