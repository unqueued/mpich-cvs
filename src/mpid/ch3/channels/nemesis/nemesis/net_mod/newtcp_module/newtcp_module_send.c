/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"

#define NUM_PREALLOC_SENDQ 10
#define MAX_SEND_IOV 10

#define SENDQ_EMPTY(q) GENERIC_Q_EMPTY (q)
#define SENDQ_HEAD(q) GENERIC_Q_HEAD (q)
#define SENDQ_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, dev.next)
#define SENDQ_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, dev.next)


typedef struct MPID_nem_newtcp_module_send_q_element
{
    struct MPID_nem_newtcp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to send */
    char *start;                       /* pointer to next byte to send */
    MPID_nem_cell_ptr_t cell;
    /*     char buf[MPID_NEM_MAX_PACKET_LEN];*/ /* data to be sent */
} MPID_nem_newtcp_module_send_q_element_t;

struct {MPIDI_VC_t *head;} send_list = {0};
struct {MPID_nem_newtcp_module_send_q_element_t *top;} free_buffers = {0};

#define ALLOC_Q_ELEMENT(e) do {                                                                                                         \
        if (S_EMPTY (free_buffers))                                                                                                     \
        {                                                                                                                               \
            MPIU_CHKPMEM_MALLOC (*(e), MPID_nem_newtcp_module_send_q_element_t *, sizeof(MPID_nem_newtcp_module_send_q_element_t),      \
                                 mpi_errno, "send queue element");                                                                      \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            S_POP (&free_buffers, e);                                                                                                   \
        }                                                                                                                               \
    } while (0)

/* FREE_Q_ELEMENTS() frees a list if elements starting at e0 through e1 */
#define FREE_Q_ELEMENTS(e0, e1) S_PUSH_MULTIPLE (&free_buffers, e0, e1)
#define FREE_Q_ELEMENT(e) S_PUSH (&free_buffers, e)

