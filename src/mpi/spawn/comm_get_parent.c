/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_parent */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_parent = PMPI_Comm_get_parent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_parent  MPI_Comm_get_parent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_parent as PMPI_Comm_get_parent
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_get_parent PMPI_Comm_get_parent

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_parent

/*@
   MPI_Comm_get_parent - short description

   Output Parameter:
. parent - the parent communicator (handle) 

   Notes:

 If a process was started with 'MPI_Comm_spawn' or 'MPI_Comm_spawn_multiple', 
 'MPI_Comm_get_parent' returns the parent intercommunicator of the current 
  process. This parent intercommunicator is created implicitly inside of 
 'MPI_Init' and is the same intercommunicator returned by 'MPI_Comm_spawn'
  in the parents. 

  If the process was not spawned, 'MPI_Comm_get_parent' returns 
  'MPI_COMM_NULL'.

  After the parent communicator is freed or disconnected, 'MPI_Comm_get_parent'
  returns 'MPI_COMM_NULL'. 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_get_parent(MPI_Comm *parent)
{
    static const char FCNAME[] = "MPI_Comm_get_parent";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_PARENT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_PARENT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(parent,"parent",mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_PARENT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_PARENT);
    return MPI_SUCCESS;
}
