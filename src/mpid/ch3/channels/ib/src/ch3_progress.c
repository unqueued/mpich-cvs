/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

static inline int handle_read(MPIDI_VC *vc, int nb);
static inline int handle_written(MPIDI_VC * vc);

#ifndef MPICH_SINGLE_THREADED
void MPIDI_CH3_Progress_start()
{
    /* MT - This function is empty for the single-threaded implementation */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    ibu_wait_t out;
    unsigned register count;
    unsigned completions = MPIDI_CH3I_progress_completions;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    do
    {
	mpi_errno = ibu_wait(MPIDI_CH3I_Process.set, 0, &out);
	if (mpi_errno != IBU_SUCCESS)
	{
	    MPIU_Internal_error_printf("ibu_wait returned IBU_FAIL, error %d\n", out.error);
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibu_wait", "**ibu_wait %d", mpi_errno);
	    goto fn_exit;
	}
	switch (out.op_type)
	{
	case IBU_OP_TIMEOUT:
	    /*MPIDU_Yield();*/
	    /*sched_yield();*/
	    break;
	case IBU_OP_READ:
	    MPIDI_DBG_PRINTF((50, FCNAME, "ibu_wait reported %d bytes read", out.num_bytes));
	    mpi_errno = handle_read(out.user_ptr, out.num_bytes);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
	    }
	    break;
	case IBU_OP_WRITE:
	    MPIDI_DBG_PRINTF((50, FCNAME, "ibu_wait reported %d bytes written", out.num_bytes));
	    mpi_errno = handle_written(out.user_ptr);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
	    }
	    break;
	case IBU_OP_CLOSE:
	    break;
	default:
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibu_op", "**ibu_op %d", out.op_type);
	    goto fn_exit;
	}

	/* pound on the write queues since ibu_wait currently does not return IBU_OP_WRITE */
	for (i=0; i<MPIDI_CH3I_Process.pg->size; i++)
	{
	    if (MPIDI_CH3I_Process.pg->vc_table[i].ch.send_active != NULL)
		/*if (i != MPIR_Process.comm_world->rank)*/
	    {
		mpi_errno = handle_written(&MPIDI_CH3I_Process.pg->vc_table[i]);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
		    return mpi_errno;
		}
	    }
	}
    } 
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);

fn_exit:
#ifdef MPICH_DBG_OUTPUT
    count = MPIDI_CH3I_progress_completions - completions;
    if (is_blocking)
    {
	MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", count));
    }
    else
    {
	if (count > 0)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "exiting (non-blocking), count=%d", count));
	}
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_poke()
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    mpi_errno = MPIDI_CH3I_Progress(0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    return mpi_errno;
}

#ifndef MPICH_SINGLE_THREADED
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT - This function is empty for the single-threaded implementation */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);

    /*
     * Wait for any pending communication to complete.  This prevents another process from hanging if it performs a send and then
     * attempts to cancel it.
     */
    MPIR_Nest_incr();
    {
	mpi_errno = NMPI_Barrier(MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_finalize", 0);
	}
    }
    MPIR_Nest_decr();
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    return mpi_errno;
}

/*
 * MPIDI_CH3I_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the iovec has been consumed, return true; otherwise return
 * false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
{
    int offset = req->ch.iov_offset;
    const int count = req->dev.iov_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    
    while (offset < count)
    {
	if (req->dev.iov[offset].MPID_IOV_LEN <= (unsigned int)nb)
	{
	    nb -= req->dev.iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    req->dev.iov[offset].MPID_IOV_BUF = (char*)req->dev.iov[offset].MPID_IOV_BUF + nb;
	    req->dev.iov[offset].MPID_IOV_LEN -= nb;
	    req->ch.iov_offset = offset;
	    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning FALSE"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->ch.iov_offset = offset;

    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning TRUE"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

#define post_pkt_recv(vc) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_POST_PKT_RECV); \
    MPIDI_FUNC_ENTER(MPID_STATE_POST_PKT_RECV); \
    vc->ch.req->dev.iov[0].MPID_IOV_BUF = (void *)&vc->ch.req->ch.pkt; \
    vc->ch.req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t); \
    vc->ch.req->dev.iov_count = 1; \
    vc->ch.req->ch.iov_offset = 0; \
    vc->ch.req->dev.ca = MPIDI_CH3I_CA_HANDLE_PKT; \
    vc->ch.recv_active = vc->ch.req; \
    /*mpi_errno = */ibu_post_read(vc->ch.ibu, &vc->ch.req->ch.pkt, sizeof(MPIDI_CH3_Pkt_t), NULL); \
    MPIDI_FUNC_EXIT(MPID_STATE_POST_PKT_RECV); \
}

