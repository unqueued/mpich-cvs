/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * Send a synchronous eager message.  This is an optimization that you
 * may want to use for programs that make extensive use of MPI_Ssend and
 * MPI_Issend for short messages.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_EagerSyncNoncontigSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPIDI_CH3_EagerSyncNoncontigSend - Eagerly send noncontiguous data in
   synchronous mode.

   Some implementations may choose to use Rendezvous sends (see ch3u_rndv.c)
   for all Synchronous sends (MPI_Issend and MPI_Ssend).  An eager 
   synchronous send eliminates one of the handshake messages, but 
   most application codes should not be using synchronous sends in
   performance-critical operations.
*/
int MPIDI_CH3_EagerSyncNoncontigSend( MPID_Request **sreq_p, 
				      const void * buf, int count, 
				      MPI_Datatype datatype, MPIDI_msg_sz_t data_sz, 
				      int dt_contig, MPI_Aint dt_true_lb,
				      int rank, 
				      int tag, MPID_Comm * comm, 
				      int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
    MPIDI_VC_t * vc;
    MPID_Request *sreq = *sreq_p;
    
    sreq->cc = 2;
    sreq->dev.OnDataAvail = 0;
    
    MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
    es_pkt->match.rank = comm->rank;
    es_pkt->match.tag = tag;
    es_pkt->match.context_id = comm->context_id + context_offset;
    es_pkt->sender_req_id = sreq->handle;
    es_pkt->data_sz = data_sz;

    MPIDI_Comm_get_vc(comm, rank, &vc);
    
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    MPIU_DBG_MSGPKT(vc,tag,es_pkt->match.context_id,rank,data_sz,"EagerSync");

    if (dt_contig)
    {
        MPID_IOV iov[2];
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                                            "sending contiguous sync eager message, data_sz=" MPIDI_MSG_SZ_FMT, 
					    data_sz));
	
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)es_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*es_pkt);
	iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *)buf + dt_true_lb);
	iov[1].MPID_IOV_LEN = data_sz;	
	
	mpi_errno = MPIU_CALL(MPIDI_CH3,iSendv(vc, sreq, iov, 2));
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    *sreq_p = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }
    else
    {
	MPIU_DBG_MSG_D(CH3_OTHER,VERBOSE,
		       "sending non-contiguous sync eager message, data_sz=" MPIDI_MSG_SZ_FMT, 
		       data_sz);
	
	sreq->dev.segment_ptr = MPID_Segment_alloc( );
	/* if (!sreq->dev.segment_ptr) { MPIU_ERR_POP(); } */
	MPID_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
	sreq->dev.segment_first = 0;
	sreq->dev.segment_size = data_sz;
	
        mpi_errno = vc->sendNoncontig_fn(vc, sreq, es_pkt, sizeof(MPIDI_CH3_Pkt_eager_sync_send_t));
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    *sreq_p = NULL;
    goto fn_exit;
}

/* Send a zero-sized message with eager synchronous.  This is a temporary
   routine, as we may want to replace this with a counterpart to the
   Eager Short message */
int MPIDI_CH3_EagerSyncZero(MPID_Request **sreq_p, int rank, int tag, 
			    MPID_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
    MPIDI_VC_t * vc;
    MPID_Request *sreq = *sreq_p;
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending zero length message");
    
    sreq->cc = 2;
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
    sreq->dev.OnDataAvail = 0;
    
    MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
    es_pkt->match.rank = comm->rank;
    es_pkt->match.tag = tag;
    es_pkt->match.context_id = comm->context_id + context_offset;
    es_pkt->sender_req_id = sreq->handle;
    es_pkt->data_sz = 0;
    
    MPIDI_Comm_get_vc(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    MPIU_DBG_MSGPKT(vc,tag,es_pkt->match.context_id,rank,(MPIDI_msg_sz_t)0,"EagerSync0");
    mpi_errno = MPIU_CALL(MPIDI_CH3,iSend(vc, sreq, es_pkt, sizeof(*es_pkt)));
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	*sreq_p = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

 fn_exit:
    return mpi_errno;
}

/* 
 * These routines are called when a receive matches an eager sync send 
 */
int MPIDI_CH3_EagerSyncAck( MPIDI_VC_t *vc, MPID_Request *rreq )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
    MPID_Request * esa_req;
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending eager sync ack");
    MPIDI_Pkt_init(esa_pkt, MPIDI_CH3_PKT_EAGER_SYNC_ACK);
    esa_pkt->sender_req_id = rreq->dev.sender_req_id;
    mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, esa_pkt, sizeof(*esa_pkt), 
					      &esa_req));
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    if (esa_req != NULL)
    {
	MPID_Request_release(esa_req);
    }
 fn_fail:
    return mpi_errno;
}

