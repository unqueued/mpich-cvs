/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "timerconf.h"
#include "mpichtimer.h"
#include "mpiimpl.h"


#if MPICH_TIMER_KIND == USE_GETHRTIME 
void MPID_Wtime( MPID_Time_t *timeval )
{
    *timeval = gethrtime();
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = 1.0e-9 * (double)( t2 - t1 );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = 1.0e-9 * t;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 += (*t2 - *t1);
}
double MPID_Wtick( void )
{
    /* According to the documentation, ticks should be in nanoseconds.  This 
       is untested */ 
    return 1.0e-9;
}


#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
void MPID_Wtime( MPID_Time_t *timeval )
{
    /* POSIX timer (14.2.1, page 311) */
    clock_gettime( CLOCK_REALTIME, timeval );
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 
		1.0e-9 * (double) (t2->tv_nsec - t1->tv_nsec) );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = ((double) t->tv_sec + 1.0e-9 * (double) t->tv_nsec );
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    int nsec, sec;
    
    nsec = t1->tv_nsec + t2->tv_nsec;
    sec  = t1->tv_sec + t2->tv_sec;
    if (nsec > 1.0e9) {
	nsec -= 1.0e9;
	sec++;
    }
    t3->sec = sec;
    t3->nsec = nsec;
}
double MPID_Wtick( void )
{
    struct timespec res;
    int rc;

    rc = clock_getres( CLOCK_REALTIME, &res );
    if (!rc) 
	/* May return -1 for unimplemented ! */
	return res.tv_sec + 1.0e-9 * res.tv_nsec;

    /* Sigh.  If not POSIX not implemented, then we need to use the generic 
       tick routine */
    return MPID_Generic_wtick();
}
#define MPICH_NEEDS_GENERIC_WTICK
/* Rename the function so that we can access it */
#define MPID_Wtick MPID_Generic_wtick

#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
void MPID_Wtime( MPID_Time_t *tval )
{
    gettimeofday(tval,NULL);
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 
		.000001 * (double) (t2->tv_usec - t1->tv_usec) );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = (double) t->tv_sec + .000001 * (double) t->tv_usec;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    int usec, sec;
    
    usec = t2->tv_usec - t1->tv_usec;
    sec  = t2->tv_sec - t1->tv_sec;
    t3->tv_usec += usec;
    t3->tv_sec += sec;
    if (t3->tv_usec > 1.0e6) {
	t3->tv_usec -= 1.0e6;
	t3->tv_sec++;
    }
}
#define MPICH_NEEDS_GENERIC_WTICK


#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
#include <sys/time.h>
double g_timer_frequency;
void MPID_Wtime_init()
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPID_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPID_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    g_timer_frequency = (t2 - t1) / (td2 - td1);
}
/* Time stamps created by a macro */
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = (double)( *t2 - *t1 ) / g_timer_frequency;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    /* This returns the number of cycles as the "time".  This isn't correct
       for implementing MPI_Wtime, but it does allow us to insert cycle
       counters into test programs */
    *val = (double)*t / g_timer_frequency;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 += (*t2 - *t1);
}



#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
/* Code from LinuxJournal #42 (Oct-97), p50; 
   thanks to Dave Covey dnc@gi.alaska.edu
   Untested
 */
    unsigned long cc
    asm volatile( "rpcc %0" : "=r"(cc) : : "memory" );
    /* Convert to time.  Scale cc by 1024 incase it would overflow a double;
       consider using long double as well */
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
    *diff = 1024.0 * ((double)(cc/1024) / (double)CLOCK_FREQ_HZ);
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
}
double MPID_Wtick( void ) 
{
    return 1.0;
}


