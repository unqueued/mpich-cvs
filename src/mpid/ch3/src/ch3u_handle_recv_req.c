/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_recv_req()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iRead() are
 * allowed to perform additional up-calls if they complete the requested work
 * immediately. *** Care must be take to avoid deep recursion.  With some
 * thread packages, exceeding the stack space allocated to a thread can result
 * in overwriting the stack of another thread. ***
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Handle_recv_req(MPIDI_VC * vc, MPID_Request * rreq)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    assert(rreq->ch3.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(rreq->ch3.ca)
    {
	case MPIDI_CH3_CA_NONE:
	{
	    /* as the action name says, do nothing... */
	    break;
	}

	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    MPIDI_CH3U_Request_load_recv_iov(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    MPIDI_CH3U_Request_load_recv_iov(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    if (rreq->ch3.segment_first != rreq->ch3.recv_data_sz)
	    {
		rreq->status.count = rreq->ch3.segment_first;
		rreq->status.MPI_ERROR = MPI_ERR_UNKNOWN;
	    }
	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_EUBUF_AND_COMPLETE:
	{
	    MPIDI_CH3U_Request_unpack_uebuf(rreq);
	    MPIU_Free(rreq->ch3.tmpbuf);
	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_COMPLETE:
	{
	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	default:
	{
	    MPIDI_err_printf(FCNAME, "action %d UNIMPLEMENTED", rreq->ch3.ca);
	    abort();
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);
}

