/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Sendrecv_replace */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Sendrecv_replace = PMPI_Sendrecv_replace
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Sendrecv_replace  MPI_Sendrecv_replace
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Sendrecv_replace as PMPI_Sendrecv_replace
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Sendrecv_replace PMPI_Sendrecv_replace

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Sendrecv_replace

/*@
    MPI_Sendrecv_replace - Sends and receives using a single buffer

Input Parameters:
+ count - number of elements in send and receive buffer (integer) 
. datatype - type of elements in send and receive buffer (handle) 
. dest - rank of destination (integer) 
. sendtag - send message tag (integer) 
. source - rank of source (integer) 
. recvtag - receive message tag (integer) 
- comm - communicator (handle) 

Output Parameters:
+ buf - initial address of send and receive buffer (choice) 
- status - status object (Status) 

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_TRUNCATE
.N MPI_ERR_EXHAUSTED

@*/
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag,
			 MPI_Comm comm, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Sendrecv_replace";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SENDRECV_REPLACE);
    
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
	    
    MPID_MPI_PT2PT_FUNC_ENTER_BOTH(MPID_STATE_MPI_SENDRECV_REPLACE);
    
    /* Convert handles to MPI objects. */
    MPID_Comm_get_ptr(comm, comm_ptr);
    MPIR_Nest_incr();
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype * datatype_ptr = NULL;
	    
	    /* Validate communicator */
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            if (mpi_errno) {
		goto fn_exit;
            }
	    
            /* Validate datatype */
	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    MPIR_ERRTEST_USERBUFFER(buf, count, datatype, mpi_errno);
	    
	    /* Validate count */
	    MPIR_ERRTEST_COUNT(count, mpi_errno);

	    /* Validate status (status_ignore is not the same as null) */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);

	    /* Validate tags */
	    MPIR_ERRTEST_SEND_TAG(sendtag, mpi_errno);
	    MPIR_ERRTEST_RECV_TAG(recvtag, mpi_errno);

	    /* Validate source and destination */
	    if (comm_ptr) {
		MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
		MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    }
            if (mpi_errno) {
		goto fn_exit;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#   if defined(MPID_Sendrecv_replace)
    {
	mpi_errno = MPID_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm_ptr, status)
    }
#   else
    {
	MPID_Request * sreq;
	MPID_Request * rreq;
	void * tmpbuf = NULL;
	int tmpbuf_size;
	int tmpbuf_count;

	mpi_errno = NMPI_Pack_size(count, datatype, comm, &tmpbuf_size);
	if (mpi_errno != MPI_SUCCESS)
	{
	    goto blk_exit;
	}

	tmpbuf = MPIU_Malloc(tmpbuf_size);
	if (tmpbuf == NULL)
	{
	    mpi_errno = MPIR_ERR_MEMALLOCFAILED;
	    goto blk_exit;
	}

	tmpbuf_count = 0;
	mpi_errno = NMPI_Pack(buf, count, datatype, tmpbuf, tmpbuf_size, &tmpbuf_count, comm);
	if (mpi_errno != MPI_SUCCESS)
	{
	    goto blk_exit;
	}
	
	mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag, comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &rreq);
	if (mpi_errno)
	{
	    goto fn_exit;
	}

	mpi_errno = MPID_Isend(tmpbuf, tmpbuf_count, MPI_PACKED, dest, sendtag, comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &sreq);
	if (mpi_errno)
	{
	    /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
	    MPID_Request_release(rreq);
	    goto fn_exit;
	}

	while(1)
	{
	    MPID_Progress_start();
	
	    if (*sreq->cc_ptr != 0 || *rreq->cc_ptr != 0)
	    {
		MPID_Progress_wait();
	    }
	    else
	    {
		MPID_Progress_end();
		break;
	    }
	}

	if (status != MPI_STATUS_IGNORE)
	{
	    *status = rreq->status;
	}

	if (mpi_errno == MPI_SUCCESS)
	{
	    mpi_errno = rreq->status.MPI_ERROR;
	}
    
	if (mpi_errno == MPI_SUCCESS)
	{
	    mpi_errno = sreq->status.MPI_ERROR;
	}
    
	MPID_Request_release(sreq);
	MPID_Request_release(rreq);

      blk_exit:
	if (tmpbuf != NULL)
	{
	    MPIU_Free(tmpbuf);
	}
    }
#   endif

    MPIR_Nest_decr();
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SENDRECV_REPLACE);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS : MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}
