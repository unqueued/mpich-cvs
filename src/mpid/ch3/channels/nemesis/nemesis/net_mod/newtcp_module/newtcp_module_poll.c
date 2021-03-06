/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <errno.h>

#define RECV_MAX_PKT_LEN 1024

static char *recv_buf;

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll_init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);

    MPIU_CHKPMEM_MALLOC(recv_buf, char*, RECV_MAX_PKT_LEN, mpi_errno, "NewTCP temporary buffer");
    MPIU_CHKPMEM_COMMIT();

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


int MPID_nem_newtcp_module_poll_finalize()
{
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_recv_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_recv_handler (struct pollfd *pfd, sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;

    if (((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active == NULL)
    {
        /* receive a new message */
        CHECK_EINTR(bytes_recvd, recv(sc->fd, recv_buf, RECV_MAX_PKT_LEN, 0));
        if (bytes_recvd <= 0)
        {
            if (bytes_recvd == -1 && errno == EAGAIN) /* handle this fast */
                goto fn_exit;

            if (bytes_recvd == 0)
            {
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            }
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror(errno));
        }
    
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "New recv %d", bytes_recvd);

        mpi_errno = MPID_nem_handle_pkt(sc->vc, recv_buf, bytes_recvd);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        /* there is a pending receive, receive it directly into the user buffer */
        MPID_Request *rreq = ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active;
        MPID_IOV *iov = &rreq->dev.iov[rreq->dev.iov_offset];
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

        CHECK_EINTR(bytes_recvd, readv(sc->fd, iov, rreq->dev.iov_count));
        if (bytes_recvd <= 0)
        {
            if (bytes_recvd == -1 && errno == EAGAIN) /* handle this fast */
                goto fn_exit;

            if (bytes_recvd == 0)
            {
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            }
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror(errno));
        }

        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Cont recv %d", bytes_recvd);

        /* update the iov */
        for (iov = &rreq->dev.iov[rreq->dev.iov_offset]; iov < &rreq->dev.iov[rreq->dev.iov_offset + rreq->dev.iov_count]; ++iov)
        {
            if (bytes_recvd < iov->MPID_IOV_LEN)
            {
                iov->MPID_IOV_BUF = (char *)iov->MPID_IOV_BUF + bytes_recvd;
                iov->MPID_IOV_LEN -= bytes_recvd;
                rreq->dev.iov_count = &rreq->dev.iov[rreq->dev.iov_offset + rreq->dev.iov_count] - iov;
                rreq->dev.iov_offset = iov - rreq->dev.iov;
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "bytes_recvd = %d", bytes_recvd);
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "iov len = %d", iov->MPID_IOV_LEN);
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "iov_offset = %d", rreq->dev.iov_offset);
                goto fn_exit;
            }
            bytes_recvd -= iov->MPID_IOV_LEN;
        }
        
        /* the whole iov has been received */

        reqFn = rreq->dev.OnDataAvail;
        if (!reqFn)
        {
            MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);
            MPIDI_CH3U_Request_complete(rreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
            ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active = NULL;
        }
        else
        {
            int complete = 0;
                
            mpi_errno = reqFn(sc->vc, rreq, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            if (complete)
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
                ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active = NULL;
            }
            else
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...not complete");
            }
        }        
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_nem_newtcp_module_connpoll();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
