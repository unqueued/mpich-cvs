/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Isend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Isend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Isend(const void * buf, int count, MPI_Datatype datatype, int rank,
	       int tag, MPID_Comm * comm, int context_offset,
               MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));
    
    sreq = MPIDI_CH3_Request_create();
    if (sreq == NULL)
    {
	MPIDI_ERR_PRINTF((FCNAME, "send request allocation failed"));
	mpi_errno = MPI_ERR_NOMEM;
	goto fn_exit;
    }
    
    sreq->ref_count = 2;
    sreq->kind = MPID_REQUEST_SEND;
    sreq->comm = comm;
    sreq->ch3.match.rank = rank;
    sreq->ch3.match.tag = tag;
    sreq->ch3.match.context_id = comm->context_id + context_offset;
    sreq->ch3.user_buf = (void *) buf;
    sreq->ch3.user_count = count;
    sreq->ch3.datatype = datatype;
    sreq->ch3.vc = comm->vcr[rank];
    
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    {
	dt_contig = TRUE;
	data_sz = count * MPID_Datatype_get_basic_size(datatype);
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, dt_contig,
			  MPID_Datatype_get_basic_size(datatype), data_sz));
    }
    else
    {
	MPID_Datatype * dtp;
	
	MPID_Datatype_get_ptr(datatype, dtp);
	dt_contig = dtp->is_contig;
	data_sz = count * dtp->size;
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, dt_contig,
			  dtp->size, data_sz));
    }

    if (data_sz == 0)
    {
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;

	    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
	    sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending zero length message"));
	    eager_pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	    eager_pkt->match.rank = comm->rank;
	    eager_pkt->match.tag = tag;
	    eager_pkt->match.context_id = comm->context_id + context_offset;
	    eager_pkt->sender_req_id = sreq->handle;
	    eager_pkt->data_sz = 0;

	    MPIDI_CH3_iSend(comm->vcr[rank], sreq, eager_pkt,
			    sizeof(*eager_pkt));

	    goto fn_exit;
    }
    
    /* TODO - flow control: limit number of outstanding eager messsages */
    
    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_send_t) <=
	MPIDI_CH3_EAGER_MAX_MSG_SIZE)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
	MPID_IOV iov[MPID_IOV_LIMIT];
	int iov_n;
	
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
	
	eager_pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	eager_pkt->match.rank = comm->rank;
	eager_pkt->match.tag = tag;
	eager_pkt->match.context_id = comm->context_id + context_offset;
	eager_pkt->sender_req_id = sreq->handle;
	eager_pkt->data_sz = data_sz;
	
	iov[0].MPID_IOV_BUF = eager_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);
	
	if (dt_contig)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending contiguous eager message, "
			      "data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	    
	    iov[1].MPID_IOV_BUF = (void *) buf;
	    iov[1].MPID_IOV_LEN = data_sz;
	    iov_n = 2;
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous eager "
			      "message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    MPID_Segment_init(buf, count, datatype, &sreq->ch3.segment);
	    sreq->ch3.segment_first = 0;
	    sreq->ch3.segment_size = data_sz;
	    
	    iov_n = MPID_IOV_LIMIT - 1;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(
		sreq, &iov[1], &iov_n);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Request_release(sreq);
		sreq = NULL;
		goto fn_exit;
	    }
	    iov_n += 1;
	}
	
	MPIDI_CH3_iSendv(sreq->ch3.vc, sreq, iov, iov_n);
    }
    else
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const rts_pkt =
	    &upkt.rndv_req_to_send;
	    
	MPIDI_DBG_PRINTF((15, FCNAME, "sending rndv RTS, data_sz="
			  MPIDI_MSG_SZ_FMT, data_sz));
	    
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_RNDV_MSG);
	sreq->ch3.ca = MPIDI_CH3_CA_NONE;
	
	rts_pkt->type = MPIDI_CH3_PKT_RNDV_REQ_TO_SEND;
	rts_pkt->match.rank = comm->rank;
	rts_pkt->match.tag = tag;
	rts_pkt->match.context_id = comm->context_id + context_offset;
	rts_pkt->sender_req_id = sreq->handle;
	rts_pkt->data_sz = data_sz;

	MPIDI_CH3_iSend(comm->vcr[rank], sreq, rts_pkt, sizeof(*rts_pkt));
    }

  fn_exit:
    *request = sreq;
    if (sreq)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x",
			  sreq->handle));
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    return mpi_errno;
}

