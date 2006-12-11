/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"

//#define printf(x...) do {} while(0)

#define NUM_PREALLOC_SENDQ 10
#define MAX_SEND_IOV 10

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
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    size_t offset;
    MPID_nem_pkt_t *pkt;
    MPID_nem_newtcp_module_send_q_element_t *e;
    MPIU_CHKPMEM_DECL(2);

    pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */

    pkt->mpich2.datalen = datalen;
    pkt->mpich2.source  = MPID_nem_mem_region.rank;    

    if (!MPID_nem_newtcp_module_vc_is_connected (vc))
    {
        /* MPID_nem_newtcp_module_connection_progress (vc);  try to get connected  */
        mpi_errno = MPID_nem_newtcp_module_connect (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        /*  FIXME define the use of this and above commented function */

        if (!MPID_nem_newtcp_module_vc_is_connected (vc))
        {
            /*printf ("  vc not connected, enqueuing %d\n", MPID_NEM_PACKET_LEN (pkt));*/
            goto enqueue_cell_and_exit;
        }
    }

/*     while(!Q_EMPTY (vc_ch->send_queue))//DARIUS */
/*     { */
/*         mpi_errno = send_queued (vc); /\* try to empty the queue *\/ */
/*         if (mpi_errno) MPIU_ERR_POP (mpi_errno); */
/*     } */
    
    if (!Q_EMPTY (vc_ch->send_queue))
    {
        mpi_errno = send_queued (vc); /* try to empty the queue */
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        if (!Q_EMPTY (vc_ch->send_queue))
        {
            /*printf ("  send queue not empty, enqueuing %d\n", MPID_NEM_PACKET_LEN (pkt));*/
            goto enqueue_cell_and_exit;
        }
    }

    /* start sending the cell */

    CHECK_EINTR (offset, write (vc_ch->sc->fd, pkt, MPID_NEM_PACKET_LEN (pkt)));
/*     MPIU_Assert (offset != 0);//DARIUS */
    MPIU_ERR_CHKANDJUMP (offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
    if (offset == -1)
    {
        if (errno == EAGAIN)
            offset = 0;
        else
            MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror (errno));
    }
    
    /*if (offset != -1)
      printf("  write  %d (%d)\n", offset, MPID_NEM_PACKET_LEN (pkt));*/

    if (offset == MPID_NEM_PACKET_LEN (pkt))
    {
/*         printf ("sent %d (%d)\n", MPID_NEM_PACKET_LEN (pkt), pkt->header.seqno);//DARIUS */
        MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);
        goto fn_exit; /* whole pkt has been sent, we're done */
    }
    
/*     printf ("sent %d of %d (%d)\n", offset, MPID_NEM_PACKET_LEN (pkt), pkt->header.seqno);//DARIUS */
    /* part of the pkt wasn't sent, enqueue it */
    ALLOC_Q_ELEMENT (&e); /* may call MPIU_CHKPMEM_MALLOC */
    e->cell = cell;
    e->len = MPID_NEM_PACKET_LEN (pkt) - offset;
    e->start = (char *)pkt + offset;
    /*printf ("  enqueuing %d\n", e->len);*/
    
    Q_ENQUEUE_EMPTY (&vc_ch->send_queue, e);
    VC_L_ADD (&send_list, vc);
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();    
    return mpi_errno;
 enqueue_cell_and_exit:
    /* enqueue cell on send queue and exit */
    ALLOC_Q_ELEMENT (&e); /* may call MPIU_CHKPMEM_MALLOC */
    e->cell = cell;
    e->len = MPID_NEM_PACKET_LEN (pkt);
    e->start = (char *)pkt;
    Q_ENQUEUE (&vc_ch->send_queue, e);
    goto fn_exit;
 fn_fail:
    MPIU_CHKPMEM_REAP();
/*     printf ("num_elements = %d\n", num_elements);//DARIUS     */
/*     printf ("num_alloced = %d\n", num_alloced);//DARIUS     */
/*     printf ("num_freed = %d\n", num_freed);//DARIUS     */
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME send_queued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_queued (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_IOV iov[MAX_SEND_IOV];
    int count;
    MPID_nem_newtcp_module_send_q_element_t *e, *e_first, *e_last;
    ssize_t bytes_sent;
    ssize_t bytes_queued;    

    MPIU_Assert (!Q_EMPTY (vc_ch->send_queue));

    /* construct iov of pending sends */
    bytes_queued = 0;
    count = 0;
    e_last = NULL;
    e = Q_HEAD (vc_ch->send_queue);
    do
    {
        iov[count].MPID_IOV_BUF = e->start;
        iov[count].MPID_IOV_LEN = e->len;

        bytes_queued += e->len;
        ++count;
        e_last = e;
        e = e->next;
    }
    while (count < MAX_SEND_IOV && e);

    /* write iov */
    CHECK_EINTR (bytes_sent, writev (vc_ch->sc->fd, iov, count));
/*     MPIU_Assert (bytes_sent != 0);//DARIUS */
    MPIU_ERR_CHKANDJUMP1 (bytes_sent == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
    MPIU_ERR_CHKANDJUMP (bytes_sent == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");

    /*if (bytes_sent != -1)
    {
        int i;
        
        printf("  writev %d (", bytes_sent);
        for (i = 0; i < count; ++i)
            printf ("%d ", iov[i].MPID_IOV_LEN);
        printf (")\n");
        }*/
    

    /* remove pending sends that were sent */

    if (bytes_sent <= 0) /* nothing was sent */
        goto fn_exit;

/*     printf ("packets sent\n");//DARIUS */
    e_first = Q_HEAD (vc_ch->send_queue);    
    e_last = NULL;
    e = Q_HEAD (vc_ch->send_queue);
    while (1)
    {        
        MPIU_Assert (bytes_sent);
        MPIU_Assert (e);
        
        if (e->len < bytes_sent)
        {
/*             printf ("  %d (%d)\n", MPID_NEM_PACKET_LEN (&e->cell->pkt), e->cell->pkt.header.seqno);//DARIUS */
            MPID_nem_queue_enqueue (MPID_nem_process_free_queue, e->cell);
            bytes_sent -= e->len;
            e_last = e;
            e = e->next;
        }
        else if (e->len > bytes_sent)
        {
/*             printf ("  %d leftover\n", bytes_sent);//DARIUS */
            e->len -= bytes_sent;
            e->start += bytes_sent;
            break;
        }
        else /* (e->len == bytes_sent) */
        {
/*             printf ("  %d (%d)\n", MPID_NEM_PACKET_LEN (&e->cell->pkt), e->cell->pkt.header.seqno);//DARIUS */
            MPID_nem_queue_enqueue (MPID_nem_process_free_queue, e->cell);
            e_last = e;
            break;
        }
    }

    if (e_last != NULL) /* did we send at least one queued send? */
    {
        Q_REMOVE_ELEMENTS (&vc_ch->send_queue, e_first, e_last);
        FREE_Q_ELEMENTS (e_first, e_last);
        if (Q_EMPTY (vc_ch->send_queue))
            VC_L_REMOVE (&send_list, vc);
    }

    
 fn_exit:
    return mpi_errno;
 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = send_list.head; vc; vc = vc->ch.newtcp_sendl_next)
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

/*     printf ("*** connected *** %d\n", vc->ch.sc->fd); //DARIUS     */

    if (!Q_EMPTY (vc->ch.send_queue))
    {
        VC_L_ADD (&send_list, vc);
        mpi_errno = send_queued (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_fail:    
    return mpi_errno;
}