#elif MPICH_TIMER_KIND == USE_WIN86_CYCLE
double g_timer_frequency;
double MPID_Wtick(void)
{
    return g_timer_frequency;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *d)
{
    *d = (double)(__int64)*t / g_timer_frequency;
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff)
{
    *diff = (double)((__int64)( *t2 - *t1 )) / g_timer_frequency;
}
void MPID_Wtime_init()
{
    MPID_Time_t t1, t2;
    DWORD s1, s2;
    double d;
    int i;

    MPID_Wtime(&t1);
    MPID_Wtime(&t1);

    /* time an interval using both timers */
    s1 = GetTickCount();
    MPID_Wtime(&t1);
    /*Sleep(250);*/ /* Sleep causes power saving cpu's to stop which stops the counter */
    while (GetTickCount() - s1 < 200)
    {
	for (i=2; i<1000; i++)
	    d = (double)i / (double)(i-1);
    }
    s2 = GetTickCount();
    MPID_Wtime(&t2);

    /* calculate the frequency of the assembly cycle counter */
    g_timer_frequency = (double)((__int64)(t2 - t1)) / ((double)(s2 - s1) / 1000.0);
    /*
    printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n", 
	(int)(t2-t1), (int)(s2 - s1), g_timer_frequency, g_timer_frequency * 1.0e-6);
    */
}
/*
void TIMER_INIT()
{
    TIMER_TYPE t1, t2;
    FILETIME ft1, ft2;
    SYSTEMTIME st1, st2;
    ULARGE_INTEGER u1, u2;

    t1 = 5;
    t2 = 5;

    GET_TIME(&t1);
    GET_TIME(&t1);

    //GetSystemTimeAsFileTime(&ft1);
    GetSystemTime(&st1);
    GET_TIME(&t1);
    Sleep(500);
    //GetSystemTimeAsFileTime(&ft2);
    GetSystemTime(&st2);
    GET_TIME(&t2);

    SystemTimeToFileTime(&st1, &ft1);
    SystemTimeToFileTime(&st2, &ft2);

    u1.QuadPart = ft1.dwHighDateTime;
    u1.QuadPart = u1.QuadPart << 32;
    u1.QuadPart |= ft1.dwLowDateTime;
    u2.QuadPart = ft2.dwHighDateTime;
    u2.QuadPart = u2.QuadPart << 32;
    u2.QuadPart |= ft2.dwLowDateTime;

    g_timer_frequency = (double)((__int64)(t2 - t1)) / (1e-7 * (double)((__int64)(u2.QuadPart - u1.QuadPart)));
    printf("t2   %10d\nt1   %10d\ndiff %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n", 
	(int)t2, (int)t1, (int)(t2-t1), (int)(u2.QuadPart - u1.QuadPart), g_timer_frequency, g_timer_frequency * 1.0e-6);
    printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n", 
	(int)(t2-t1), (int)(u2.QuadPart - u1.QuadPart), g_timer_frequency, g_timer_frequency * 1.0e-6);
}
*/



#elif MPICH_TIMER_KIND == USE_QUERYPERFORMANCECOUNTER
static double g_timer_frequency=0.0;  /* High performance counter frequency */
void MPID_Wtime_init(void)
{
    LARGE_INTEGER n;
    QueryPerformanceFrequency(&n);
    g_timer_frequency = (double)n.QuadPart;
}
double MPID_Wtick(void)
{
    return g_timer_frequency;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = (double)t->QuadPart / g_timer_frequency;
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    LARGE_INTEGER n;
    n.QuadPart = t2->QuadPart - t1->QuadPart;
    *diff = (double)n.QuadPart / g_timer_frequency;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
  /* ??? */
}



#endif

#ifdef MPICH_NEEDS_GENERIC_WTICK
/*
 * For timers that do not have defined resolutions, compute the resolution
 * by sampling the clock itself.
 */
double MPID_Wtick( void )
{
    static double tickval = -1.0;
    double timediff;
    MPID_Time_t t1, t2;
    int    cnt;
    int    icnt;

    if (tickval < 0.0) {
	tickval = 1.0e6;
	for (icnt=0; icnt<10; icnt++) {
	    cnt = 1000;
	    MPID_Wtime( &t1 );
	    while (cnt--) {
		MPID_Wtime( &t2 );
		MPID_Wtime_diff( &t1, &t2, &timediff );
		if (timediff > 0) break;
		}
	    if (cnt && timediff > 0.0 && timediff < tickval) {
		MPID_Wtime_diff( &t1, &t2, &tickval );
	    }
	}
    }
    return tickval;
}
#endif
