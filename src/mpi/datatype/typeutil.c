/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

/* This is the utility file for datatypes that contains the basic datatype items
   and storage management */
#ifndef MPID_DATATYPE_PREALLOC 
#define MPID_DATATYPE_PREALLOC 8
#endif

/* Preallocated datatype objects */
MPID_Datatype MPID_Datatype_builtin[MPID_DATATYPE_N_BUILTIN];
MPID_Datatype MPID_Datatype_direct[MPID_DATATYPE_PREALLOC];
MPIU_Object_alloc_t MPID_Datatype_mem = { 0, 0, 0, 0, MPID_DATATYPE, 
			      sizeof(MPID_Datatype), MPID_Datatype_direct,
					  MPID_DATATYPE_PREALLOC};

/* Call this routine to associate a MPID_Datatype with each predefined 
   datatype.  We do this with lazy initialization because many MPI 
   programs do not require anything except the predefined datatypes, and
   all of the necessary information about those is stored within the
   MPI_Datatype handle.  However, if the user wants to change the name
   (character string, set with MPI_Type_set_name) associated with a
   predefined name, then the structures must be allocated.
*/
static MPI_Datatype mpi_dtypes[] = {
    MPI_CHAR,
    MPI_UNSIGNED_CHAR,
    MPI_BYTE,
    MPI_WCHAR_T,
    MPI_SHORT,
    MPI_UNSIGNED_SHORT,
    MPI_INT,
    MPI_UNSIGNED,
    MPI_LONG,
    MPI_UNSIGNED_LONG,
    MPI_FLOAT,
    MPI_DOUBLE,
    MPI_LONG_DOUBLE,
    MPI_LONG_LONG_INT,
    MPI_LONG_LONG,
    MPI_PACKED,
    MPI_LB,
    MPI_UB,
    MPI_FLOAT_INT,
    MPI_DOUBLE_INT,
    MPI_LONG_INT,
    MPI_SHORT_INT,
    MPI_2INT,
    MPI_LONG_DOUBLE_INT,
/* Fortran types */
    MPI_COMPLEX,
    MPI_DOUBLE_COMPLEX,
    MPI_LOGICAL,
    MPI_REAL,
    MPI_DOUBLE_PRECISION,
    MPI_INTEGER,
    MPI_2INTEGER,
    MPI_2COMPLEX,
    MPI_2DOUBLE_COMPLEX,
    MPI_2REAL,
    MPI_2DOUBLE_PRECISION,
    MPI_CHARACTER,
};

void MPIR_Datatype_init( void )
{
    int i;
    MPID_Datatype *dptr;
    static int is_init = 0;
    
    if (is_init) return;
    {
	MPID_Common_thread_lock();
	if (!is_init) { 
	    for (i=0; i<MPID_DATATYPE_N_BUILTIN; i++) {
		dptr		   = &MPID_Datatype_builtin[i];
		dptr->handle	   = mpi_dtypes[i];
		dptr->is_permanent = 1;
		dptr->is_contig	   = 1;
		dptr->ref_count	   = 1;
		dptr->size	   = MPID_Datatype_get_size(mpi_dtypes[i]);
		dptr->extent	   = dptr->size;
		dptr->ub	   = dptr->size;
		dptr->true_ub	   = dptr->size;
		dptr->combiner	   = MPI_COMBINER_NAMED;
	    }
	    is_init = 1;
	}
	MPID_Common_thread_unlock();
    }
}
/* 
 * This routine computes the extent of a datatype.  
 * This routine handles not only the various upper bound and lower bound
 * markers but also the alignment rules set by the environment (the PAD).
 */
/* *** NOT DONE *** */
#ifdef FOO
MPI_Aint MPIR_Type_compute_extent( MPID_Datatype *datatype_ptr )
{
    /* Compute the \mpids{MPI_Datatype}{ub} */
    If a sticky ub exists for the old datatype (datatypes for struct) {
	use the \mpids{MPI_Datatype}{sticky_ub} and set the sticky ub flag
        (\mpids{MPI_Datatype}{MPID_TYPE_STICKY_UB}).
    }
    else {
        use the \mpids{MPI_Datatype}{true_ub}
    }
   /* Similar for the \mpids{MPI_Datatype}{lb},
   \mpids{MPI_Datatype}{sticky_lb},
   \mpids{MPI_Datatype}{MPID_TYPE_STICKY_LB}   and
   \mpids{MPI_Datatype}{true_lb}. */

   /* Determine PAD from alignment rules */
   datatype_ptr->extent = datatype_ptr->ub - datatype_ptr->lb + PAD;

   /* Similar for \mpids{MPI_Datatype}{true_extent} */
									       }
#endif
