/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICHTIMER_H
#define MPICHTIMER_H
/*
 * This include file provide the definitions that are necessary to use the
 * timer calls, including the definition of the time stamp type and 
 * any inlined timer calls.
 *
 * The include file timerconf.h (created by autoheader from configure.in)
 * is needed only to build the function versions of the timers.
 */
/* Include the appropriate files */
#define USE_GETHRTIME 1
#define USE_CLOCK_GETTIME 2
#define USE_GETTIMEOFDAY 3
#define USE_LINUX86_COUNTER 4
#define USE_LINUXALPHA_COUNTER 5
#define USE_QUERYPERFORMANCECOUNTER 6
#define MPICH_TIMER_KIND USE_QUERYPERFORMANCECOUNTER

#if MPICH_TIMER_KIND == USE_GETHRTIME 
#include <sys/time.h>
#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
#include <time.h>
#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#include <sys/types.h>
#include <sys/time.h>
#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
#elif MPICH_TIMER_KIND == USE_QUERYPERFORMANCECOUNTER
#include <winsock2.h>
#include <windows.h>
#endif

/* Define a time stamp */
typedef LARGE_INTEGER MPID_Time_t;

/* 
 * Prototypes.  These are defined here so that inlined timer calls can
 * use them, as well as any profiling and timing code that is built into
 * MPICH
 */
void MPID_Wtime( MPID_Time_t * );
void MPID_Wtime_diff( MPID_Time_t *, MPID_Time_t *, double * );
void MPID_Wtime_acc( MPID_Time_t *, MPID_Time_t *, MPID_Time_t * );
void MPID_Wtime_todouble( MPID_Time_t *, double * );
double MPID_Wtick( void );
void MPID_Wtime_init(void);

/* Inlined timers.  Note that any definition of one of the functions
   prototyped above in terms of a macro will simply cause the compiler
   to use the macro instead of the function definition.

   Currently, all except the Windows performance counter timers
   define MPID_Wtime_init as null; by default, the value of
   MPI_WTIME_IS_GLOBAL is false.
 */

#if MPICH_TIMER_KIND == USE_GETHRTIME 
#define MPID_Wtime_init()

#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
#define MPID_Wtime_init()

#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#define MPID_Wtime_init()

#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
#define MPID_Wtime_init()
#define MPID_Wtime(var) {long long t1;\
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (t1));\
   *(var) = t1;}

#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
#define MPID_Wtime_init()

#endif

#endif
