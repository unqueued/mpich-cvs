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

/* -- Begin Profiling Symbol Block for routine MPI_Testall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Testall = PMPI_Testall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Testall  MPI_Testall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Testall as PMPI_Testall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Testall PMPI_Testall

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Testall

/*@
    MPI_Waitall - Waits for all given communications to complete

Input Parameters:
+ count - lists length (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameter:
. array_of_statuses - array of status objects (array of Status).  May be
  'MPI_STATUSES_NULL'

Notes:

If one or more of the requests completes with an error, MPI_ERR_IN_STATUS is
returned.  An error value will be present is elements of array_of_status
associated with the requests.  Likewise, the MPI_ERROR field in the status
elements associated with requests that have successfully completed will be
MPI_SUCCESS.  Finally, those requests that have not completed will have a value
of MPI_ERR_PENDING.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Testall";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = NULL;
    MPI_Status * status_ptr;
    int i;
    int n_completed;
    int active_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TESTALL);

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
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TESTALL);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests",
				 mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    /* NOTE: MPI_STATUSES_IGNORE != NULL */
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

    n_completed = 0;
    for (i = 0; i < count; i++)
    {
	if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	    MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
	    /* Validate object pointers if error checking is enabled */
#           ifdef HAVE_ERROR_CHECKING
	    {
		MPID_BEGIN_ERROR_CHECKS;
		{
		    MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
		    if (mpi_errno) {
			goto fn_exit;
		    }
		    
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif	    
	}
	else
	{
	    request_ptrs[i] = NULL;
	    n_completed += 1;
	}
    }

    if (n_completed == count)
    {
	*flag = TRUE;
	goto fn_exit;
    }
    
    MPID_Progress_test();
	    
    for (i = 0; i < count; i++)
    {
	if (request_ptrs[i] != NULL && *request_ptrs[i]->cc_ptr == 0)
	{
	    n_completed++;
	    if (request_ptrs[i]->status.MPI_ERROR != MPI_SUCCESS)
	    {
		mpi_errno = MPI_ERR_IN_STATUS;
	    }
	}
    }
    
    if (n_completed == count || mpi_errno == MPI_ERR_IN_STATUS)
    {
	for (i = 0; i < count; i++)
	{
	    if (request_ptrs[i] != NULL)
	    {
		if (*request_ptrs[i]->cc_ptr == 0) 
		{
		    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ?
			&array_of_statuses[i] : MPI_STATUS_IGNORE;
		    rc = MPIR_Request_complete(&array_of_requests[i],
					       request_ptrs[i], status_ptr,
					       &active_flag);
		    if (rc != MPI_SUCCESS)
		    {
			mpi_errno = MPI_ERR_IN_STATUS;
		    }
		}
		else
		{
		    array_of_statuses[i].MPI_ERROR = MPI_ERR_PENDING;
		}
	    }
	    else
	    {
		status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ?
		    &array_of_statuses[i] : MPI_STATUS_IGNORE;
		MPIR_Status_set_empty(status_ptr);
	    }
	}
    }
	
    *flag = (n_completed == count) ? TRUE : FALSE;

  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TESTALL);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
