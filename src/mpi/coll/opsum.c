/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#define MPIR_LSUM(a,b) ((a)+(b))

void MPIR_SUM ( 
	void *invec, 
	void *inoutvec, 
	int *Len, 
	MPI_Datatype *type )
{
    int i, len = *Len;

    switch (*type) {
    case MPI_INT: {
        int * restrict a = (int *)inoutvec; 
        int * restrict b = (int *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED: {
        unsigned * restrict a = (unsigned *)inoutvec; 
        unsigned * restrict b = (unsigned *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_LONG: {
        long * restrict a = (long *)inoutvec; 
        long * restrict b = (long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_LONG_INT)
    case MPI_LONG_LONG: case MPI_LONG_LONG_INT: {
        long long * restrict a = (long long *)inoutvec; 
        long long * restrict b = (long long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
#endif
    
    case MPI_UNSIGNED_LONG: {
        unsigned long * restrict a = (unsigned long *)inoutvec; 
        unsigned long * restrict b = (unsigned long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_SHORT: {
        short * restrict a = (short *)inoutvec; 
        short * restrict b = (short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_SHORT: {
        unsigned short * restrict a = (unsigned short *)inoutvec; 
        unsigned short * restrict b = (unsigned short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_CHAR: {
        char * restrict a = (char *)inoutvec; 
        char * restrict b = (char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_CHAR: {
        unsigned char * restrict a = (unsigned char *)inoutvec; 
        unsigned char * restrict b = (unsigned char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_FLOAT: {
        float * restrict a = (float *)inoutvec; 
        float * restrict b = (float *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
    case MPI_DOUBLE: {
        double * restrict a = (double *)inoutvec; 
        double * restrict b = (double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE: {
        long double * restrict a = (long double *)inoutvec; 
        long double * restrict b = (long double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LSUM(a[i],b[i]);
        break;
    }
#endif
#ifdef UNIMPLEMENTED
    case MPI_COMPLEX: {
        s_complex * restrict a = (s_complex *)inoutvec; 
        s_complex * restrict b = (s_complex *)invec;
        for ( i=0; i<len; i++ ) {
            a[i].re = MPIR_LSUM(a[i].re ,b[i].re);
            a[i].im = MPIR_LSUM(a[i].im ,b[i].im);
        }
        break;
    }
    case MPI_DOUBLE_COMPLEX: {
        d_complex * restrict a = (d_complex *)inoutvec; 
        d_complex * restrict b = (d_complex *)invec;
        for ( i=0; i<len; i++ ) {
            a[i].re = MPIR_LSUM(a[i].re ,b[i].re);
            a[i].im = MPIR_LSUM(a[i].im ,b[i].im);
        }
        break;
    }
#endif
    default:
        /* TEMPORARY ERROR MESSAGE. NEED TO RETURN PROPER ERROR CODE */
        printf("MPI_SUM operation not supported for this datatype\n");
        NMPI_Abort(MPI_COMM_WORLD, 1);
        break;
    }
}

