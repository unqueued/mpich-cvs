/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Error_class */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Error_class = PMPI_Error_class
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Error_class  MPI_Error_class
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Error_class as PMPI_Error_class
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Error_class PMPI_Error_class

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Error_class

/*@
   MPI_Error_class - Converts an error code into an error class

Input Parameter:
. errorcode - Error code returned by an MPI routine 

Output Parameter:
. errorclass - Error class associated with 'errorcode'

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Error_class(int errorcode, int *errorclass)
{
    static const char FCNAME[] = "MPI_Error_class";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERROR_CLASS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERROR_CLASS);
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    /* ... body of routine ...  */
    /* We include the dynamic bit because this is needed to fully
       describe the dynamic error classes */
    *errorclass = errorcode & (ERROR_CLASS_MASK | ERROR_DYN_MASK);
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_CLASS);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_error_class", "**mpi_error_class %d %p", errorcode, errorclass);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_CLASS);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

