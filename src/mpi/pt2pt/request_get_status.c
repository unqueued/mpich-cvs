/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"


/* -- Begin Profiling Symbol Block for routine MPI_Request_get_status */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Request_get_status = PMPI_Request_get_status
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Request_get_status  MPI_Request_get_status
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Request_get_status as PMPI_Request_get_status
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Request_get_status
#define MPI_Request_get_status PMPI_Request_get_status

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Request_get_status

/*@
   MPI_Request_get_status - Nondestructive test for the completion of a Request

Input Parameter:
.  request - request (handle)

Output Parameters:
+  flag - true if operation has completed (logical)
-  status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

   Notes:
   Unlike 'MPI_Test', 'MPI_Request_get_status' does not deallocate or
   deactivate the request.  A call to one of the test/wait routines or 
   'MPI_Request_free' should be made to release the request object.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Request_get_status";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REQUEST_GET_STATUS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REQUEST_GET_STATUS);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_REQUEST(request, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( request, request_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate request_ptr */
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (*request_ptr->cc_ptr != 0) {
	/* request not complete. poke the progress engine. Req #3130 */
	mpi_errno = MPID_Progress_test();
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    }
    
    if (*request_ptr->cc_ptr == 0)
    {
	switch(request_ptr->kind)
	{
        case MPID_REQUEST_SEND:
        {
            if (status != MPI_STATUS_IGNORE)
            {
                status->cancelled = request_ptr->status.cancelled;
            }
            mpi_errno = request_ptr->status.MPI_ERROR;
            break;
        }
        
        case MPID_REQUEST_RECV:
        {
            MPIR_Request_extract_status(request_ptr, status);
            mpi_errno = request_ptr->status.MPI_ERROR;
            break;
        }
        
        case MPID_PREQUEST_SEND:
        {
            MPID_Request * prequest_ptr = request_ptr->partner_request;
            
            if (prequest_ptr != NULL)
            {
		if (prequest_ptr->kind != MPID_UREQUEST)
		{
		    if (status != MPI_STATUS_IGNORE)
		    {
			status->cancelled = request_ptr->status.cancelled;
		    }
		    mpi_errno = prequest_ptr->status.MPI_ERROR;
		}
		else
		{
		    /* This is needed for persistent Bsend requests */
		    MPIU_THREADPRIV_DECL;
		    MPIU_THREADPRIV_GET;
		    MPIR_Nest_incr();
		    {
			int rc;
			
			rc = MPIR_Grequest_query(prequest_ptr);
			if (mpi_errno == MPI_SUCCESS)
			{
			    mpi_errno = rc;
			}
			if (status != MPI_STATUS_IGNORE)
			{
			    status->cancelled = prequest_ptr->status.cancelled;
			}
			if (mpi_errno == MPI_SUCCESS)
			{
			    mpi_errno = prequest_ptr->status.MPI_ERROR;
			}
		    }
		    MPIR_Nest_decr();
		}
            }
            else
            {
                if (request_ptr->status.MPI_ERROR != MPI_SUCCESS)
                {
                    /* if the persistent request failed to start then 
                       make the error code available */
                    if (status != MPI_STATUS_IGNORE)
                    {
                        status->cancelled = request_ptr->status.cancelled;
                    }
                    mpi_errno = request_ptr->status.MPI_ERROR;
                }
                else
                {
                    MPIR_Status_set_empty(status);
                }
            }
	    
            break;
        }
        
        case MPID_PREQUEST_RECV:
        {
            MPID_Request * prequest_ptr = request_ptr->partner_request;
            
            if (prequest_ptr != NULL)
            {
                MPIR_Request_extract_status(prequest_ptr, status);
                mpi_errno = prequest_ptr->status.MPI_ERROR;
            }
            else
            {
                /* if the persistent request failed to start then
                   make the error code available */
                mpi_errno = request_ptr->status.MPI_ERROR;
                MPIR_Status_set_empty(status);
            }
	    
            break;
        }

        case MPID_UREQUEST:
        {
	    MPIU_THREADPRIV_DECL;
	    MPIU_THREADPRIV_GET;
	    MPIR_Nest_incr();
	    {
		int rc;
		
		rc = MPIR_Grequest_query(request_ptr);
		if (mpi_errno == MPI_SUCCESS)
		{
		    mpi_errno = rc;
		}
		if (status != MPI_STATUS_IGNORE)
		{
		    status->cancelled = request_ptr->status.cancelled;
		}
		MPIR_Request_extract_status(request_ptr, status);
	    }
	    MPIR_Nest_decr();
	    
            break;
        }
        
        default:
            break;
	}

	*flag = TRUE;
    }
    else
    {
	*flag = FALSE;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    /* --END ERROR HANDLING-- */

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_GET_STATUS);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
	    FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_request_get_status",
	    "**mpi_request_get_status %R %p %p", request, flag, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
