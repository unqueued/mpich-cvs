/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static MPID_Request * create_request(void * pkt, MPIDI_msg_sz_t pkt_sz, int nb)
{
    MPID_Request * sreq;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST);

    sreq = MPIDI_CH3_Request_create();
    if (sreq == NULL)
	return NULL;
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    /* memcpy(&sreq->ssm.pkt, pkt, pkt_sz); */
    assert(pkt_sz == sizeof(MPIDI_CH3_Pkt_t));
    sreq->ssm.pkt = *(MPIDI_CH3_Pkt_t *) pkt;
    sreq->ch3.iov[0].MPID_IOV_BUF = (char *) &sreq->ssm.pkt + nb;
    sreq->ch3.iov[0].MPID_IOV_LEN = pkt_sz - nb;
    sreq->ch3.iov_count = 1;
    sreq->ssm.iov_offset = 0;
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
int MPIDI_CH3_iStartMsg(MPIDI_VC * vc, void * pkt, MPIDI_msg_sz_t pkt_sz, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert(pkt_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The MM channel uses a fixed length header, the size of which is the maximum of all possible packet headers */
    pkt_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)pkt);
    
    if (vc->ssm.state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
	{
	    int nb;
	    int rc;

	    /* MT - need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               not also try to write */
	    if (vc->ssm.bShm)
	    {
		rc = MPIDI_CH3I_SHM_write(vc, pkt, pkt_sz, &nb);
	    }
	    else
	    {
		rc = sock_write(vc->ssm.sock, pkt, pkt_sz, &nb);
	    }
	    if (rc == SOCK_SUCCESS)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));
		
		if (nb == pkt_sz)
		{ 
		    MPIDI_DBG_PRINTF((55, FCNAME, "entire write complete, %d bytes", nb));
		    /* done.  get us out of here as quickly as possible. */
		}
		else
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "partial write of %d bytes, request enqueued at head", nb));
		    sreq = create_request(pkt, pkt_sz, nb);
		    if (sreq == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			return mpi_errno;
		    }
		    MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		    if (vc->ssm.bShm)
			vc->ssm.send_active = sreq;
		    else
			MPIDI_CH3I_SSM_VC_post_write(vc, sreq);
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed, rc=%d", rc));
		sreq = MPIDI_CH3_Request_create();
		if (sreq == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		    return mpi_errno;
		}
		sreq->kind = MPID_REQUEST_SEND;
		sreq->cc = 0;
		/* TODO: Create an appropriate error message based on the return value */
		sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send in progress, request enqueued"));
	    sreq = create_request(pkt, pkt_sz, 0);
	    if (sreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		return mpi_errno;
	    }
	    MPIDI_CH3I_SendQ_enqueue(vc, sreq);
	}
    }
    else if (vc->ssm.state == MPIDI_CH3I_VC_STATE_UNCONNECTED) /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  posting connect and enqueuing request"));
	
	/* Form a new connection */
	/*MPIDI_CH3I_VC_post_connect(vc);*/

	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(pkt, pkt_sz, 0);
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);

	MPIDI_CH3I_VC_post_connect(vc);
    }
    else if (vc->ssm.state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "forming connection, request enqueued"));
	sreq = create_request(pkt, pkt_sz, 0);
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }
    else
    {
	/* Connection failed, so allocate a request and return an error. */
	MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed"));
	sreq = MPIDI_CH3_Request_create();
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
	sreq->kind = MPID_REQUEST_SEND;
	sreq->cc = 0;
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
    }

    *sreq_ptr = sreq;

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return MPI_SUCCESS;
}
