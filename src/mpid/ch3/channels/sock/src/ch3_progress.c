/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "sock.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

enum conn_state
{
    CONN_STATE_UNCONNECTED,
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTING,
    CONN_STATE_OPEN_CSEND,
    CONN_STATE_OPEN_CRECV,
    CONN_STATE_OPEN_LRECV,
    CONN_STATE_OPEN_LSEND,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED,
    CONN_STATE_FAILED
};


typedef struct MPIDI_CH3I_Connection
{
    MPIDI_VC * vc;
    sock_t sock;
    enum conn_state state;
    MPID_Request * send_active;
    MPID_Request * recv_active;
    MPIDI_CH3_Pkt_t pkt;
} MPIDI_CH3I_Connection_t;

static sock_set_t sock_set;
static int listener_port = 0;
static MPIDI_CH3I_Connection_t * listener_conn = NULL;

static int shutting_down = FALSE;


static inline MPIDI_CH3I_Connection_t * connection_alloc(void);
static inline void connection_free(MPIDI_CH3I_Connection_t * conn);
static inline void connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
static inline void connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn);
static inline void connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);
static void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
static void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
static int sock_errno_to_mpi_errno(int sock_errno);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_start()
{
    /* MT - This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_START);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_START);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_START);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress(int is_blocking)
{
    int rc;
    int register count;
    sock_event_t event;
    unsigned completions = MPIDI_CH3I_progress_completions;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    do
    {
	rc = sock_wait(sock_set, is_blocking ? SOCK_INFINITE_TIME : 0, &event);
	if (rc == SOCK_ERR_TIMEOUT)
	{
	    break;
	}
	assert(rc == SOCK_SUCCESS);

	switch (event.op_type)
	{
	    case SOCK_OP_TIMEOUT:
	    {
		goto fn_exit;
	    }
	    
	    case SOCK_OP_READ:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;
		
		MPID_Request * rreq = conn->recv_active;
		
		if (event.error != SOCK_SUCCESS)
		{
		    connection_recv_fail(conn, event.error);
		    break;
		}
		
		if (conn->recv_active)
		{
		    conn->recv_active = NULL;
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
			assert(vc->sc.pg_rank == pg_rank);

			if (vc->sc.conn == NULL || MPIR_Process.comm_world->rank < pg_rank)
			{
			    vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->sc.sock = conn->sock;
			    vc->sc.conn = conn;
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
			    conn->vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			    assert(conn->vc->sc.conn == conn);
			    assert(conn->vc->sc.sock == conn->sock);
			    
			    connection_post_recv_pkt(conn);
			    connection_post_sendq_req(conn);
			}
			else
			{
			    conn->vc = NULL;
			    conn->state = CONN_STATE_CLOSING;
			    sock_post_close(conn->sock);
			}
		    }
		    else
		    {
			int mpi_errno;
			
			mpi_errno = MPIR_Err_create_code(MPI_ERR_INTERN, "[ch3:sock] received packet of unknown type %d",
							 conn->pkt.type);
			MPID_Abort(NULL, mpi_errno);
		    }
		}

		break;
	    }
	    
	    case SOCK_OP_WRITE:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;

		if (event.error != SOCK_SUCCESS)
		{
		    connection_send_fail(conn, event.error);
		    break;
		}
		
		if (conn->send_active)
		{
		    MPID_Request * sreq = conn->send_active;
		    
		    conn->send_active = NULL;
		    MPIDI_CH3U_Handle_send_req(conn->vc, sreq);
		    if (conn->send_active == NULL)
		    {
			MPIDI_CH3I_SendQ_dequeue(conn->vc);
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
			    connection_post_recv_pkt(conn);
			    connection_post_sendq_req(conn);
			}
			else
			{
			    /* head-to-head connections - close this connection */
			    conn->state = CONN_STATE_CLOSING;
			    rc = sock_post_close(conn->sock);
			    assert(rc == SOCK_SUCCESS);
			}
		    }
		}

		break;
	    }
	    
	    case SOCK_OP_ACCEPT:
	    {
		MPIDI_CH3I_Connection_t * conn;

		conn = connection_alloc();
		rc = sock_accept(listener_conn->sock, sock_set, conn, &conn->sock);
		if (rc == SOCK_SUCCESS)
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
	    
	    case SOCK_OP_CONNECT:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;

		if (event.error != SOCK_SUCCESS)
		{
		    int mpi_errno;

		    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "[ch3:sock] failed to connnect to process %d:%d",
						     /* FIXME: pgid */ -1, conn->vc->sc.pg_rank);
		    MPID_Abort(NULL, mpi_errno);

		}
		
		conn->state = CONN_STATE_OPEN_CSEND;
		conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
		conn->pkt.sc_open_req.pg_id = -1; /* FIXME: multiple process groups may exist */
		conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;
		connection_post_send_pkt(conn);
		    
		break;
	    }
	    
	    case SOCK_OP_CLOSE:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;
		
		/* If the conn pointer is NULL then the close was intentional */
		if (conn != NULL)
		{
		    if (conn->state == CONN_STATE_CLOSING)
		    {
			assert(conn->send_active == NULL);
			assert(conn->recv_active == NULL);
			conn->sock = SOCK_INVALID_SOCK;
			conn->state = CONN_STATE_CLOSED;
			connection_free(conn);
		    }
		    else if (conn->state == CONN_STATE_LISTENING && shutting_down)
		    {
			assert(listener_conn == conn);
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
    }
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);

  fn_exit:
    count = MPIDI_CH3I_progress_completions - completions;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", count));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
    return count;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_poke()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    MPIDI_CH3_Progress(FALSE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT: This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_END);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_END);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_END);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int rc;
    sock_t sock;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    rc = sock_init();
    if (rc != SOCK_SUCCESS)
    {
	mpi_errno = sock_errno_to_mpi_errno(rc);
	MPID_Abort(NULL, mpi_errno);
	goto fn_exit;
    }
    
    /* create sock set */
    rc = sock_create_set(&sock_set);
    if (rc != SOCK_SUCCESS)
    {
	mpi_errno = sock_errno_to_mpi_errno(rc);
	MPID_Abort(NULL, mpi_errno);
	goto fn_exit;
    }
    
    /* establish non-blocking listener */
    listener_conn = connection_alloc();
    listener_conn->sock = NULL;
    listener_conn->vc = NULL;
    listener_conn->state = CONN_STATE_LISTENING;
    listener_conn->send_active = NULL;
    listener_conn->recv_active = NULL;
    
    rc = sock_listen(sock_set, listener_conn, &listener_port, &sock);
    if (rc != SOCK_SUCCESS)
    {
	mpi_errno = sock_errno_to_mpi_errno(rc);
	MPID_Abort(NULL, mpi_errno);
	goto fn_exit;
    }
    
    listener_conn->sock = sock;
    
  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    /* MT: in a multi-threaded environment, finalize() should signal any thread(s) blocking on sock_wait() and wait for those
       threads to complete before destroying the progress engine data structures.  We may need to add a function to the sock
       interface for this. */
    
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    MPIR_Nest_incr();
    {
	NMPI_Barrier(MPI_COMM_WORLD); /* FIXME: this barrier may not be necessary */
	shutting_down = TRUE;
	NMPI_Barrier(MPI_COMM_WORLD);
    }
    MPIR_Nest_decr();
    
    /* Shut down the listener */
    rc = sock_post_close(listener_conn->sock);
    assert(rc == SOCK_SUCCESS);

    /* XXX: Wait for listener sock to close */
    
    /* FIXME: Cleanly shutdown other socks and free connection structures. (close protocol?) */

    sock_destroy_set(sock_set);
    sock_finalize();

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Listener_get_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
short MPIDI_CH3I_Listener_get_port()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    return listener_port;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC * vc)
{
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int port;
    int rc;
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    assert(vc->sc.state == MPIDI_CH3I_VC_STATE_UNCONNECTED);
    
    vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
    
    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);
    
    rc = snprintf(key, key_max_sz, "P%d-port", vc->sc.pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = PMI_KVS_Get(vc->sc.pg->kvs_name, key, val);
    assert(rc == 0);
    rc = sscanf(val, "%d", &port);
    
    rc = snprintf(key, key_max_sz, "P%d-hostname", vc->sc.pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = PMI_KVS_Get(vc->sc.pg->kvs_name, key, val);
    assert(rc == 0);

    conn = connection_alloc();

    rc = sock_post_connect(sock_set, conn, val, port, &conn->sock);
    if (rc == SOCK_SUCCESS)
    {
	vc->sc.sock = conn->sock;
	vc->sc.conn = conn;
	conn->vc = vc;
	conn->state = CONN_STATE_CONNECTING;
	conn->send_active = NULL;
	conn->recv_active = NULL;
    }
    else
    {
	if (rc == SOCK_ERR_HOST_LOOKUP)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "[ch3:sock] failed to obtain host information for process %d:%d (%s)",
					     /* FIXME: pgid*/ -1, conn->vc->sc.pg_rank, val);
	}
	else if (rc == SOCK_ERR_CONN_REFUSED)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "[ch3:sock] failed to connect to process %d:%d (%s)",
					     /* FIXME: pgid */ -1, conn->vc->sc.pg_rank, val);
	}
	else
	{
	    mpi_errno = sock_errno_to_mpi_errno(rc);
	}

	MPID_Abort(NULL, mpi_errno);
	
	vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
	connection_free(conn);
    }
    
    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_VC_post_read(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_CH3I_Connection_t * conn = vc->sc.conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    assert (conn->recv_active == NULL);
    conn->recv_active = req;
    sock_post_readv(conn->sock, req->ch3.iov, req->ch3.iov_count, NULL);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_VC_post_write(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_CH3I_Connection_t * conn = vc->sc.conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    
    assert (conn->send_active == NULL);
    conn->send_active = req;
    sock_post_writev(conn->sock, req->ch3.iov, req->ch3.iov_count, NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
}

#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_CH3I_Connection_t * connection_alloc(void)
{
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno;
    
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "ch3:sock unable to allocate a connection structure", 0 );
	MPID_Abort(NULL, mpi_errno);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return conn;
}

#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_FREE);
    
    MPIU_Free(conn);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_FREE);
}

