/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

#undef FUNCNAME
#define FUNCNAME handle_sock_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int handle_sock_op(MPIDU_Sock_event_t *event_ptr)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_SOCK_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_SOCK_OP);
    switch (event_ptr->op_type)
    {
    case MPIDU_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    MPID_Request * rreq = conn->recv_active;

	    if (event_ptr->error != MPIDU_SOCK_SUCCESS)
	    {
		if (!shutting_down || MPIR_ERR_GET_CLASS(event_ptr->error) != MPIDU_SOCK_ERR_CONN_CLOSED /*event_ptr->error != SOCK_EOF*/)  /* FIXME: this should be handled by the close protocol */
		{
		    connection_recv_fail(conn, event_ptr->error);
		}

		break;
	    }

	    if (conn->recv_active)
	    {
		conn->recv_active = NULL;
		/* decrement the number of active reads */
		MPIDI_CH3I_sock_read_active--;
		MPIDI_CH3U_Handle_recv_req(conn->vc, rreq);
		if (conn->recv_active == NULL)
		{ 
		    connection_post_recv_pkt(conn);
		}
	    }
	    else /* incoming packet header */
	    {

		if (conn->pkt.type < MPIDI_CH3_PKT_END_CH3)
		{
		    conn->recv_active = NULL;
		    MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt);
		    if (conn->recv_active == NULL)
		    { 
			connection_post_recv_pkt(conn);
		    }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		{
		    int pg_id;
		    int pg_rank;
		    MPIDI_VC * vc;

		    pg_id = conn->pkt.sc_open_req.pg_id;
		    pg_rank = conn->pkt.sc_open_req.pg_rank;
		    vc = &MPIDI_CH3I_Process.pg->vc_table[pg_rank]; /* FIXME: need to lookup process group from pg_id */
#ifdef MPICH_DBG_OUTPUT
		    /*assert(vc->ch.pg_rank == pg_rank);*/
		    if (vc->ch.pg_rank != pg_rank)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**rank", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			return mpi_errno;
		    }
#endif

		    if (vc->ch.conn == NULL || MPIR_Process.comm_world->rank < pg_rank)
		    {
			vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			vc->ch.sock = conn->sock;
			vc->ch.conn = conn;
			conn->vc = vc;

			conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			conn->pkt.sc_open_resp.ack = TRUE;
		    }
		    else
		    {
			conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			conn->pkt.sc_open_resp.ack = FALSE;
		    }

		    conn->state = CONN_STATE_OPEN_LSEND;
		    connection_post_send_pkt(conn);

		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP)
		{
		    if (conn->pkt.sc_open_resp.ack)
		    {
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
#ifdef MPICH_DBG_OUTPUT
			/*
			assert(conn->vc->ch.conn == conn);
			assert(conn->vc->ch.sock == conn->sock);
			*/
			if (conn->vc->ch.conn != conn)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**bad_conn", "**bad_conn %p %p", conn->vc->ch.conn, conn);
			    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			    return mpi_errno;
			}
			if (conn->vc->ch.sock != conn->sock)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**bad_sock", "**bad_sock %d %d",
				MPIDU_Sock_get_sock_id(conn->vc->ch.sock), MPIDU_Sock_get_sock_id(conn->sock));
			    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			    return mpi_errno;
			}
