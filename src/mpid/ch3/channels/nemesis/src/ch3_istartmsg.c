/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
/*#include "mpidpre.h"*/
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif


/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If
 * the entire message is successfully sent, then NULL is returned.
 * Otherwise a request is allocated, the header is copied into the
 * request, and a pointer to the request is returned.  An error
 * condition also results in a request be allocated and the errror
 * being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg (MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    if (((MPIDI_CH3I_VC *)vc->channel_private)->iStartContigMsg)
    {
        mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->iStartContigMsg(vc, hdr, hdr_sz, NULL, 0, sreq_ptr);
        goto fn_exit;
    }

    /*MPIU_Assert(((MPIDI_CH3I_VC *)vc->channel_private)->is_local);*/
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* This channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE))
       /* MT */
    {
	MPIU_DBG_MSG_D (CH3_CHANNEL, VERBOSE, "iStartMsg %d", (int) hdr_sz);
	mpi_errno = MPID_nem_mpich2_send_header (hdr, hdr_sz, vc, &again);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	if (again)
	{
	    goto enqueue_it;
	}
	else
	{
	    *sreq_ptr = NULL;
	}
    }
    else
    {
	goto enqueue_it;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
    
 enqueue_it:
    {
	MPID_Request * sreq = NULL;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));

	/* create a request */
	sreq = MPID_Request_create();
	MPIU_Assert (sreq != NULL);
	MPIU_Object_set_ref (sreq, 2);
	sreq->kind = MPID_REQUEST_SEND;

	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) hdr;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->dev.pending_pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;
	sreq->dev.iov_count = 1;
	sreq->dev.iov_offset = 0;
        sreq->ch.noncontig = FALSE;
	sreq->ch.vc = vc;
	sreq->dev.OnDataAvail = 0;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
	*sreq_ptr = sreq;
	
	goto fn_exit;
    }
}


