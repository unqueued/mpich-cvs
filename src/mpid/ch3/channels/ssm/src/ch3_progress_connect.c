/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

sock_set_t sock_set;
int listener_port = 0;
MPIDI_CH3I_Connection_t * listener_conn = NULL;

int shutting_down = FALSE;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Listener_get_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Listener_get_port()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    return listener_port;
}

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL)
	return 0;
    sscanf(pszIP, "%u.%u.%u.%u", &a, &b, &c, &d);
    /*MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));*/
    nIP = (d << 24) | (c << 16) | (b << 8) | a;
    return nIP;
}

static unsigned int GetMask(char *pszMask)
{
    int i, nBits;
    unsigned int nMask = 0;
    unsigned int a,b,c,d;

    if (pszMask == NULL)
	return 0;

    if (strstr(pszMask, "."))
    {
	sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	/*MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));*/
	nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else
    {
	nBits = atoi(pszMask);
	for (i=0; i<nBits; i++)
	{
	    nMask = nMask << 1;
	    nMask = nMask | 0x1;
	}
    }
    /*
    unsigned int a, b, c, d;
    a = ((unsigned char *)(&nMask))[0];
    b = ((unsigned char *)(&nMask))[1];
    c = ((unsigned char *)(&nMask))[2];
    d = ((unsigned char *)(&nMask))[3];
    MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));
    */
    return nMask;
}

static int GetHostAndPort(char *host, int *port, char *business_card)
{
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet, nNicMask;
    char *temp, *pszHost, *pszIP, *pszPort;
    unsigned int ip;

    pEnv = getenv("MPICH_NETMASK");
    if (pEnv != NULL)
    {
	MPIU_Strncpy(pszNetMask, pEnv, 50);
	token = strtok(pszNetMask, "/");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nNicNet = GetIP(pszNetMask);
		nNicMask = GetMask(token);

		/* parse each line of the business card and match the ip address with the network mask */
		temp = MPIU_Strdup(business_card);
		token = strtok(temp, ":\r\n");
		while (token)
		{
		    pszHost = token;
		    pszIP = strtok(NULL, ":\r\n");
		    pszPort = strtok(NULL, ":\r\n");
		    ip = GetIP(pszIP);
		    /*msg_printf("masking '%s'\n", pszIP);*/
		    if ((ip & nNicMask) == nNicNet)
		    {
			/* the current ip address matches the requested network so return these values */
			MPIU_Strncpy(host, pszIP, MAXHOSTNAMELEN); /*pszHost);*/
			*port = atoi(pszPort);
			MPIU_Free(temp);
			return MPI_SUCCESS;
		    }
		    token = strtok(NULL, ":\r\n");
		}
		if (temp)
		    MPIU_Free(temp);
	    }
	}
    }

    temp = MPIU_Strdup(business_card);
    if (temp == NULL)
    {
	/*MPIDI_err_printf("GetHostAndPort", "MPIU_Strdup failed\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MPIU_Strdup", 0);
    }
    /* move to the host part */
    token = strtok(temp, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    /*strcpy(host, token);*/
    /* move to the ip part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    MPIU_Strncpy(host, token, MAXHOSTNAMELEN); /* use the ip string instead of the hostname, it's more reliable */
    /* move to the port part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    *port = atoi(token);
    MPIU_Free(temp);

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Shm_connect(MPIDI_VC *vc, char *business_card, int *flag)
{
    int mpi_errno;
    char hostname[256];
    char *queue_name;
    MPIDI_CH3I_BootstrapQ queue;
    MPIDI_CH3I_Shmem_queue_info shm_info/*, shm_remote_info*/;
    /*int num_bytes;*/
    int i;

#if 0
    /* for now always return false until the sock stuff works */
    *flag = FALSE;
    return MPI_SUCCESS;
#endif

    hostname[0] = '\0';
    MPIU_Strncpy(hostname, business_card, 256);
    queue_name = strtok(hostname, ":");
    if (queue_name == NULL)
    {
	*flag = FALSE;
	return MPI_SUCCESS;
    }
    queue_name = strtok(NULL, "\n");
    if (queue_name == NULL)
    {
	*flag = FALSE;
	return MPI_SUCCESS;
    }

    if (strcmp(MPIDI_CH3I_Process.pg->shm_hostname, hostname) != 0)
    {
	*flag = FALSE;
	/*MPIU_DBG_PRINTF(("%s != %s\n", MPIDI_CH3I_Process.pg->shm_hostname, hostname));*/
	return MPI_SUCCESS;
    }

    *flag = TRUE;
    /*MPIU_DBG_PRINTF(("%s == %s\n", MPIDI_CH3I_Process.pg->shm_hostname, hostname));*/

    /*MPIU_DBG_PRINTF(("attaching to queue: %s\n", queue_name));*/
    mpi_errno = MPIDI_CH3I_BootstrapQ_attach(queue_name, &queue);
    if (mpi_errno != MPI_SUCCESS)
    {
	*flag = FALSE;
	return MPI_SUCCESS;
    }

    /* create the write queue */
    mpi_errno = MPIDI_CH3I_SHM_Get_mem(sizeof(MPIDI_CH3I_SHM_Queue_t), &vc->mm.shm_write_queue_info);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmconnect_getmem", 0);
	return mpi_errno;
    }
    vc->mm.write_shmq = vc->mm.shm_write_queue_info.addr;
    vc->mm.write_shmq->head_index = 0;
    vc->mm.write_shmq->tail_index = 0;
    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq head = 0"));
    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = 0"));
    for (i=0; i<MPIDI_CH3I_NUM_PACKETS; i++)
    {
	vc->mm.write_shmq->packet[i].offset = 0;
	vc->mm.write_shmq->packet[i].avail = MPIDI_CH3I_PKT_EMPTY;
    }

    /* send the queue connection information */
    /*MPIU_DBG_PRINTF(("write_shmq: %p, name - %s\n", vc->mm.write_shmq, vc->mm.shm_write_queue_info.key));*/
    shm_info.info = vc->mm.shm_write_queue_info;
    shm_info.pg_id = 0;
    shm_info.pg_rank = MPIR_Process.comm_world->rank;
    shm_info.pid = getpid();
    MPIU_DBG_PRINTF(("MPIDI_CH3I_Shm_connect: sending bootstrap queue info from rank %d to msg queue %s\n", MPIR_Process.comm_world->rank, queue_name));
    mpi_errno = MPIDI_CH3I_BootstrapQ_send_msg(queue, &shm_info, sizeof(shm_info));
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_send", 0);
	return mpi_errno;
    }

    /* MPIU_Free the queue resource */
    /*MPIU_DBG_PRINTF(("detaching from queue: %s\n", queue_name));*/
    mpi_errno = MPIDI_CH3I_BootstrapQ_detach(queue);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_detach", 0);
	return mpi_errno;
    }

    return MPI_SUCCESS;
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
    char host[MAXHOSTNAMELEN];
    int port;
    int rc;
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno = MPI_SUCCESS;
    int connected;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    assert(vc->mm.state == MPIDI_CH3I_VC_STATE_UNCONNECTED);

    vc->mm.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    rc = snprintf(key, key_max_sz, "P%d-shm_businesscard", vc->mm.pg_rank);
    if (rc < 0 || rc > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", rc);
	return mpi_errno;
    }
    rc = PMI_KVS_Get(vc->mm.pg->kvs_name, key, val);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", rc);
	return mpi_errno;
    }

    connected = FALSE;
    rc = MPIDI_CH3I_Shm_connect(vc, val, &connected);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_connect", "**post_connect %s", "MPIDI_CH3I_Shm_connect");
	return mpi_errno;
    }
    if (connected)
    {
	MPIU_Free(val);
	MPIU_Free(key);

	/*MPIU_DBG_PRINTF(("shmem connected\n"));*/
	vc->mm.shm_next_writer = MPIDI_CH3I_Process.shm_writing_list;
	MPIDI_CH3I_Process.shm_writing_list = vc;

	vc->mm.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	vc->mm.bShm = TRUE;
	vc->mm.shm_reading_pkt = TRUE;
	vc->mm.send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */

	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    rc = snprintf(key, key_max_sz, "P%d-sock_businesscard", vc->mm.pg_rank);
    if (rc < 0 || rc > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", rc);
	return mpi_errno;
    }
    rc = PMI_KVS_Get(vc->mm.pg->kvs_name, key, val);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", rc);
	return mpi_errno;
    }

    rc = GetHostAndPort(host, &port, val);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_connect", "**post_connect %s", "GetHostAndPort");
	return mpi_errno;
    }
    /*MPIU_DBG_PRINTF(("Connecting to: host %s, port %d\n", host, port));*/

    conn = connection_alloc();
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", "**nomem %s", "connection");
	return mpi_errno;
    }

    rc = sock_post_connect(sock_set, conn, host, port, &conn->sock);
    if (rc == SOCK_SUCCESS)
    {
	vc->mm.sock = conn->sock;
	vc->mm.conn = conn;
	conn->vc = vc;
	conn->state = CONN_STATE_CONNECTING;
	conn->send_active = NULL;
	conn->recv_active = NULL;
    }
    else
    {
	if (rc == SOCK_ERR_HOST_LOOKUP)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**hostlookup", "**hostlookup %d %d %s",
					     /* FIXME: pgid*/ -1, conn->vc->mm.pg_rank, val);
	}
	else if (rc == SOCK_ERR_CONN_REFUSED)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**connrefused", "**connrefused %d %d %s",
					     /* FIXME: pgid */ -1, conn->vc->mm.pg_rank, val);
	}
	else
	{
	    mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(rc, FCNAME);
	}

	/*MPID_Abort(NULL, mpi_errno);*/

	vc->mm.state = MPIDI_CH3I_VC_STATE_FAILED;
	connection_free(conn);
    }

    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPIDI_CH3I_Connection_t * connection_alloc(void)
{
    MPIDI_CH3I_Connection_t * conn;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return conn;
}

#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void connection_free(MPIDI_CH3I_Connection_t * conn)
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
void connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
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
void connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
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
void connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

#ifdef MPICH_DBG_OUTPUT
    memset(&conn->pkt, 0, sizeof(conn->pkt));
#endif
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
void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_SEND_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_SEND_FAIL);

    mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(sock_errno, FCNAME);

#   if 0
    {
	conn->state = CONN_STATE_FAILED;
	if (conn->vc != NULL)
	{
	    
	    conn->vc->mm.state = MPIDI_CH3I_VC_STATE_FAILED;
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
void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_RECV_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_RECV_FAIL);

    mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(sock_errno, FCNAME);
    MPID_Abort(NULL, mpi_errno);

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_RECV_FAIL);
}