#endif

			connection_post_recv_pkt(conn);
			connection_post_sendq_req(conn);
		    }
		    else
		    {
			conn->vc = NULL;
			conn->state = CONN_STATE_CLOSING;
			MPIDU_Sock_post_close(conn->sock);
		    }
		}
		else
		{
		    MPIDI_DBG_Print_packet(&conn->pkt);
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**badpacket", "**badpacket %d",
			conn->pkt.type);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
		    return mpi_errno;
		}
	    }

	    break;
	}

    case MPIDU_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    if (event_ptr->error != MPIDU_SOCK_SUCCESS)
	    {
		connection_send_fail(conn, event_ptr->error);
		break;
	    }

	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		conn->send_active = NULL;
		MPIDI_CH3U_Handle_send_req(conn->vc, sreq);
		if (conn->send_active == NULL)
		{
		    if (MPIDI_CH3I_SendQ_head(conn->vc) == sreq)
		    {
			MPIDI_CH3I_SendQ_dequeue(conn->vc);
		    }
		    connection_post_sendq_req(conn);
		}
	    }
	    else /* finished writing internal packet header */
	    {
		if (conn->state == CONN_STATE_OPEN_CSEND)
		{
		    /* finished sending open request packet */
		    /* post receive for open response packet */
		    conn->state = CONN_STATE_OPEN_CRECV;
		    connection_post_recv_pkt(conn);
		}
		else if (conn->state == CONN_STATE_OPEN_LSEND)
		{
		    /* finished sending open response packet */
		    if (conn->pkt.sc_open_resp.ack == TRUE)
		    { 
			/* post receive for packet header */
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			connection_post_recv_pkt(conn);
			connection_post_sendq_req(conn);
		    }
		    else
		    {
			/* head-to-head connections - close this connection */
			conn->state = CONN_STATE_CLOSING;
			mpi_errno = MPIDU_Sock_post_close(conn->sock);
			/*assert(rc == SOCK_SUCCESS);*/
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**sock_post_close", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			    return mpi_errno;
			}
		    }
		}
	    }

	    break;
	}

    case MPIDU_SOCK_OP_ACCEPT:
	{
	    MPIDI_CH3I_Connection_t * conn;

	    conn = connection_alloc();
	    if (conn == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
		return mpi_errno;
	    }
	    mpi_errno = MPIDU_Sock_accept(listener_conn->sock, sock_set, conn, &conn->sock);
	    if (mpi_errno == MPI_SUCCESS)
	    { 
		conn->vc = NULL;
		conn->state = CONN_STATE_OPEN_LRECV;
		conn->send_active = NULL;
		conn->recv_active = NULL;

		connection_post_recv_pkt(conn);
	    }
	    else
	    {
		connection_free(conn);
	    }

	    break;
	}

    case MPIDU_SOCK_OP_CONNECT:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    if (event_ptr->error != MPIDU_SOCK_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**connfailed", "**connfailed %d %d",
		    /* FIXME: pgid */ -1, conn->vc->ch.pg_rank);
		MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
		return mpi_errno;
	    }

	    conn->state = CONN_STATE_OPEN_CSEND;
	    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
	    conn->pkt.sc_open_req.pg_id = -1; /* FIXME: multiple process groups may exist */
	    conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;
	    connection_post_send_pkt(conn);

	    break;
	}

    case MPIDU_SOCK_OP_CLOSE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    /* If the conn pointer is NULL then the close was intentional */
	    if (conn != NULL)
	    {
		if (conn->state == CONN_STATE_CLOSING)
		{
#ifdef MPICH_DBG_OUTPUT
		    /*
		    assert(conn->send_active == NULL);
		    assert(conn->recv_active == NULL);
		    */
		    if (conn->send_active != NULL || conn->recv_active != NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**conn_still_active", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			return mpi_errno;
		    }
#endif
		    conn->sock = MPIDU_SOCK_INVALID_SOCK;
		    conn->state = CONN_STATE_CLOSED;
		    connection_free(conn);
		}
		else if (conn->state == CONN_STATE_LISTENING && shutting_down)
		{
#ifdef MPICH_DBG_OUTPUT
		    /*assert(listener_conn == conn);*/
		    if (listener_conn != conn)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_listener", "**invalid_listener %p", conn);
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			return mpi_errno;
		    }
#endif
		    connection_free(listener_conn);
		    listener_conn = NULL;
		    listener_port = 0;
		}
		else
		{ 
		    /* FIXME: an error has occurred */
		}
	    }
	    break;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SSM_VC_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SSM_VC_post_read(MPIDI_VC * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn = vc->ch.conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, vc=%08p, rreq=0x%08x", vc, rreq->handle));

#ifdef MPICH_DBG_OUTPUT
    /*assert (conn->recv_active == NULL);*/
    if (conn->recv_active != NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**multi_post_read", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
	return mpi_errno;
    }
#endif
    conn->recv_active = rreq;
    mpi_errno = MPIDU_Sock_post_readv(conn->sock, rreq->dev.iov + rreq->ch.iov_offset, rreq->dev.iov_count - rreq->ch.iov_offset, NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SSM_VC_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SSM_VC_post_write(MPIDI_VC * vc, MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn = vc->ch.conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, vc=%08p, sreq=0x%08x", vc, sreq->handle));

#ifdef MPICH_DBG_OUTPUT
    /*assert (conn->send_active == NULL);*/
    if (conn->send_active != NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**multi_post_write", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
	return mpi_errno;
    }
    /*assert (!vc->ch.bShm);*/
    if (vc->ch.bShm)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_sock_write_on_shm", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
	return mpi_errno;
    }
#endif

    conn->send_active = sreq;
    mpi_errno = MPIDU_Sock_post_writev(conn->sock, sreq->dev.iov + sreq->ch.iov_offset, sreq->dev.iov_count - sreq->ch.iov_offset, NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    return mpi_errno;
}
