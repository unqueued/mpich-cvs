/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create = PMPI_Win_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create  MPI_Win_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create as PMPI_Win_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_create PMPI_Win_create
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_create

/*@
   MPI_Win_create - Create an MPI Window object for one-sided communication

   Input Parameters:
+ base - initial address of window (choice) 
. size - size of window in bytes (nonnegative integer) 
. disp_unit - local unit size for displacements, in bytes (positive integer) 
. info - info argument (handle) 
- comm - communicator (handle) 

  Output Parameter:
. win - window object returned by the call (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
@*/
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, 
		   MPI_Comm comm, MPI_Win *win)
{
    static const char FCNAME[] = "MPI_Win_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_CREATE);

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_CREATE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Comm_get_ptr( comm, comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Info *info_ptr = NULL;

            MPID_Info_get_ptr( info, info_ptr );

            /* Validate pointers */
	    MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
            if (size < 0)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_SIZE,
                                                  "**rmasize",
                                                  "**rmasize %d", size);  
            if (disp_unit <= 0)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
                                                 "**arg", "**arg %s", 
                                                 "disp_unit must be positive");  
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Win_create(base, size, disp_unit, info, comm_ptr, &win_ptr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_win_create", "**mpi_win_create %p %d %d %I %C %p", base, size, disp_unit, info, comm, win);
	MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);
	MPIR_Err_return_win(NULL, FCNAME, mpi_errno);
    }

    /* return the handle of the window object to the user */
    *win = win_ptr->handle;
    
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);
    return mpi_errno;
}
