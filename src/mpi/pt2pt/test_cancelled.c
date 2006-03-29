/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Test_cancelled */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Test_cancelled = PMPI_Test_cancelled
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Test_cancelled  MPI_Test_cancelled
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Test_cancelled as PMPI_Test_cancelled
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Test_cancelled PMPI_Test_cancelled

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Test_cancelled

/*@
  MPI_Test_cancelled - Tests to see if a request was cancelled

Input Parameter:
. status - status object (Status) 

Output Parameter:
. flag - true if the request was cancelled, false otherwise (logical) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Test_cancelled(MPI_Status *status, int *flag)
{
    static const char FCNAME[] = "MPI_Test_cancelled";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TEST_CANCELLED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TEST_CANCELLED);
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL( status, "status", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( flag, "flag", mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    *flag = status->cancelled;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST_CANCELLED);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_test_cancelled",
	    "**mpi_test_cancelled %p %p", status, flag);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

