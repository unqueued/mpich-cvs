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
   MPI_Test_cancelled - test cancelled

   Arguments:
+  MPI_Status *status - status
-  int *flag - flag

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Test_cancelled(MPI_Status *status, int *flag)
{
    static const char FCNAME[] = "MPI_Test_cancelled";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TEST_CANCELLED);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL( status, "status", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( flag, "flag", mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST_CANCELLED);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    *flag = status->cancelled;
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST_CANCELLED);
    return MPI_SUCCESS;
}

