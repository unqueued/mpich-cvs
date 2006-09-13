/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <errno.h>


typedef struct recv_overflow_buf
{
    char *start;
    int len;
    MPIDI_VC_t *vc;
    char buf[MPID_NEM_MAX_PACKET_LEN];
} recv_overflow_buf_t;

static recv_overflow_buf_t recv_overflow_buf;

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll_init()
{
    int mpi_errno = MPI_SUCCESS;

    recv_overflow_buf.start = NULL;
    recv_overflow_buf.len = 0;
        
    return mpi_errno;
}

int MPID_nem_newtcp_module_poll_finalize()
{
    return MPI_SUCCESS;
}

static int receive_exactly_one_packet (MPIDI_VC_t *vc);

/* breakout_pkts -- This is called after receiving data into a cell.
   If there were multiple packets received into this cell, this
   function copies any additional packets into their own cells.  If
   there is only a fraction of a packet left, the cell is not enqueued
   onto the process receive queue, but left as a pending receive in
   the VC structure.  If we run out of free cells before all of the
   packets have been copied out, we copy the extra data into the
   recv_overflow_buf.

   NOTE: For performance purposes, the fast-path case (received
   exactly one packet) should be handled before calling this function.
   For this reason, this function won't handle the exactly-one-packet
   case.
*/
#undef FUNCNAME
#define FUNCNAME breakout_pkts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int breakout_pkts (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t v_cell, int len)
{
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_t *cell = (MPID_nem_cell_t *) v_cell;; /* non-volatile cell */
    struct {MPID_nem_abs_cell_t *head, *tail;} cell_queue = {0};
    MPID_nem_pkt_t *next_pkt;

    if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)))
    {
        MPIU_Assert (vc_ch->pending_recv.cell == NULL);
        vc_ch->pending_recv.cell = cell;
        vc_ch->pending_recv.end = (char *)MPID_NEM_CELL_TO_PACKET (cell) + len;
        vc_ch->pending_recv.len = len;
        /*        printf (" recvd pkt fragment got %d of %d\n", len, MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)));*/
        goto fn_exit;
    }

    /* there is more than one packet in this cell */
    vc_ch->pending_recv.cell = NULL;

    /* we can't enqueue the cell onto the process recv queue yet
       because we haven't copied all of the additional packets out yet
       (in case we're multithreaded).  So we need to put them in a
       separate queue now, and queue them onto the process recv queue
       once were all done. */
    Q_ENQUEUE_EMPTY (&cell_queue, (MPID_nem_abs_cell_t *)cell);
    len -= MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell));
    next_pkt = (MPID_nem_pkt_t *)((char *)MPID_NEM_CELL_TO_PACKET (cell) + MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)));
    
    MPIU_Assert (len > 0); /* This function is only called when there is less than one packet or more than one packet in the cell */
    
    while (!MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
    {
        MPID_nem_queue_dequeue (MPID_nem_newtcp_module_free_queue, &v_cell);
        cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
        
        if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (next_pkt))
        {
            MPIU_Assert (vc_ch->pending_recv.cell == NULL);
            MPID_NEM_MEMCPY (MPID_NEM_CELL_TO_PACKET (cell), next_pkt, len);
            vc_ch->pending_recv.cell = cell;
            vc_ch->pending_recv.end = (char *)MPID_NEM_CELL_TO_PACKET (cell) + len;
            vc_ch->pending_recv.len = len;
            goto enqueue_and_exit;
        }
        
        MPID_NEM_MEMCPY (MPID_NEM_CELL_TO_PACKET (cell), next_pkt, MPID_NEM_PACKET_LEN (next_pkt));
        Q_ENQUEUE (&cell_queue, (MPID_nem_abs_cell_t *)cell);
        len -= MPID_NEM_PACKET_LEN (next_pkt);
        next_pkt = (MPID_nem_pkt_t *)((char *)next_pkt + MPID_NEM_PACKET_LEN (next_pkt));

        if (len == 0)
        {
            /* all packets in the buffer have been completely recvd so there are no pending recvs */
            vc_ch->pending_recv.cell = NULL;
            goto enqueue_and_exit;
        }
    }

    /* we ran out of free cells, copy into overflow buffer */
    MPIU_Assert (vc->ch.pending_recv.cell == NULL);
    MPIU_Assert (recv_overflow_buf.start == NULL);
    MPID_NEM_MEMCPY (recv_overflow_buf.buf, next_pkt, len);
    recv_overflow_buf.start = recv_overflow_buf.buf;
    recv_overflow_buf.len = len;
    recv_overflow_buf.vc = vc;
    
 enqueue_and_exit:
    /* enqueue the received cells onto the process receive queue */
    while (!Q_EMPTY (cell_queue))
    {
        MPID_nem_abs_cell_t *acell;
        
        Q_DEQUEUE (&cell_queue, &acell);
        MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, (MPID_nem_cell_t *)acell);
    }
    
 fn_exit:
    return mpi_errno;
}

