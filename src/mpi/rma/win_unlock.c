/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_unlock */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_unlock = PMPI_Win_unlock
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_unlock  MPI_Win_unlock
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_unlock as PMPI_Win_unlock
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_unlock PMPI_Win_unlock

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_unlock

/*@
   MPI_Win_unlock - Completes an RMA access epoch at the target process

   Input Parameters:
+ rank - rank of window (nonnegative integer) 
- win - window object (handle) 

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
@*/
int MPI_Win_unlock(int rank, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_unlock";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_UNLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_UNLOCK);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
                return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Win_unlock(rank, win_ptr);

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
	return MPI_SUCCESS;
    }

    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_win_unlock", "**mpi_win_unlock %d %W", rank, win);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
    return MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
}