/* 
 * Here are the routines that are called by the progress engine to handle
 * the various rendezvous message requests (cancel of sends is in 
 * mpid_cancel_send.c).
 */    

#define set_request_info(rreq_, pkt_, msg_type_)		\
{								\
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.rank;		\
    (rreq_)->status.MPI_TAG = (pkt_)->match.tag;		\
    (rreq_)->status.count = (pkt_)->data_sz;			\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}

int MPIDI_CH3_PktHandler_EagerSyncSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_eager_send_t * es_pkt = &pkt->eager_send;
    MPID_Request * rreq;
    int found;
    int complete;
    char *data_buf;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    
    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
     "received eager sync send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
	      es_pkt->sender_req_id, es_pkt->match.rank, es_pkt->match.tag, 
              es_pkt->match.context_id));
    MPIU_DBG_MSGPKT(vc,es_pkt->match.tag,es_pkt->match.context_id,
		    es_pkt->match.rank,es_pkt->data_sz,
		    "ReceivedEagerSync");
	    
    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&es_pkt->match, &found);
    if (rreq == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomemreq");
    }
    
    set_request_info(rreq, es_pkt, MPIDI_REQUEST_EAGER_MSG);

    data_len = ((*buflen - sizeof(MPIDI_CH3_Pkt_t) >= rreq->dev.recv_data_sz)
                ? rreq->dev.recv_data_sz : *buflen - sizeof(MPIDI_CH3_Pkt_t));
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    if (found)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
	MPID_Request * esa_req;

	if (rreq->dev.recv_data_sz == 0) {
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
	    MPIDI_CH3U_Request_complete(rreq);
	    *rreqp = NULL;
	}
	else {
	    mpi_errno = MPIDI_CH3U_Receive_data_found( rreq, data_buf,
                                                       &data_len, &complete );
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
		    "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SYNC_SEND");
	    }

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + data_len;

            if (complete) 
            {
                MPIDI_CH3U_Request_complete(rreq);
                *rreqp = NULL;
            }
            else
            {
                *rreqp = rreq;
            }
	}
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending eager sync ack");
	
	MPIDI_Pkt_init(esa_pkt, MPIDI_CH3_PKT_EAGER_SYNC_ACK);
	esa_pkt->sender_req_id = rreq->dev.sender_req_id;
	mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, esa_pkt, 
						  sizeof(*esa_pkt), &esa_req));
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|syncack");
	}
	if (esa_req != NULL) {
	    MPID_Request_release(esa_req);
	}
    }
    else
    {
	if (rreq->dev.recv_data_sz == 0) {
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
	    MPIDI_CH3U_Request_complete(rreq);
	    *rreqp = NULL;
	}
	else {
	    mpi_errno = MPIDI_CH3U_Receive_data_unexpected( rreq, data_buf,
                                                            &data_len, &complete );
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
		    "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SYNC_SEND");
	    }

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + data_len;

            if (complete) 
            {
                MPIDI_CH3U_Request_complete(rreq);
                *rreqp = NULL;
            }
            else
            {
                *rreqp = rreq;
            }
	}
	MPIDI_Request_set_sync_send_flag(rreq, TRUE);
    }
 fn_fail:
    return mpi_errno;
}

int MPIDI_CH3_PktHandler_EagerSyncAck( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				       MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_eager_sync_ack_t * esa_pkt = &pkt->eager_sync_ack;
    MPID_Request * sreq;
    
    MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
	   "received eager sync ack pkt, sreq=0x%08x", esa_pkt->sender_req_id);
	    
    MPID_Request_get_ptr(esa_pkt->sender_req_id, sreq);
    /* decrement CC (but don't mark data transfer as complete since the 
       transfer could still be in progress) */

    /* FIXME: This sometimes segfaults */
    MPIDI_CH3U_Request_complete(sreq);  /* brad : seen this segfault in ssm 
					   dynamic process...? */
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;
    return MPI_SUCCESS;
}

#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_EagerSyncSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_SEND\n"));
    MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_send.sender_req_id));
    MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_sync_send.match.context_id));
    MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_sync_send.match.tag));
    MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_sync_send.match.rank));
    MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_sync_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
    MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_sync_send.seqnum));
#endif
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_EagerSyncAck( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_ACK\n"));
    MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_ack.sender_req_id));
    return MPI_SUCCESS;
}
#endif