#undef FUNCNAME
#define FUNCNAME handle_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_read(MPIDI_VC *vc, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * req;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_READ);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    req = vc->ch.recv_active;
    if (req == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
	return mpi_errno;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */
	    MPIDI_CA_t ca = req->dev.ca;
	    
	    vc->ch.recv_active = NULL;
	    
	    /*if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
	    {
		MPIDI_CH3_Pkt_t * pkt = &req->ch.pkt;
		
		if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "received CH3 packet %d, calllng CH3U_Handle_recv_pkt()", pkt->type));
		    MPIDI_CH3U_Handle_recv_pkt(vc, pkt);
		    MPIDI_DBG_PRINTF((65, FCNAME, "CH3U_Handle_recv_pkt() returned"));
		    if (vc->ch.recv_active == NULL)
		    {
			MPIDI_DBG_PRINTF((65, FCNAME, "complete; posting new recv packet"));
			post_pkt_recv(vc);
			MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
			return mpi_errno;
		    }
		}
	    }
	    else */ if (ca == MPIDI_CH3_CA_COMPLETE)
	    {
		MPIDI_DBG_PRINTF((65, FCNAME, "received requested data, decrementing CC"));
		/* mark data transfer as complete adn decrment CC */
		req->dev.iov_count = 0;
		MPIDI_CH3U_Request_complete(req);
		post_pkt_recv(vc);
		MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
		return mpi_errno;
	    }
	    else if (ca < MPIDI_CH3_CA_END_CH3)
	    {
		/* XXX - This code assumes that if another read is not posted by the device during the callback, then the
		   device is not expecting any more data for request.  As a result, the channels posts a read for another
		   packet */
		MPIDI_DBG_PRINTF((65, FCNAME, "finished receiving iovec, calling CH3U_Handle_recv_req()"));
		MPIDI_CH3U_Handle_recv_req(vc, req);
		if (vc->dev.iov_count == 0)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "request (assumed) complete, posting new recv packet"));
		    post_pkt_recv(vc);
		    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
		    return mpi_errno;
		}
	    }
	    else
	    {
#ifdef MPICH_DBG_OUTPUT
		/*
		assert(ca != MPIDI_CH3I_CA_HANDLE_PKT);
		assert(ca < MPIDI_CH3_CA_END_CH3);
		*/
		if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
		    return mpi_errno;
		}
		if (ca >= MPIDI_CH3_CA_END_CH3)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
		    return mpi_errno;
		}
#endif
	    }
	}
	else
	{
#ifdef MPICH_DBG_OUTPUT
	    /*assert(req->ch.iov_offset < req->dev.iov_count);*/
	    if (req->ch.iov_offset >= req->dev.iov_count)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->ch.iov_offset, req->dev.iov_count);
	    }
#endif
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
	    return mpi_errno;
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d",
	    req->dev.iov + req->ch.iov_offset, req->dev.iov_count - req->ch.iov_offset));
    }
    
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME handle_written
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_written(MPIDI_VC * vc)
{
    int nb, mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_WRITTEN);
    
    /*MPIDI_DBG_PRINTF((60, FCNAME, "entering"));*/
    while (vc->ch.send_active != NULL)
    {
	MPID_Request * req = vc->ch.send_active;

	/*
	if (req->ch.iov_offset >= req->dev.iov_count)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "iov_offset(%d) >= iov_count(%d)", req->ch.iov_offset, req->dev.iov_count));
	}
	*/
#ifdef MPICH_DBG_OUTPUT
	/*assert(req->ch.iov_offset < req->dev.iov_count);*/
	if (req->ch.iov_offset >= req->dev.iov_count)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->ch.iov_offset, req->dev.iov_count);
	    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
	    return mpi_errno;
	}
#endif
	/*MPIDI_DBG_PRINTF((60, FCNAME, "calling ibu_post_writev"));*/
	nb = ibu_writev(vc->ch.ibu, req->dev.iov + req->ch.iov_offset, req->dev.iov_count - req->ch.iov_offset);
	MPIDI_DBG_PRINTF((60, FCNAME, "ibu_post_writev returned %d", nb));

	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		MPIDI_CA_t ca = req->dev.ca;
			
		vc->ch.send_active = NULL;
		
		if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "sent requested data, decrementing CC"));
		    MPIDI_CH3I_SendQ_dequeue(vc);
		    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		    /* mark data transfer as complete and decrment CC */
		    req->dev.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		}
		else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->ch.pkt;
		    
		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			MPIDI_DBG_PRINTF((65, FCNAME, "setting ch.send_active"));
			vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		    }
		    else
		    {
			MPIDI_DBG_PRINTF((71, FCNAME, "unknown packet type %d", pkt->type));
		    }
		}
		else if (ca < MPIDI_CH3_CA_END_CH3)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "finished sending iovec, calling CH3U_Handle_send_req()"));
		    MPIDI_CH3U_Handle_send_req(vc, req);
		    if (vc->dev.iov_count == 0)
		    {
			/* NOTE: This code assumes that if another write is not posted by the device during the callback, then the
			   device has completed the current request.  As a result, the current request is dequeded and next request
			   in the queue is processed. */
			MPIDI_DBG_PRINTF((65, FCNAME, "request (assumed) complete"));
			MPIDI_DBG_PRINTF((65, FCNAME, "dequeuing req and posting next send"));
			if (MPIDI_CH3I_SendQ_head(vc) == req)
			{
			    MPIDI_CH3I_SendQ_dequeue(vc);
			}
			vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		    }
		}
		else
		{
#ifdef MPICH_DBG_OUTPUT
		    MPIDI_DBG_PRINTF((65, FCNAME, "ca = %d", ca));
		    /*assert(ca < MPIDI_CH3I_CA_END_IB);*/
		    if (ca >= MPIDI_CH3I_CA_END_IB)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
			return mpi_errno;
		    }
#endif
		}
	    }
	    else
	    {
#ifdef MPICH_DBG_OUTPUT
		MPIDI_DBG_PRINTF((65, FCNAME, "iovec updated by %d bytes but not complete", nb));
		/*assert(req->ch.iov_offset < req->dev.iov_count);*/
		if (req->ch.iov_offset >= req->dev.iov_count)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->ch.iov_offset, req->dev.iov_count);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
		    return mpi_errno;
		}
#endif
		break;
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((65, FCNAME, "ibu_post_writev returned %d bytes", nb));
	    break;
	}
    }

    /*MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));*/

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
    return mpi_errno;
}
