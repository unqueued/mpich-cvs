/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

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

/* request completion actions */
static int do_cts(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete);
static int do_send(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete);
static int do_cookie(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete);

/* packet handlers */
static int pkt_RTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);
static int pkt_CTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);
static int pkt_DONE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);
static int pkt_COOKIE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_pkthandler_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);

    /* Check that the array is large enough */
    if (arraySize <= MPIDI_NEM_PKT_END) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN, "**ch3|pktarraytoosmall");
    }

    pktArray[MPIDI_NEM_PKT_LMT_RTS] = pkt_RTS_handler;
    pktArray[MPIDI_NEM_PKT_LMT_CTS] = pkt_CTS_handler;
    pktArray[MPIDI_NEM_PKT_LMT_DONE] = pkt_DONE_handler;
    pktArray[MPIDI_NEM_PKT_LMT_COOKIE] = pkt_COOKIE_handler;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPID_nem_lmt_RndvSend - Send a request to perform a rendezvous send */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_RndvSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_RndvSend(MPID_Request **sreq_p, const void * buf, int count, MPI_Datatype datatype, int dt_contig,
                          MPIDI_msg_sz_t data_sz, MPI_Aint dt_true_lb, int rank, int tag, MPID_Comm * comm, int context_offset)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)&upkt;
    MPIDI_VC_t *vc;
    MPID_Request *sreq =*sreq_p;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_RNDVSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_RNDVSEND);
  
    MPIDI_Comm_get_vc(comm, rank, &vc);

    /* if the lmt functions are not set, fall back to the default rendezvous code */
    if (((MPIDI_CH3I_VC *)vc->channel_private)->lmt_initiate_lmt == NULL)
    {
        mpi_errno = MPIDI_CH3_RndvSend(sreq_p, buf, count, datatype, dt_contig, data_sz, dt_true_lb, rank, tag, comm, context_offset);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIU_DBG_MSG_D(CH3_OTHER,VERBOSE,
		   "sending lmt RTS, data_sz=" MPIDI_MSG_SZ_FMT, data_sz);
    sreq->partner_request = NULL;
    sreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;
	
    MPIDI_Pkt_init(rts_pkt, MPIDI_NEM_PKT_LMT_RTS);
    rts_pkt->match.rank	      = comm->rank;
    rts_pkt->match.tag	      = tag;
    rts_pkt->match.context_id = comm->context_id + context_offset;
    rts_pkt->sender_req_id    = sreq->handle;
    rts_pkt->data_sz	      = data_sz;

    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(rts_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);

    mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_initiate_lmt(vc, &upkt, sreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_RNDVSEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
 * This routine processes a rendezvous message once the message is matched.
 * It is used in mpid_recv and mpid_irecv.
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_RndvRecv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_RndvRecv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int complete = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_RNDVRECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_RNDVRECV);

    /* if the lmt functions are not set, fall back to the default rendezvous code */
    if (((MPIDI_CH3I_VC *)vc->channel_private)->lmt_initiate_lmt == NULL)
    {
        mpi_errno = MPIDI_CH3_RecvRndv(vc, rreq);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE, "lmt RTS in the request");

    mpi_errno = do_cts(vc, rreq, &complete);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(complete);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_RNDVRECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_RTS_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_RTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_PKT_RTS_HANDLER);
    MPID_Request * rreq;
    int found;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)pkt;
    char *data_buf;
    MPIDI_msg_sz_t data_len;
    MPIU_CHKPMEM_DECL(1);
        
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_RTS_HANDLER);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST, "received LMT RTS pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d, data_sz=" MPIDI_MSG_SZ_FMT,
                                        rts_pkt->sender_req_id, rts_pkt->match.rank, rts_pkt->match.tag, rts_pkt->match.context_id,
                                        rts_pkt->data_sz));

    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&rts_pkt->match, &found);
    MPIU_ERR_CHKANDJUMP(rreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);

    rreq->ch.lmt_req_id = rts_pkt->sender_req_id;
    rreq->ch.lmt_data_sz = rts_pkt->data_sz;

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    if (rts_pkt->cookie_len == 0)
    {
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;
        rreq->dev.iov_count = 0;
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;

        if (found)
        {
            /* there's no cookie to receive, and we found a match, so handle the cts directly */
            int complete;
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"posted request found");
            mpi_errno = do_cts(vc, rreq, &complete);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            MPIU_Assert (complete);
        }
        else
        {
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");
            rreq->dev.OnDataAvail = 0;
            MPIDI_CH3_Progress_signal_completion();
        }
    }
    else
    {
        /* set for the cookie to be received into the tmp_cookie in the request */
        MPIU_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, char *, rts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = rts_pkt->cookie_len;

        /* if all data has been received, copy it here, otherwise let channel do the copy */
        if (data_len >= rts_pkt->cookie_len)
        {
            MPID_NEM_MEMCPY(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, data_buf, rts_pkt->cookie_len);
            *rreqp = NULL;
            *buflen = sizeof(MPIDI_CH3_Pkt_t) + rts_pkt->cookie_len;
        }
        else
        {
            rreq->dev.iov[0] = rreq->ch.lmt_tmp_cookie;
            rreq->dev.iov_count = 1;
            *rreqp = rreq;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            
            if (found)
            {
                rreq->dev.OnDataAvail = do_cts;
            }
            else
            {
                MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");
                rreq->dev.OnDataAvail = 0;
                MPIDI_CH3_Progress_signal_completion();
            }
        }
    }    
        
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_RTS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_CTS_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_CTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    MPID_nem_pkt_lmt_cts_t * const cts_pkt = (MPID_nem_pkt_lmt_cts_t *)pkt;
    MPID_Request *sreq;
    MPID_Request *rts_sreq;
    char *data_buf;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_PKT_CTS_HANDLER);
    
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_CTS_HANDLER);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received rndv CTS pkt");

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    MPID_Request_get_ptr(cts_pkt->sender_req_id, sreq);

    sreq->ch.lmt_req_id = cts_pkt->receiver_req_id;
    sreq->ch.lmt_data_sz = cts_pkt->data_sz;

    /* Release the RTS request if one exists.
       MPID_Request_fetch_and_clear_rts_sreq() needs to be atomic to
       prevent cancel send from cancelling the wrong (future) request.
       If MPID_Request_fetch_and_clear_rts_sreq() returns a NULL
       rts_sreq, then MPID_Cancel_send() is responsible for releasing
       the RTS request object. */
    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
    if (rts_sreq != NULL)
        MPID_Request_release(rts_sreq);

    if (cts_pkt->cookie_len != 0)
    {
        MPIU_CHKPMEM_MALLOC(sreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, char *, cts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
        sreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = cts_pkt->cookie_len;

        /* if all data has been received, copy it here, otherwise let channel do the copy */
        if (data_len >= cts_pkt->cookie_len)
        {
            MPID_NEM_MEMCPY(sreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, data_buf, cts_pkt->cookie_len);
            mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_start_send(vc, sreq, sreq->ch.lmt_tmp_cookie);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            *buflen = sizeof(MPIDI_CH3_Pkt_t) + cts_pkt->cookie_len;
            *rreqp = NULL;
        }
        else
        {
            /* create a recv req and set up to receive the cookie into the sreq's tmp_cookie */
            MPID_Request *rreq;
            MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_fail);
            /* FIXME:  where does this request get freed? */
            
            rreq->dev.iov[0] = sreq->ch.lmt_tmp_cookie;
            rreq->dev.iov_count = 1;
            rreq->ch.lmt_req = sreq;
            rreq->dev.OnDataAvail = do_send;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            *rreqp = rreq;
        }
    }
    else
    {
        MPID_IOV cookie = {0,0};
        mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_start_send(vc, sreq, cookie);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;
    }
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_CTS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_DONE_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_DONE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_done_t * const done_pkt = (MPID_nem_pkt_lmt_done_t *)pkt;
    MPID_Request *req;
    MPIDI_STATE_DECL(MPID_STATE_PKT_DONE_HANDLER);
    
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_DONE_HANDLER);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    MPID_Request_get_ptr(done_pkt->req_id, req);

    switch (MPIDI_Request_get_type(req))
    {
    case MPIDI_REQUEST_TYPE_RECV:
        mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_done_recv(vc, req);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        break;
    case MPIDI_REQUEST_TYPE_SEND:
        mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_done_send(vc, req);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        break;
    default:
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "unexpected request type");
        break;
    }   

    *rreqp = NULL;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_DONE_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_COOKIE_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_COOKIE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_cookie_t * const cookie_pkt = (MPID_nem_pkt_lmt_cookie_t *)pkt;
    MPID_Request *req;
    char *data_buf;
    MPIDI_msg_sz_t data_len;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_PKT_COOKIE_HANDLER);
    
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_COOKIE_HANDLER);

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    MPID_Request_get_ptr(cookie_pkt->req_id, req);

    if (cookie_pkt->cookie_len != 0)
    {
        if (data_len >= cookie_pkt->cookie_len)
        {
            /* call handle cookie with cookie data in receive buffer */
            MPID_IOV cookie;

            cookie.MPID_IOV_BUF = data_buf;
            cookie.MPID_IOV_LEN = cookie_pkt->cookie_len;
            mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_handle_cookie(vc, req, cookie);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            *rreqp = NULL;
            *buflen = sizeof(MPIDI_CH3_Pkt_t) + cookie_pkt->cookie_len;
        }
        else
        {
            /* create a recv req and set up to receive the cookie into the rreq's tmp_cookie */
            MPID_Request *rreq;
            
            MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_fail);
            MPIU_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, char *, cookie_pkt->cookie_len, mpi_errno, "tmp cookie buf");
            /* FIXME:  where does this request get freed? */
            rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = cookie_pkt->cookie_len;
            
            rreq->dev.iov[0] = rreq->ch.lmt_tmp_cookie;
            rreq->dev.iov_count = 1;
            rreq->ch.lmt_req = req;
            rreq->dev.OnDataAvail = do_cookie;
            *rreqp = rreq;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
        }
    }
    else
    {
        MPID_IOV cookie = {0,0};

        mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_handle_cookie(vc, req, cookie);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;
    }

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_COOKIE_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_cts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_cts(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_IOV s_cookie;
    MPIDI_STATE_DECL(MPID_STATE_DO_CTS);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_CTS);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"posted request found");

    /* determine amount of data to be transfered and check for truncation */
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (rreq->ch.lmt_data_sz > data_sz)
    {
        MPIU_ERR_SET2(rreq->status.MPI_ERROR, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", rreq->ch.lmt_data_sz, data_sz);
        rreq->ch.lmt_data_sz = data_sz;
    }
    
    s_cookie = rreq->ch.lmt_tmp_cookie;

    mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_start_recv(vc, rreq, s_cookie);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* free cookie buffer allocated in RTS handler */
    if (rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN)
    {
        MPIU_Free(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF);
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;
    }
    
    *complete = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_CTS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME do_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_send(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV r_cookie;
    MPID_Request * const sreq = rreq->ch.lmt_req;
    MPIDI_STATE_DECL(MPID_STATE_DO_SEND);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_SEND);

    r_cookie = sreq->ch.lmt_tmp_cookie;

    mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_start_send(vc, sreq, r_cookie);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* free cookie buffer allocated in CTS handler */
    MPIU_Free(sreq->ch.lmt_tmp_cookie.MPID_IOV_BUF);
    sreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;

    *complete = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME do_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_cookie(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV cookie;
    MPID_Request *req = rreq->ch.lmt_req;
    MPIDI_STATE_DECL(MPID_STATE_DO_COOKIE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_COOKIE);

    cookie = req->ch.lmt_tmp_cookie;

    mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->lmt_handle_cookie(vc, req, cookie);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* free cookie buffer allocated in COOKIE handler */
    MPIU_Free(req->ch.lmt_tmp_cookie.MPID_IOV_BUF);
    req->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;

    *complete = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_COOKIE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
