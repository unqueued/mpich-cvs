/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_subarray */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_subarray = PMPI_Type_create_subarray
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_subarray  MPI_Type_create_subarray
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_subarray as PMPI_Type_create_subarray
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_subarray PMPI_Type_create_subarray

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_subarray

/*@
   MPI_Type_create_subarray - create datatype subarray

   Arguments:
+  int ndims - ndims
.  int array_of_sizes[] - sizes
.  int array_of_subsizes[] - subsizes
.  int array_of_starts[] - starts
.  int order - order
.  MPI_Datatype oldtype - old datatype
-  MPI_Datatype *newtype - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_create_subarray(int ndims, int array_of_sizes[], int array_of_subsizes[], int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_subarray";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
    return MPI_SUCCESS;
}