#undef FUNCNAME
#define FUNCNAME connection_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    /* post send of next request on the send queue */
    conn->send_active = MPIDI_CH3I_SendQ_head(conn->vc); /* MT */
    if (conn->send_active != NULL)
    {
	int rc;
	
	rc = sock_post_writev(conn->sock, conn->send_active->ch3.iov, conn->send_active->ch3.iov_count, NULL);
	if (rc != SOCK_SUCCESS)
	{
	    connection_send_fail(conn, rc);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
}

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT);
    
    rc = sock_post_write(conn->sock, &conn->pkt, sizeof(conn->pkt), NULL);
    if (rc != SOCK_SUCCESS)
    {
	connection_send_fail(conn, rc);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT);
}

#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);
    
    rc = sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt), NULL);
    if (rc != SOCK_SUCCESS)
    {
	connection_recv_fail(conn, rc);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
}


#undef FUNCNAME
#define FUNCNAME connection_send_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_SEND_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_SEND_FAIL);

    mpi_errno = sock_errno_to_mpi_errno(sock_errno);

#   if 0
    {
	conn->state = CONN_STATE_FAILED;
	if (conn->vc != NULL)
	{
	    
	    conn->vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
	    MPIDI_CH3U_VC_send_failure(conn->vc, mpi_errno);
	}
    }
