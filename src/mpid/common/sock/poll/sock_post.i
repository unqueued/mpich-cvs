/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_connect
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_connect(struct MPIDU_Sock_set * sock_set, void * user_ptr, char * host_description, int port,
			    struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    struct hostent * hostent;
    int fd = -1;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CONNECT);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * Create a non-blocking socket with Nagle's algorithm disabled
     */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|socket", "**sock|poll|socket %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nodelay", "**sock|poll|nodelay %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    /*
     * Allocate and initialize sock and poll structures
     *
     * NOTE: pollfd->fd is initialized to -1.  It is only set to the true fd value when an operation is posted on the sock.  This
     * (hopefully) eliminates a little overhead in the OS and avoids repetitive POLLHUP events when the connection is closed by
     * the remote process.
     */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;

    /*
     * Convert hostname to IP address
     *
     * FIXME: this should handle failures caused by a backed up listener queue at the remote process.  It should also use a
     * specific interface if one is specified by the user.
     */
    strtok(host_description, " ");
    hostent = gethostbyname(host_description);
    if (hostent == NULL || hostent->h_addrtype != AF_INET)
    {
	/* FIXME: we should make multiple attempts and try different interfaces */
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|hostres",
	    "**sock|poll|hostres %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, host_description), mpi_errno, fn_fail);
	pollinfo->os_errno = errno;
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
	
	goto fn_exit;
    }
    MPIU_Assert(hostent->h_length == sizeof(addr.sin_addr.s_addr));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, hostent->h_addr_list[0], sizeof(addr.sin_addr.s_addr));
    addr.sin_port = htons(port);

    /*
     * Set and verify the socket buffer size
     */
    if (MPIDU_Socki_socket_bufsz > 0)
    {
	int bufsz;
	socklen_t bufsz_len;

	bufsz = MPIDU_Socki_socket_bufsz;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, bufsz_len);
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|setsndbufsz",
		"**sock|poll|setsndbufsz %d %d %s", bufsz, errno, MPIU_Strerror(errno));
	    goto fn_fail;
	    
	}
	
	bufsz = MPIDU_Socki_socket_bufsz;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, bufsz_len);
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|setrcvbufsz",
		"**sock|poll|setrcvbufsz %d %d %s", bufsz, errno, MPIU_Strerror(errno));
	    goto fn_fail;
	    
	}
	
	bufsz_len = sizeof(bufsz);
	rc = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);
	if (rc == 0)
	{
	    if (bufsz < MPIDU_Socki_socket_bufsz * 0.9 || bufsz < MPIDU_Socki_socket_bufsz * 1.0)
	    {
		MPIU_Msg_printf("WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDU_Socki_socket_bufsz, bufsz);
	    }
	}

    	bufsz_len = sizeof(bufsz);
	rc = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, &bufsz_len);
	if (rc == 0)
	{
	    if (bufsz < MPIDU_Socki_socket_bufsz * 0.9 || bufsz < MPIDU_Socki_socket_bufsz * 1.0)
	    {
		MPIU_Msg_printf("WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDU_Socki_socket_bufsz, bufsz);
	    }
	}
    }
    
    /*
     * Attempt to establish the connection
     */
    do
    {
        rc = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    while (rc == -1 && errno == EINTR);
    
    if (rc == 0)
    {
	/* connection succeeded */
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPI_SUCCESS, mpi_errno, fn_fail);
    }
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTING;
	MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);
    }
    else
    {
	pollinfo->os_errno = errno;
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;

	if (errno == ECONNREFUSED)
	{
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
		"**sock|connrefused", "**sock|poll|connrefused %d %d %s",
		pollinfo->sock_set->id, pollinfo->sock_id, host_description), mpi_errno, fn_fail);
	}
	else
	{
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
		"**sock|oserror", "**sock|poll|oserror %d %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, errno,
		MPIU_Strerror(errno)), mpi_errno, fn_fail);
	}
    }

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
    return mpi_errno;

  fn_fail:
    if (fd != -1)
    { 
	close(fd);
    }

    if (sock != NULL)
    {
	MPIDU_Socki_sock_free(sock);
    }

    goto fn_exit;
}
/* end MPIDU_Sock_post_connect() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_listen
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
#ifndef USHRT_MAX
#define USHRT_MAX 65535   /* 2^16-1 */
#endif
int MPIDU_Sock_listen(struct MPIDU_Sock_set * sock_set, void * user_ptr, int * port, struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    long flags;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    if (*port < 0 || *port > USHRT_MAX)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_PORT,
					 "**sock|badport", "**sock|badport %d", *port);
	goto fn_exit;
    }

    /*
     * Create a non-blocking socket for the listener
     */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|socket", "**sock|poll|socket %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    /* set SO_REUSEADDR to a prevent a fixed service port from being bound to during subsequent invocations */
    if (*port != 0)
    {
	flags = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(long));
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|reuseaddr", "**sock|poll|reuseaddr %d %s", errno, MPIU_Strerror(errno));
	    goto fn_fail;
	}
    }

    /* make the socket non-blocking so that accept() will return immediately if no new connection is available */
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    /*
     * Bind the socket to all interfaces and the specified port.  The port specified by the calling routine may be 0, indicating
     * that the operating system can select an available port in the ephemeral port range.
     */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short) *port);
    rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    /*
     * Set and verify the socket buffer size
     */
    if (MPIDU_Socki_socket_bufsz > 0)
    {
	int bufsz;
	socklen_t bufsz_len;

	bufsz = MPIDU_Socki_socket_bufsz;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, bufsz_len);
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|setsndbufsz",
		"**sock|poll|setsndbufsz %d %d %s", bufsz, errno, MPIU_Strerror(errno));
	    goto fn_fail;
	    
	}
	
	bufsz = MPIDU_Socki_socket_bufsz;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, bufsz_len);
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|setrcvbufsz",
		"**sock|poll|setrcvbufsz %d %d %s", bufsz, errno, MPIU_Strerror(errno));
	    goto fn_fail;
	    
	}
	
	bufsz_len = sizeof(bufsz);
	rc = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);
	if (rc == 0)
	{
	    if (bufsz < MPIDU_Socki_socket_bufsz * 0.9 || bufsz < MPIDU_Socki_socket_bufsz * 1.0)
	    {
		MPIU_Msg_printf("WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDU_Socki_socket_bufsz, bufsz);
	    }
	}

    	bufsz_len = sizeof(bufsz);
	rc = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, &bufsz_len);
	if (rc == 0)
	{
	    if (bufsz < MPIDU_Socki_socket_bufsz * 0.9 || bufsz < MPIDU_Socki_socket_bufsz * 1.0)
	    {
		MPIU_Msg_printf("WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDU_Socki_socket_bufsz, bufsz);
	    }
	}
    }
    
    /*
     * Start listening for incoming connections...
     */
    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|listen", "**sock|poll|listen %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    /*
     * Get listener port.  Techincally we don't need to do this if a port was specified by the calling routine; but it adds an
     * extra error check.
     */
    addr_len = sizeof(addr);
    rc = getsockname(fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|getport", "**sock|poll|getport %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    *port = (unsigned int) ntohs(addr.sin_port);

    /*
     * Allocate and initialize sock and poll structures.  If another thread is blocking in poll(), that thread must be woke up
     * long enough to pick up the addition of the listener socket.
     */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_LISTENER;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
    return mpi_errno;

  fn_fail:
    if (fd != -1)
    { 
	close(fd);
    }

    goto fn_exit;
}
/* end MPIDU_Sock_listen() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_read
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_read(struct MPIDU_Sock * sock, void * buf, MPIU_Size_t minlen, MPIU_Size_t maxlen,
			 MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badlen", "**sock|badlen %d %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, minlen, maxlen);
	goto fn_exit;
    }

    pollinfo->read.buf.ptr = buf;
    pollinfo->read.buf.min = minlen;
    pollinfo->read.buf.max = maxlen;
    pollinfo->read_iov_flag = FALSE;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    
    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READ);
    return mpi_errno;
}
/* end MPIDU_Sock_post_read() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_readv
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_readv(struct MPIDU_Sock * sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READV);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    if (iov_n < 1 || iov_n > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badiovn", "**sock|badiovn %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, iov_n);
	goto fn_exit;
    }

    pollinfo->read.iov.ptr = iov;
    pollinfo->read.iov.count = iov_n;
    pollinfo->read.iov.offset = 0;
    pollinfo->read_iov_flag = TRUE;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
    return mpi_errno;
}
/* end MPIDU_Sock_post_readv() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_write(struct MPIDU_Sock * sock, void * buf, MPIU_Size_t minlen, MPIU_Size_t maxlen,
			  MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badlen", "**sock|badlen %d %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, minlen, maxlen);
	goto fn_exit;
    }

    pollinfo->write.buf.ptr = buf;
    pollinfo->write.buf.min = minlen;
    pollinfo->write.buf.max = maxlen;
    pollinfo->write_iov_flag = FALSE;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    return mpi_errno;
}
/* end MPIDU_Sock_post_write() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_writev
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_writev(struct MPIDU_Sock * sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    if (iov_n < 1 || iov_n > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badiovn", "**sock|badiovn %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, iov_n);
	goto fn_exit;
    }

    pollinfo->write.iov.ptr = iov;
    pollinfo->write.iov.count = iov_n;
    pollinfo->write.iov.offset = 0;
    pollinfo->write_iov_flag = TRUE;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    return mpi_errno;
}
/* end MPIDU_Sock_post_writev() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_close
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_close(struct MPIDU_Sock * sock)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    
    if (pollinfo->state == MPIDU_SOCKI_STATE_CLOSING)
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**sock|closing_already",
	    "**sock|closing_already %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }

    if (pollinfo->type == MPIDU_SOCKI_TYPE_COMMUNICATION)
    {
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN | POLLOUT))
	{
	    int event_mpi_errno;
	    
	    event_mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_SOCK_CLOSED, "**sock|close_cancel",
		"**sock|close_cancel %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	    
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN))
	    {
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					  MPI_SUCCESS, mpi_errno, fn_exit);
	    }

	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  MPI_SUCCESS, mpi_errno, fn_exit);
	    }
	
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	}
    }
    else /* if (pollinfo->type == MPIDU_SOCKI_TYPE_LISTENER) */
    {
	/*
	 * The event queue may contain an accept event which means that MPIDU_Sock_accept() may be legally called after
	 * MPIDU_Sock_post_close().  However, MPIDU_Sock_accept() must be called before the close event is return by
	 * MPIDU_Sock_wait().
	 */
	MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }
    
    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CLOSE, 0, pollinfo->user_ptr, MPI_SUCCESS, mpi_errno, fn_exit);
    pollinfo->state = MPIDU_SOCKI_STATE_CLOSING;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
    return mpi_errno;
}
