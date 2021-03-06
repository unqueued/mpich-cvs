/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPIDI_CH3I_SendNoncontig - Sends a message by packing
   directly into cells.  The caller must initialize sreq->dev.segment
   as well as segment_first and segment_size. */
int MPIDI_CH3I_SendNoncontig( MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz )
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SENDNONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SENDNONCONTIG);

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)header);

    sreq->dev.OnFinal = 0;
    sreq->dev.OnDataAvail = 0;

    if (!MPIDI_CH3I_SendQ_empty(CH3_NORMAL_QUEUE)) /* MT */
    {
        /* send queue is not empty, just enqueue this request */

        MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));

	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
        sreq->ch.noncontig    = TRUE;
        sreq->ch.header_sz    = hdr_sz;
	sreq->ch.vc           = vc;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
        goto fn_exit;
    }

    /* send as many cells of data as you can */
    MPID_nem_mpich2_send_seg_header(sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size, header, hdr_sz, vc, &again);
    while(!again && sreq->dev.segment_first < sreq->dev.segment_size)
        MPID_nem_mpich2_send_seg(sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size, vc, &again);

    if (again)
    {
        /* we didn't finish sending everything */
        sreq->ch.noncontig = TRUE;
        sreq->ch.vc = vc;
        if (sreq->dev.segment_first == 0) /* nothing was sent, save header */
        {
            sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
            sreq->ch.header_sz    = hdr_sz;
        }
        else
        {
            /* part of message was sent, make this req an active send */
            MPIU_Assert(MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
            MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
        }
        MPIDI_CH3I_SendQ_enqueue(sreq, CH3_NORMAL_QUEUE);
        goto fn_exit;
    }

    /* finished sending all data, complete the request */
    if (!sreq->dev.OnDataAvail)
    {
        MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
        MPIDI_CH3U_Request_complete(sreq);
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, ".... complete %d bytes", (int) (sreq->dev.segment_size));
    }
    else
    {
        int complete = 0;
        mpi_errno = sreq->dev.OnDataAvail(vc, sreq, &complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_Assert(complete); /* all data has been sent, we should always complete */
        
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, ".... complete %d bytes", (int) (sreq->dev.segment_size));
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SENDNONCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