static int send_queued (MPIDI_VC_t *vc);

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL (NUM_PREALLOC_SENDQ);
    
    /* preallocate sendq elements */
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        MPID_nem_newtcp_module_send_q_element_t *e;
        
        MPIU_CHKPMEM_MALLOC (e, MPID_nem_newtcp_module_send_q_element_t *,
                             sizeof(MPID_nem_newtcp_module_send_q_element_t), mpi_errno, "send queue element");
        S_PUSH (&free_buffers, e);
    }

    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_Assert(0);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME send_queued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_queued (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *sreq;
    MPIDI_msg_sz_t offset;
    MPID_IOV *iov;
    int complete;

    while (!SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
    {
        sreq = SENDQ_HEAD(VC_FIELD(vc, send_queue));
        MPIU_Assert(sreq->dev.iov_count <= 2);
        
        iov = &sreq->dev.iov[sreq->ch.iov_offset];

/*         printf("sreq = %p sreq->dev.iov = %p iov = %p\n", sreq, sreq->dev.iov, iov); */
/*         printf("iov[0].MPID_IOV_BUF = %p iov[0].MPID_IOV_LEN = %d iov_count = %d\n", iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN, sreq->dev.iov_count);//DARIUS */
/*         printf("&iov[0].MPID_IOV_LEN = %p sreq->ch.iov_offset = %d\n", &iov[0].MPID_IOV_LEN, sreq->ch.iov_offset);//DARIUS */
        CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, sreq->dev.iov_count));
        MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        if (offset == -1)
        {
            if (errno == EAGAIN)
                offset = 0;
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

        complete = 1;
        for (iov = &sreq->dev.iov[sreq->ch.iov_offset]; iov < &sreq->dev.iov[sreq->ch.iov_offset + sreq->dev.iov_count]; ++iov)
        {
            if (offset < iov->MPID_IOV_LEN)
            {
                iov->MPID_IOV_BUF = (char *)iov->MPID_IOV_BUF + offset;
                iov->MPID_IOV_LEN -= offset;
                sreq->ch.iov_offset = iov - sreq->dev.iov;
                complete = 0;
                break;
            }
            offset -= iov->MPID_IOV_LEN;
        }
        if (complete)
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

            SENDQ_DEQUEUE(&VC_FIELD(vc, send_queue), &sreq);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                break;
            }
            else
            {
                int complete = 0;
                
                mpi_errno = reqFn(vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (complete)
                {
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    break;
                }

                MPIU_Assert(0); /* FIXME:  I don't think we should get here with contig messages */
                
                sreq->ch.vc = vc;
                break;
            }
        }
    }

    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        VC_L_REMOVE(&send_list, vc);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = send_list.head; vc; vc = vc->ch.next)
    {
        mpi_errno = send_queued (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_finalize()
{
    int mpi_errno = MPI_SUCCESS;

/*     printf ("MPID_nem_newtcp_module_send_finalize\n");//DARIUS */
    while (!VC_L_EMPTY (send_list))
        MPID_nem_newtcp_module_send_progress();

    while (!S_EMPTY (free_buffers))
    {
        MPID_nem_newtcp_module_send_q_element_t *e;
        S_POP (&free_buffers, &e);
        MPIU_Free (e);
    }
/*     printf ("  done\n");//DARIUS */

    return mpi_errno;
}

/* MPID_nem_newtcp_module_conn_est -- this function is called when the
   connection is finally extablished to send any pending sends */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_conn_est
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_conn_est (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

/*     printf ("*** connected *** %d\n", VC_FIELD(vc, sc)->fd); //DARIUS     */

    if (!SENDQ_EMPTY (VC_FIELD(vc, send_queue)))
    {
        VC_L_ADD (&send_list, vc);
        mpi_errno = send_queued (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_fail:    
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_msg_sz_t offset = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPID_IOV iov[2];

            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            iov[1].MPID_IOV_BUF = data;
            iov[1].MPID_IOV_LEN = data_sz;
        
            CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, 2));
            MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            if (offset == -1)
            {
                if (errno == EAGAIN)
                    offset = 0;
                else
                    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
            }
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

            if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
            {
                /* sent whole message */
                *sreq_ptr = NULL;
                goto fn_exit;
            }
        }
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* create and enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = 0;
    sreq->ch.vc = vc;
    sreq->ch.iov_offset = 0;

/*     printf("&sreq->dev.pending_pkt = %p sizeof(MPIDI_CH3_PktGeneric_t) = %d\n", &sreq->dev.pending_pkt, sizeof(MPIDI_CH3_PktGeneric_t));//DARIUS */
/*     printf("offset = %d\n", offset);//DARIUS */

    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset ;
        if (data_sz)
        {
            sreq->dev.iov[1].MPID_IOV_BUF = data;
            sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }

/*     printf("sreq = %p sreq->dev.iov = %p\n", sreq, sreq->dev.iov); */
/*     printf("sreq->dev.iov[0].MPID_IOV_BUF = %p\n", sreq->dev.iov[0].MPID_IOV_BUF);//DARIUS */
/*     printf("sreq->dev.iov[0].MPID_IOV_LEN = %d\n", sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */
/*     printf("&sreq->dev.iov[0].MPID_IOV_LEN = %p\n", &sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */

    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)) && MPID_nem_newtcp_module_vc_is_connected(vc))
        VC_L_ADD(&send_list, vc);
    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);

    *sreq_ptr = sreq;
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t offset = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iSendContig");

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPID_IOV iov[2];

            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            iov[1].MPID_IOV_BUF = data;
            iov[1].MPID_IOV_LEN = data_sz;
        
            CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, 2));
            MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            if (offset == -1)
            {
                if (errno == EAGAIN)
                    offset = 0;
                else
                    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
            }
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

            if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
            {
                /* sent whole message */
                int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

                reqFn = sreq->dev.OnDataAvail;
                if (!reqFn)
                {
                    MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                    MPIDI_CH3U_Request_complete(sreq);
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    goto fn_exit;
                }
                else
                {
                    int complete = 0;
                
                    mpi_errno = reqFn(vc, sreq, &complete);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                    if (complete)
                    {
                        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                        goto fn_exit;
                    }

                    MPIU_Assert(0); /* FIXME:  I don't think we should get here with contig messages */
                
                    sreq->ch.vc = vc;
                    goto fn_exit;
                }
            }
        }
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* create and enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

/*     printf("&sreq->dev.pending_pkt = %p sizeof(MPIDI_CH3_PktGeneric_t) = %d\n", &sreq->dev.pending_pkt, sizeof(MPIDI_CH3_PktGeneric_t));//DARIUS */

    sreq->ch.vc = vc;
    sreq->ch.iov_offset = 0;

    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset ;
        if (data_sz)
        {
            sreq->dev.iov[1].MPID_IOV_BUF = data;
            sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }

/*     printf("sreq = %p sreq->dev.iov = %p\n", sreq, sreq->dev.iov); */
/*     printf("sreq->dev.iov[0].MPID_IOV_BUF = %p\n", sreq->dev.iov[0].MPID_IOV_BUF);//DARIUS */
/*     printf("sreq->dev.iov[0].MPID_IOV_LEN = %d\n", sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */
/*     printf("&sreq->dev.iov[0].MPID_IOV_LEN = %p\n", &sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */

    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)) && MPID_nem_newtcp_module_vc_is_connected(vc))
        VC_L_ADD(&send_list, vc);
    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
