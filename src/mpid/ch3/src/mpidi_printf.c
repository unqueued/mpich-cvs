/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <stdio.h>
#include <stdarg.h>

/* style: allow:vprintf:1 sig:0 */
/* style: allow:printf:2 sig:0 */

#undef MPIDI_dbg_printf
void MPIDI_dbg_printf(int level, char * func, char * fmt, ...)
{
    MPID_Common_thread_lock();
    {
	va_list list;

	if (MPIR_Process.comm_world)
	{
            MPIU_dbglog_printf("[%d] %s(): ", MPIR_Process.comm_world->rank, func);
	}
	else
	{
	    MPIU_dbglog_printf("[-1] %s(): ", func);
	}
	va_start(list, fmt);
	MPIU_dbglog_vprintf(fmt, list);
	va_end(list);
	MPIU_dbglog_printf("\n");
	fflush(stdout);
    }
    MPID_Common_thread_unlock();
}

#undef MPIDI_err_printf
void MPIDI_err_printf(char * func, char * fmt, ...)
{
    MPID_Common_thread_lock();
    {
	va_list list;

	if (MPIR_Process.comm_world)
	{
	    printf("[%d] ERROR - %s(): ", MPIR_Process.comm_world->rank, func);
	}
	else
	{
	    printf("[-1] ERROR - %s(): ", func);
	}
	va_start(list, fmt);
	vprintf(fmt, list);
	va_end(list);
	printf("\n");
	fflush(stdout);
    }
    MPID_Common_thread_unlock();
}

#ifdef MPICH_DBG_OUTPUT
void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt)
{
    MPID_Common_thread_lock();
    {
	MPIU_DBG_PRINTF(("MPIDI_CH3_Pkt_t:\n"));
	switch(pkt->type)
	{
	    case MPIDI_CH3_PKT_EAGER_SEND:
		MPIU_DBG_PRINTF((" type ......... EAGER_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
		MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_sync_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_sync_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_sync_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_sync_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_sync_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
		MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_ACK\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_ack.sender_req_id));
		break;
	    case MPIDI_CH3_PKT_READY_SEND:
		MPIU_DBG_PRINTF((" type ......... READY_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->ready_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->ready_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->ready_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->ready_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->ready_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->ready_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
		MPIU_DBG_PRINTF((" type ......... REQ_TO_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->rndv_req_to_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->rndv_req_to_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->rndv_req_to_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
		MPIU_DBG_PRINTF((" type ......... CLR_TO_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
		MPIU_DBG_PRINTF((" recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
		break;
	    case MPIDI_CH3_PKT_RNDV_SEND:
		MPIU_DBG_PRINTF((" type ......... RNDV_SEND\n"));
		MPIU_DBG_PRINTF((" recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
		break;
	    case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
		MPIU_DBG_PRINTF((" type ......... CANCEL_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->cancel_send_req.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->cancel_send_req.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->cancel_send_req.match.rank));
		break;
	    case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
		MPIU_DBG_PRINTF((" type ......... CANCEL_SEND_RESP\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
		MPIU_DBG_PRINTF((" ack .......... %d\n", pkt->cancel_send_resp.ack));
		break;
	    case MPIDI_CH3_PKT_PUT:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_PUT\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->put.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->put.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->put.datatype));
		MPIU_DBG_PRINTF((" dataloop_size. 0x%08X\n", pkt->put.dataloop_size));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->put.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->put.source_win_handle));
		/*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->put.win_ptr));*/
		break;
	    case MPIDI_CH3_PKT_GET:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->get.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->get.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->get.datatype));
		MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->get.dataloop_size));
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request_handle));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->get.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->get.source_win_handle));
		/*
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request));
		MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->get.win_ptr));
		*/
		break;
	    case MPIDI_CH3_PKT_GET_RESP:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET_RESP\n"));
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request_handle));
		/*MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request));*/
		break;
	    case MPIDI_CH3_PKT_ACCUMULATE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACCUMULATE\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->accum.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->accum.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->accum.datatype));
		MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->accum.dataloop_size));
		MPIU_DBG_PRINTF((" op ........... 0x%08X\n", pkt->accum.op));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->accum.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->accum.source_win_handle));
		/*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->accum.win_ptr));*/
		break;
	    case MPIDI_CH3_PKT_LOCK:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK\n"));
		MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock.lock_type));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock.source_win_handle));
		break;
	    case MPIDI_CH3_PKT_LOCK_GRANTED:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GRANTED\n"));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_granted.source_win_handle));
		break;
		/*
	    case MPIDI_CH3_PKT_SHARED_LOCK_OPS_DONE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_SHARED_LOCK_OPS_DONE\n"));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->shared_lock_ops_done.source_win_handle));
		break;
		*/
	    case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
		MPIU_DBG_PRINTF((" FLOW_CNTRL_UPDATE\n"));
		break;
  	    case MPIDI_CH3_PKT_CLOSE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_CLOSE\n"));
		MPIU_DBG_PRINTF((" ack ......... %s\n", pkt->close.ack ? "TRUE" : "FALSE"));
		break;
	    
	    default:
		MPIU_DBG_PRINTF((" INVALID PACKET\n"));
		MPIU_DBG_PRINTF((" unknown type ... %d\n", pkt->type));
		MPIU_DBG_PRINTF(("  type .......... EAGER_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->eager_send.match.context_id));
		MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->eager_send.data_sz));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->eager_send.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->eager_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
		MPIU_DBG_PRINTF(("  type .......... REQ_TO_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->rndv_req_to_send.match.context_id));
		MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->rndv_req_to_send.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->rndv_req_to_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
		MPIU_DBG_PRINTF(("  type .......... CLR_TO_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
		MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
		MPIU_DBG_PRINTF(("  type .......... RNDV_SEND\n"));
		MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
		MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND\n"));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->cancel_send_req.match.context_id));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->cancel_send_req.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->cancel_send_req.match.rank));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
		MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND_RESP\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
		MPIU_DBG_PRINTF(("   ack .......... %d\n", pkt->cancel_send_resp.ack));
		break;
	}
    }
    MPID_Common_thread_unlock();
}
#endif


const char * MPIDI_VC_Get_state_description(int state)
{
    switch (state)
    {
	case MPIDI_VC_STATE_INACTIVE:
	    return "MPIDI_VC_STATE_INACTIVE";
	case MPIDI_VC_STATE_ACTIVE:
	    return "MPIDI_VC_STATE_ACTIVE";
	case MPIDI_VC_STATE_LOCAL_CLOSE:
	    return "MPIDI_VC_STATE_LOCAL_CLOSE";
	case MPIDI_VC_STATE_REMOTE_CLOSE:
	    return "MPIDI_VC_STATE_REMOTE_CLOSE";
	case MPIDI_VC_STATE_CLOSE_ACKED:
	    return "MPIDI_VC_STATE_CLOSE_ACKED";
	default:
	    return "unknown";
    }
}
