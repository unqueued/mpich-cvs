/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
/* FIXME: This bsend header shouldn't be needed (the function prototype
   should be in mpiimpl.h), to allow a devices MPID_Startall to use the
   MPIR_Bsend_isend function */
#include "../../../mpi/pt2pt/bsendutil.h"

/* FIXME: Consider using function pointers for invoking persistent requests;
   if we made those part of the public request structure, the top-level routine
   could implement startall unless the device wanted to study the requests
   and reorder them */

/* FIXME: Where is the memory registration call in the init routines, 
   in case the channel wishes to take special action (such as pinning for DMA)
   the memory? This was part of the design. */

/* This macro initializes all of the fields in a persistent request */
#define MPIDI_Request_create_psreq(sreq_, mpi_errno_, FAIL_)		\
{									\
    (sreq_) = MPID_Request_create();				\
    if ((sreq_) == NULL)						\
    {									\
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"send request allocation failed");\
	(mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;				\
	FAIL_;								\
    }									\
									\
    MPIU_Object_set_ref((sreq_), 1);					\
    (sreq_)->cc   = 0;                                                  \
    (sreq_)->kind = MPID_PREQUEST_SEND;					\
    (sreq_)->comm = comm;						\
    MPIR_Comm_add_ref(comm);						\
    (sreq_)->dev.match.rank = rank;					\
    (sreq_)->dev.match.tag = tag;					\
    (sreq_)->dev.match.context_id = comm->context_id + context_offset;	\
    (sreq_)->dev.user_buf = (void *) buf;				\
    (sreq_)->dev.user_count = count;					\
    (sreq_)->dev.datatype = datatype;					\
    (sreq_)->partner_request = NULL;					\
}

	
/*
 * MPID_Startall()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Startall
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Startall(int count, MPID_Request * requests[])
{
    int i;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_STARTALL);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_STARTALL);

    for (i = 0; i < count; i++)
    {
	MPID_Request * const preq = requests[i];

	/* FIXME: The odd 7th arg (match.context_id - comm->context_id) 
	   is probably to get the context offset.  Do we really need the
	   context offset? Is there any case where the offset isn't zero? */
	switch (MPIDI_Request_get_type(preq))
	{
	    case MPIDI_REQUEST_TYPE_RECV:
	    {
		rc = MPID_Irecv(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->recvcontext_id,
		    &preq->partner_request);
		break;
	    }
	    
	    case MPIDI_REQUEST_TYPE_SEND:
	    {
		rc = MPID_Isend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_TYPE_RSEND:
	    {
		rc = MPID_Irsend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_TYPE_SSEND:
	    {
		rc = MPID_Issend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
		    preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }

	    case MPIDI_REQUEST_TYPE_BSEND:
	    {
		MPI_Request sreq_handle;
		MPIU_THREADPRIV_DECL;

		MPIU_THREADPRIV_GET;
		
		MPIR_Nest_incr();
		{
		    rc = NMPI_Ibsend(preq->dev.user_buf, preq->dev.user_count,
				     preq->dev.datatype, preq->dev.match.rank,
				     preq->dev.match.tag, preq->comm->handle, 
				     &sreq_handle);
		    if (rc == MPI_SUCCESS)
		    {
			MPID_Request_get_ptr(sreq_handle, preq->partner_request);
		    }
		}
		MPIR_Nest_decr();
		break;
	    }

	    default:
	    {
		/* --BEGIN ERROR HANDLING-- */
		rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
					  "**ch3|badreqtype %d", MPIDI_Request_get_type(preq));
		/* --END ERROR HANDLING-- */
	    }
	}
	
	if (rc == MPI_SUCCESS)
	{
	    preq->status.MPI_ERROR = MPI_SUCCESS;
	    preq->cc_ptr = &preq->partner_request->cc;
	}
	/* --BEGIN ERROR HANDLING-- */
	else
	{
	    /* If a failure occurs attempting to start the request, then we 
	       assume that partner request was not created, and stuff
	       the error code in the persistent request.  The wait and test
	       routines will look at the error code in the persistent
	       request if a partner request is not present. */
	    preq->partner_request = NULL;
	    preq->status.MPI_ERROR = rc;
	    preq->cc_ptr = &preq->cc;
	    preq->cc = 0;
	}
	/* --END ERROR HANDLING-- */
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
}

/* FIXME:
   Move the routines that initialize the persistent requests into this file,
   since startall must be used with all of them */

/*
 * MPID_Send_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Send_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		   MPID_Request ** request)
{
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEND_INIT);

    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEND_INIT);
    return mpi_errno;
}

/*
 * MPID_Ssend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Ssend_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Ssend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SSEND_INIT);

    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SSEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SSEND_INIT);
    return mpi_errno;
}

/*
 * MPID_Rsend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rsend_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Rsend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RSEND_INIT);

    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_RSEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RSEND_INIT);
    return mpi_errno;
}

/*
 * MPID_Bsend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Bsend_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Bsend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_BSEND_INIT);

    MPIDI_Request_create_psreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_BSEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;

  fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_BSEND_INIT);
    return mpi_errno;
}

/* 
 * FIXME: The ch3 implmentation of the persistent routines should
 * be very simple and use common code as much as possible.  All
 * persistent routine should be in the same file, along with 
 * startall.  Consider using function pointers to specify the 
 * start functions, as if these were generalized requests, 
 * rather than having MPID_Startall look at the request type.
 */
/*
 * MPID_Recv_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Recv_init(void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		   MPID_Request ** request)
{
    MPID_Request * rreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RECV_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RECV_INIT);
    
    rreq = MPID_Request_create();
    if (rreq == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	/* --END ERROR HANDLING-- */
	goto fn_exit;
    }
    
    MPIU_Object_set_ref(rreq, 1);
    rreq->kind = MPID_PREQUEST_RECV;
    rreq->comm = comm;
    rreq->cc   = 0;
    MPIR_Comm_add_ref(comm);
    rreq->dev.match.rank = rank;
    rreq->dev.match.tag = tag;
    rreq->dev.match.context_id = comm->recvcontext_id + context_offset;
    rreq->dev.user_buf = (void *) buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype = datatype;
    rreq->partner_request = NULL;
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_RECV);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
    }
    *request = rreq;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RECV_INIT);
    return mpi_errno;
}
