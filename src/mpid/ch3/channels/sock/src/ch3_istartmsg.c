/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static MPID_Request * create_request(void * hdr, MPIDI_msg_sz_t hdr_sz, MPIU_Size_t nb)
{
    MPID_Request * sreq;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST);

    sreq = MPIDI_CH3_Request_create();
    assert(sreq != NULL);
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    /* memcpy(&sreq->sc.pkt, hdr, hdr_sz); */
    assert(hdr_sz == sizeof(MPIDI_CH3_Pkt_t));
    sreq->sc.pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->ch3.iov[0].MPID_IOV_BUF = (char *) &sreq->sc.pkt + nb;
    sreq->ch3.iov[0].MPID_IOV_LEN = hdr_sz - nb;
    sreq->ch3.iov_count = 1;
    sreq->sc.iov_offset = 0;
    sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
    
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If the entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the header is copied into the request, and a pointer to the request is returned.
 * An error condition also results in a request be allocated and the errror being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg(MPIDI_VC * vc, void * hdr, MPIDI_msg_sz_t hdr_sz, MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The SOCK channel uses a fixed length header, the size of which is the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    if (vc->sc.state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
	{
	    MPIU_Size_t nb;
	    int rc;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	    
	    /* MT - need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               not also try to write */
	    rc = MPIDU_Sock_write(vc->sc.sock, hdr, hdr_sz, &nb);
	    if (rc == MPI_SUCCESS)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %ld bytes", (unsigned long) nb));
		
		if (nb == hdr_sz)
		{ 
		    MPIDI_DBG_PRINTF((55, FCNAME, "entire write complete, %d bytes", nb));
		    /* done.  get us out of here as quickly as possible. */
		}
		else
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "partial write of %d bytes, request enqueued at head", nb));
		    sreq = create_request(hdr, hdr_sz, nb);
		    assert(sreq != NULL);
		    MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		    mpi_errno = MPIDI_CH3I_VC_post_write(vc, sreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			MPID_Abort(NULL, mpi_errno, 13);
		    }
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - MPIDU_Sock_write failed, rc=%d", rc));
		sreq = MPIDI_CH3_Request_create();
		assert(sreq != NULL);
		sreq->kind = MPID_REQUEST_SEND;
		sreq->cc = 0;
		/* TODO: Create an appropriate error message based on the return value */
		sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send in progress, request enqueued"));
	    sreq = create_request(hdr, hdr_sz, 0);
	    assert(sreq != NULL);
	    MPIDI_CH3I_SendQ_enqueue(vc, sreq);
	}
    }
    else if (vc->sc.state == MPIDI_CH3I_VC_STATE_UNCONNECTED) /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  posting connect and enqueuing request"));
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(hdr, hdr_sz, 0);
	assert(sreq != NULL);
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);

	/* Form a new connection */
	MPIDI_CH3I_VC_post_connect(vc);
    }
    else if (vc->sc.state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "forming connection, request enqueued"));
	sreq = create_request(hdr, hdr_sz, 0);
	assert(sreq != NULL);
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }
    else
    {
	/* Connection failed, so allocate a request and return an error. */
	MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed"));
	sreq = MPIDI_CH3_Request_create();
	assert(sreq != NULL);
	sreq->kind = MPID_REQUEST_SEND;
	sreq->cc = 0;
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
    }

    *sreq_ptr = sreq;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
}
