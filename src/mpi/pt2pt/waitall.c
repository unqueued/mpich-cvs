/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPID_REQUEST_PTR_ARRAY_SIZE)
#define MPID_REQUEST_PTR_ARRAY_SIZE 16
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Waitall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitall = PMPI_Waitall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitall  MPI_Waitall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitall as PMPI_Waitall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Waitall PMPI_Waitall

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitall

/*@
    MPI_Waitall - Waits for all given communications to complete

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of request handles (array of handles)

Output Parameter:
. array_of_statuses - array of status objects (array of Statuses).  May be
  'MPI_STATUSES_NULL'

Notes:

XXX - MPI_ERR_PENDING should be explained here.  It should not have been listed
as a possible error return code.

.N waitstatus

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitall(int count, MPI_Request array_of_requests[],
		MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Waitall";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = NULL;
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITALL);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITALL);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests",
				 mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses",
				 mpi_errno);
	    if (array_of_requests != NULL && count > 0)
	    {
		for (i = 0; i < count; i++)
		{
		    MPIR_ERRTEST_REQUEST(array_of_requests[i], mpi_errno);
		}
	    }
            if (mpi_errno) {
                goto fn_exit;
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI request handles to a request object pointers */
    if (count <= MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	request_ptrs = request_ptr_array;
    }
    else
    {
	request_ptrs = MPIU_Malloc(count * sizeof(MPID_Request *));
	if (request_ptrs == NULL)
	{
	    mpi_errno = MPI_ERR_NOMEM;
	    goto fn_exit;
	}
    }

    for (i = 0; i < count; i++)
    {
	MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
    }
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    for (i = 0; i < count; i++)
	    {
		MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
	    }
            if (mpi_errno) {
		goto fn_exit;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    for(;;)
    {
	int n_completed;
	int error_flag;
	
	MPID_Progress_start();

	n_completed = 0;
	error_flag = FALSE;
	for (i = 0; i < count; i++)
	{
	    if (request_ptrs[i] != NULL)
	    {
		if ((*request_ptrs[i]->cc_ptr) == 0)
		{
		    MPI_Status * status_ptr;
		    int rc;
		    
		    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ?
			&array_of_statuses[i] : MPI_STATUS_IGNORE;
		    rc = MPIR_Request_complete(&array_of_requests[i],
					       request_ptrs[i],
					       status_ptr);
		    if (rc != MPI_SUCCESS)
		    {
			error_flag = TRUE;
			mpi_errno = MPI_ERR_IN_STATUS;
		    }
		    
		    request_ptrs[i] = NULL;
		    n_completed++;
		}
	    }
	    else
	    {
		n_completed++;
	    }
	}
	
	if (n_completed == count)
	{
	    MPID_Progress_end();
	    break;
	}
	else if (error_flag)
	{
	    MPID_Progress_end();
	    for (i = 0; i < count; i++)
	    {
		if (request_ptrs[i] != NULL)
		{
		    request_ptrs[i]->status.MPI_ERROR = MPI_ERR_PENDING;
		}
	    }
	    break;
	}

	MPID_Progress_wait();
    }

  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITALL);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}