/* receive_exactly_one_packet -- tries to receive the remaining part of this packet  */
#undef FUNCNAME
#define FUNCNAME receive_exactly_one_packet
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int receive_exactly_one_packet (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    ssize_t bytes_recvd;

    /* make sure we have at least the header */
    if (vc_ch->pending_recv.len < MPID_NEM_MIN_PACKET_LEN)
    {
        CHECK_EINTR (bytes_recvd, read (vc_ch->sc->fd, vc_ch->pending_recv.end, MPID_NEM_MIN_PACKET_LEN - vc_ch->pending_recv.len));
        if (bytes_recvd == -1)
        {
            if (errno == EAGAIN)
                goto fn_exit;
            else
                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
        }
        /*        if (bytes_recvd != -1)
                  printf("  read  %d\n", bytes_recvd);*/

        vc_ch->pending_recv.len += bytes_recvd;

        if (vc_ch->pending_recv.len < MPID_NEM_MIN_PACKET_LEN)
        {
            /* still haven't received the whole header */
            /*printf (" incomplete recv\n");*/
            goto fn_exit;
        }
    }

    /* try to receive the rest of the packet */
    if (vc_ch->pending_recv.len < MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)))
    {
        CHECK_EINTR (bytes_recvd, read (vc_ch->sc->fd, vc_ch->pending_recv.end,
                                        MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)) - vc_ch->pending_recv.len));
        /*        if (bytes_recvd != -1)
                  printf("  read  %d\n", bytes_recvd);*/
        if (bytes_recvd == -1)
        {
            if (errno == EAGAIN)
                goto fn_exit;
            else
                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
        }

        if (vc_ch->pending_recv.len < MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)))
        {
            /* still haven't received the whole packet */
            /*printf (" incomplete recv\n");*/
            goto fn_exit;
        }
    }

    /* got the whole packet, enqueue on recv queue */
    MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, vc_ch->pending_recv.cell);
    vc_ch->pending_recv.cell = NULL;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_recv_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_recv_handler (struct pollfd *pfd, sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;
    MPIDI_VC_t *vc = sc->vc;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_nem_cell_ptr_t v_cell;
    MPID_nem_cell_t *cell; /* non-volatile cell */

    if (vc_ch->pending_recv.cell)
    {
        if (recv_overflow_buf.start != NULL && MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
        {
            /* This is a corner case that we handle less efficiently.  When we run out of cells, and the overflow_buf is in use,
               if we receive more than one packet into this cell we would have no place to put the additional packets.  In this
               case, we take the slow route and make sure to receive only the rest of this packet. */
            mpi_errno = receive_exactly_one_packet (vc);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            goto fn_exit;
        }
        
        /* there is a partially received pkt in tmp_cell, continue receiving into it */
        CHECK_EINTR (bytes_recvd, read (sc->fd, vc_ch->pending_recv.end, MPID_NEM_MAX_PACKET_LEN - vc_ch->pending_recv.len));
        /*if (bytes_recvd != -1)
          printf("  read  %d into tmp_cell\n", bytes_recvd);*/
        if (bytes_recvd == -1)
        {
            if (errno == EAGAIN)
                goto fn_exit;
            else
                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
        }
        vc_ch->pending_recv.len += bytes_recvd;
        
        if (vc_ch->pending_recv.len >= MPID_NEM_MIN_PACKET_LEN)
        {
            /* fast path: single packet case */
            if (vc_ch->pending_recv.len == MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)))
            {
                MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, vc_ch->pending_recv.cell);
                vc_ch->pending_recv.cell = NULL;
                /*printf (" recvd whole pkt\n");*/
                goto fn_exit;
            }
            else if (vc_ch->pending_recv.len > MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)))
            {
                /* received more than one packet */
                mpi_errno = breakout_pkts (vc, vc_ch->pending_recv.cell, vc_ch->pending_recv.len);
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            }
        }
    }
    else if (!MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
    {
        /* receive next packets into new cell */

        MPID_nem_queue_dequeue (MPID_nem_newtcp_module_free_queue, &v_cell);
        cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            
        CHECK_EINTR (bytes_recvd, read (sc->fd, MPID_NEM_CELL_TO_PACKET (cell), MPID_NEM_MAX_PACKET_LEN));
        /*        if (bytes_recvd != -1)
                  printf("  read  %d\n", bytes_recvd);*/
        if (bytes_recvd == -1)
        {
            if (errno == EAGAIN)
                goto fn_exit;
            else
                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
        }
        
        /* fast path: single packet case */
        if (bytes_recvd >= MPID_NEM_MIN_PACKET_LEN &&
            bytes_recvd == MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)))
        {
            MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);
            /*            printf (" recvd whole pkt\n");*/
            goto fn_exit;
        }
    
        mpi_errno = breakout_pkts (vc, cell, bytes_recvd);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        goto fn_exit;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME recv_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int recv_progress (void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t v_cell;
    MPID_nem_cell_t *cell; /* non-volatile cell */

    MPIDI_NEMTCP_FUNC_ENTER;
    /* Copy any packets from overflow buf into cells first */
    if (recv_overflow_buf.start)
    {
        while (!MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
        {
            MPID_nem_pkt_t *pkt;
            int len;

            pkt = (MPID_nem_pkt_t *)recv_overflow_buf.start;
            len = (recv_overflow_buf.len < MPID_NEM_MIN_PACKET_LEN ||
                   recv_overflow_buf.len < MPID_NEM_PACKET_LEN (pkt))
                ? recv_overflow_buf.len : MPID_NEM_PACKET_LEN (pkt);

            /* allocate a new cell and copy the packet (or fragment) into it */
            MPID_nem_queue_dequeue (MPID_nem_newtcp_module_free_queue, &v_cell);
            cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            MPID_NEM_MEMCPY (&cell->pkt, pkt, len);
           
            if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (pkt))
            {
                /* this was just a packet fragment, attach the cell to the vc to be filled in later */
                MPIU_Assert (recv_overflow_buf.vc->ch.pending_recv.cell == NULL);
                recv_overflow_buf.vc->ch.pending_recv.cell = cell;
                recv_overflow_buf.vc->ch.pending_recv.end = (char *)(&cell->pkt) + len;
                recv_overflow_buf.vc->ch.pending_recv.len = len;

                /* there are no more packets in the overflow buffer */
                recv_overflow_buf.start = NULL;
                break;
            }

            /* update overflow buffer pointers */
            recv_overflow_buf.start += len;
            recv_overflow_buf.len -= len;
            
            if (recv_overflow_buf.len == 0)
            {
                /* there are no more packets in the overflow buffer */
                recv_overflow_buf.start = NULL;
                break;
            }
        }
    }
    
    mpi_errno = MPID_nem_newtcp_module_connpoll();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);


 fn_exit:
    MPIDI_NEMTCP_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure"));
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_nem_newtcp_module_send_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = recv_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

