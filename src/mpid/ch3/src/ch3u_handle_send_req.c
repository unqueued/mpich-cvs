/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_send_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_send_req(MPIDI_VC * vc, MPID_Request * sreq, int *complete)
{
    static int in_routine = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);

    assert(in_routine == FALSE);
    in_routine = TRUE;
    
    assert(sreq->dev.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(sreq->dev.ca)
    {
	case MPIDI_CH3_CA_COMPLETE:
	{
            if (MPIDI_Request_get_type(sreq) == MPIDI_REQUEST_TYPE_GET_RESP)
	    { 
                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (sreq->dev.decr_ctr != NULL)
                    *(sreq->dev.decr_ctr) -= 1;
            }

	    /* mark data transfer as complete and decrement CC */
	    MPIDI_CH3U_Request_complete(sreq);
	    *complete = 1;

	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    sreq->dev.iov_count = MPID_IOV_LIMIT;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, sreq->dev.iov, &sreq->dev.iov_count);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|loadsendiov", 0);
		goto fn_exit;
	    }
	    
	    *complete = 0;
	    break;
	}
	
	default:
	{
	    *complete = 0;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badca",
					     "**ch3|badca %d", sreq->dev.ca);
	    break;
	}
    }

  fn_exit:
    in_routine = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);
    return mpi_errno;
}