#   endif

    if (conn->send_active)
    { 
	MPID_Abort(conn->send_active->comm, mpi_errno);
    }
    else
    {
	MPID_Abort(NULL, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_SEND_FAIL);
}

#undef FUNCNAME
#define FUNCNAME connection_recv_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_RECV_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_RECV_FAIL);

    mpi_errno = sock_errno_to_mpi_errno(sock_errno);
    MPID_Abort(NULL, mpi_errno);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_RECV_FAIL);
}

static int sock_errno_to_mpi_errno(int sock_errno)
{
    int mpi_errno;
    
    switch(sock_errno)
    {
	case SOCK_EOF:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] active connection unexpectedly closed", 0 );
	    break;
	case SOCK_ERR_NOMEM:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] memory allocation failure", 0 );
	    break;
	case SOCK_ERR_HOST_LOOKUP:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] hostname lookup failed", 0 );
	    break;
	case SOCK_ERR_CONN_REFUSED:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] connection refused", 0 );
	    break;
	case SOCK_ERR_CONN_FAILED:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] active connection unexpectedly terminated", 0 );
	    break;
	case SOCK_ERR_BAD_SOCK:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, "[ch3:sock] internal error - bad sock", 0 );
	    break;
	case SOCK_ERR_BAD_BUFFER:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_BUFFER, "[ch3:sock] bad user buffer", 0 );
	    break;
	default:
	    mpi_errno = MPI_ERR_OTHER;
	    break;
    }

    return mpi_errno;
}
