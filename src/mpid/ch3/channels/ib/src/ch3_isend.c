/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* static void update_request(MPID_Request * sreq, void * hdr, int hdr_sz, int nb) */
#undef update_request
#ifdef MPICH_DBG_OUTPUT
#define update_request(sreq, pkt, pkt_sz, nb) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    if (pkt_sz != sizeof(MPIDI_CH3_Pkt_t)) \
    { \
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0); \
	MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND); \
	return mpi_errno; \
    } \
    sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) pkt; \
    sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt + nb; \
    sreq->dev.iov[0].MPID_IOV_LEN = pkt_sz - nb; \
    sreq->dev.iov_count = 1; \
    sreq->ch.iov_offset = 0; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}
#else
#define update_request(sreq, pkt, pkt_sz, nb) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) pkt; \
    sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt + nb; \
    sreq->dev.iov[0].MPID_IOV_LEN = pkt_sz - nb; \
    sreq->dev.iov_count = 1; \
    sreq->ch.iov_offset = 0; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSend(MPIDI_VC * vc, MPID_Request * sreq, void * pkt, MPIDI_msg_sz_t pkt_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

    MPIU_DBG_PRINTF(("ch3_isend\n"));
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    if (pkt_sz > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
	return mpi_errno;
    }
#endif

    /* The IB implementation uses a fixed length header, the size of which is the maximum of all possible packet headers */
    pkt_sz = sizeof(MPIDI_CH3_Pkt_t);
    
    if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
    {
	int nb;

	MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));

	/* MT: need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
	   also try to write */

	MPIU_DBG_PRINTF(("ibu_write(%d bytes)\n", pkt_sz));
	mpi_errno = ibu_write(vc->ch.ibu, pkt, pkt_sz, &nb);
	if (mpi_errno == MPI_SUCCESS)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));

	    if (nb == pkt_sz)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "write complete, calling MPIDI_CH3U_Handle_send_req()"));
		MPIDI_CH3U_Handle_send_req(vc, sreq, &complete);
		if (!complete)
		{
		    MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		    vc->ch.send_active = sreq;
		}
		else
		{
		    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "partial write, enqueuing at head"));
		update_request(sreq, pkt, pkt_sz, nb);
		MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		vc->ch.send_active = sreq;
	    }
	}
	else
	{
	    sreq->status.MPI_ERROR = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibwrite", 0);
	    MPIDI_CH3U_Request_complete(sreq);
	    mpi_errno = MPI_SUCCESS;
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "send queue not empty, enqueuing"));
	update_request(sreq, pkt, pkt_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
    return mpi_errno;
}
