/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sock.h"
#include "mpiimpl.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <netdb.h>
#include <errno.h>

#if !defined(SOCK_SET_DEFAULT_SIZE)
#define SOCK_SET_DEFAULT_SIZE 16
#endif

#define SOCKI_QUOTE(A) SOCKI_QUOTE2(A)
#define SOCKI_QUOTE2(A) #A

enum sock_state
{
    SOCK_STATE_UNCONNECTED,
    SOCK_STATE_LISTENING,
    SOCK_STATE_CONNECTING,
    SOCK_STATE_CONNECTED,
    SOCK_STATE_CLOSED,
    SOCK_STATE_FAILED
};

struct pollinfo
{
    struct sock * sock;
    void * user_ptr;
    enum sock_state state;
    SOCK_IOV * read_iov;
    int read_iov_count;
    int read_iov_offset;
    sock_size_t read_nb;
    SOCK_IOV read_iov_1;
    sock_progress_update_func_t read_progress_update_fn;
    SOCK_IOV * write_iov;
    int write_iov_count;
    int write_iov_offset;
    sock_size_t write_nb;
    SOCK_IOV write_iov_1;
    sock_progress_update_func_t write_progress_update_fn;
};

struct sock_eventq_elem
{
    struct sock_event event;
    struct sock_eventq_elem * next;
};

struct sock_set
{
    int poll_arr_sz;
    int poll_n_elem;
    int starting_elem;
    struct pollfd * pollfds;
    struct pollinfo * pollinfos;
    struct sock_eventq_elem * eventq_head;
    struct sock_eventq_elem * eventq_tail;
};

struct sock
{
    int fd;
    struct sock_set * sock_set;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
};



static void socki_handle_accept(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static void socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static void socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static void socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int socki_sock_alloc(struct sock_set * sock_set, struct sock ** sockp);
static void socki_sock_free(struct sock * sock);
static void socki_event_enqueue(struct sock_set * sock_set, sock_op_t op, sock_size_t num_bytes, void * user_ptr, int error);
static int socki_event_dequeue(struct sock_set * sock_set, sock_event_t * eventp);
static int socki_adjust_iov(MPIDI_msg_sz_t nb, SOCK_IOV * const iov, const int count, int * const offsetp);
static int socki_errno_to_sock_errno(int error);

#undef FUNCNAME
#define FUNCNAME sock_init
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_init(void)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_finalize
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_FINALIZE);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_create_set
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_create_set(sock_set_t * sock_set)
{
    struct sock_set * set;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_CREATE_SET);
    
    set = MPIU_Malloc(sizeof(struct sock_set));
    if (sock_set == NULL) return SOCK_FAIL;
    set->poll_arr_sz = 0;
    set->poll_n_elem = -1;
    set->pollfds = NULL;
    set->pollinfos = NULL;
    *sock_set = set;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_CREATE_SET);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_destroy_set
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_destroy_set(sock_set_t sock_set)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_DESTROY_SET);
    
    /* XXX: close unclosed socks in set?? */
    MPIU_Free(sock_set->pollinfos);
    MPIU_Free(sock_set->pollfds);
    MPIU_Free(sock_set);
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_DESTROY_SET);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_set_user_ptr
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_set_user_ptr(sock_t sock, void * user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_SET_USER_PTR);
    
    sock->pollinfo->user_ptr = user_ptr;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_SET_USER_PTR);
    return SOCK_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME sock_listen
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_listen(sock_set_t sock_set, void * user_ptr, int * port, sock_t * sockp)
{
    int fd;
    long flags;
    struct sockaddr_in addr;
    socklen_t addr_len;
    struct sock * sock;
    int rc;
    int sock_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_LISTEN);
    
     /* establish non-blocking listener */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
    
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
                
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
    
    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    /* get listener port */
    addr_len = sizeof(addr);
    rc = getsockname(fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
    *port = ntohs(addr.sin_port);

    /* allocate and initialize sock and poll structures */
    sock_errno = socki_sock_alloc(sock_set, &sock);
    if (sock_errno != SOCK_SUCCESS)
    {
	goto fail_close;
    }

    sock->fd = fd;
    sock->pollfd->fd = fd;
    sock->pollfd->events = POLLIN;
    sock->pollfd->revents = 0;
    sock->pollinfo->user_ptr = user_ptr;
    sock->pollinfo->state = SOCK_STATE_LISTENING;

    *sockp = sock;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
    return SOCK_SUCCESS;

  fail_close:
    close(fd);
  fail:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_post_connect
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_connect(sock_set_t sock_set, void * user_ptr, char * host, int port, sock_t * sockp)
{
    struct hostent * hostent;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int fd;
    struct sock * sock;
    int rc;
    int sock_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CONNECT);

    /* create nonblocking socket */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    /* allocate and initialize sock and poll structures */
    sock_errno = socki_sock_alloc(sock_set, &sock);
    if (sock_errno != SOCK_SUCCESS)
    {
	goto fail_close;
    }
    sock->pollinfo->user_ptr = user_ptr;

    sock->fd = fd;
    sock->pollfd->fd = fd;
    sock->pollfd->events = 0;
    sock->pollfd->revents = 0;
    sock->pollinfo->user_ptr = user_ptr;
    sock->pollinfo->state = SOCK_STATE_CONNECTING;
    
    /* convert hostname to IP address */
    hostent = gethostbyname(host);
    if (hostent == NULL || hostent->h_addrtype != AF_INET)
    {
	socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_ERR_HOST_LOOKUP);
	goto fn_exit;
    }
    assert(hostent->h_length == sizeof(addr.sin_addr.s_addr));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, hostent->h_addr_list[0], sizeof(addr.sin_addr.s_addr));
    addr.sin_port = htons(port);

    /* attempt to establish connection */
    do
    {
        rc = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    while (rc == -1 && errno == EINTR);
    
    if (rc == 0)
    {
	/* connection succeeded */
	socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_SUCCESS);
	sock->pollinfo->state = SOCK_STATE_CONNECTED;
    }
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	sock->pollfd->events |= POLLOUT;
    }
    else
    {
	if (errno == ECONNREFUSED)
	{ 
	    socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_ERR_CONN_REFUSED);
	}
	else
	{
	    socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_FAIL);
	}
    }

  fn_exit:
    *sockp = sock;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return SOCK_SUCCESS;

  fail_close:
    close(fd);
  fail:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_post_close
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_close(sock_t sock)
{
    int rc;
    int flags;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CLOSE);

    if (sock->pollinfo->state == SOCK_STATE_LISTENING)
    {
	sock->pollfd->events &= ~POLLIN;
    }

    if (sock->pollfd->events & (POLLIN | POLLOUT))
    {
	return SOCK_FAIL;
    }

    socki_event_enqueue(sock->sock_set, SOCK_OP_CLOSE, 0, sock->pollinfo->user_ptr, SOCK_SUCCESS);

    flags = fcntl(sock->fd, F_GETFL, 0);
    assert(flags != -1);
    rc = fcntl(sock->fd, F_SETFL, flags & ~O_NONBLOCK);
    assert(rc != -1);
    
    close(sock->fd);
    socki_sock_free(sock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_read
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_read(sock_t sock, void * buf, int len, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READ);

    assert ((sock->pollfd->events & POLLIN) == 0);
    
    pollinfo->read_iov = &pollinfo->read_iov_1;
    pollinfo->read_iov_count = 1;
    pollinfo->read_iov_offset = 0;
    pollinfo->read_iov->SOCK_IOV_BUF = buf;
    pollinfo->read_iov->SOCK_IOV_LEN = len;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    sock->pollfd->events |= POLLIN;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READ);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_readv
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_readv(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READV);

    assert ((sock->pollfd->events & POLLIN) == 0);
    
    pollinfo->read_iov = iov;
    pollinfo->read_iov_count = n;
    pollinfo->read_iov_offset = 0;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    sock->pollfd->events |= POLLIN;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READV);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_write
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_write(sock_t sock, void * buf, int len, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITE);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    pollinfo->write_iov = &pollinfo->write_iov_1;
    pollinfo->write_iov_count = 1;
    pollinfo->write_iov_offset = 0;
    pollinfo->write_iov->SOCK_IOV_BUF = buf;
    pollinfo->write_iov->SOCK_IOV_LEN = len;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;
    sock->pollfd->events |= POLLOUT;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITE);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_writev
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_writev(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITEV);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    pollinfo->write_iov = iov;
    pollinfo->write_iov_count = n;
    pollinfo->write_iov_offset = 0;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;
    sock->pollfd->events |= POLLOUT;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITEV);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_wait
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_wait(sock_set_t sock_set, int millisecond_timeout, sock_event_t * eventp)
{
    int elem;
    int nfds;
    int found_active_elem = FALSE;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WAIT);
    

    for (;;)
    { 
	if (socki_event_dequeue(sock_set, eventp) == SOCK_SUCCESS)
	{
	    break;
	}

	do
	{
	    nfds = poll(sock_set->pollfds, sock_set->poll_n_elem, millisecond_timeout);
	}
	while (nfds < 0 && errno == EINTR);

	if (nfds == 0)
	{
	    sock_errno = SOCK_ERR_TIMEOUT;
	    break;
	}

	if (nfds == -1) 
	{
	    if (errno == ENOMEM)
	    {
		sock_errno = SOCK_ERR_NOMEM;
	    }
	    else
	    {
		assert(errno == ENOMEM);
		sock_errno = SOCK_FAIL;
	    }

	    goto fn_exit;
	}
	    
	elem = sock_set->starting_elem;
	while (nfds > 0)
	{
	    struct pollfd * const pollfd = &sock_set->pollfds[elem];
	    struct pollinfo * const pollinfo = &sock_set->pollinfos[elem];
	
	    if (pollfd->revents == 0)
	    {
		/* This optimization assumes that most FDs will not have a pending event. */
		elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
		continue;
	    }

	    assert((pollfd->revents & POLLNVAL) == 0);
	
	    if (found_active_elem == FALSE)
	    {
		found_active_elem = TRUE;
		sock_set->starting_elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	    }

	    /* According to Stevens, some errors are reported as normal data and some are reported with POLLERR.  */
	    if (pollfd->revents & (POLLIN | POLLERR | POLLHUP))
	    {
		if (pollinfo->state == SOCK_STATE_CONNECTED)
		{
		    socki_handle_read(pollfd, pollinfo);
		}
		else if (pollinfo->state == SOCK_STATE_LISTENING)
		{
		    socki_handle_accept(pollfd, pollinfo);
		}
		else
		{
		    /* The connection must have failed. */
		    if (pollfd->events & POLLIN)
		    { 
			socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					    SOCK_ERR_CONN_FAILED);
			pollfd->events &= ~POLLIN;
		    }
		}
	    }

	    if (pollfd->revents & POLLOUT)
	    {
		if (pollinfo->state == SOCK_STATE_CONNECTED)
		{
		    socki_handle_write(pollfd, pollinfo);
		}
		else if (pollinfo->state == SOCK_STATE_CONNECTING)
		{
		    socki_handle_connect(pollfd, pollinfo);
		}
		else
		{
		    /* The connection must have failed. */
		    if (pollfd->events & POLLOUT)
		    { 
			socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					    SOCK_ERR_CONN_FAILED);
			pollfd->events &= ~POLLOUT;
		    }
		}
	    }

	    nfds--;
	    elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	}
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_accept
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_accept(sock_t listener, sock_set_t sock_set, void * user_ptr, sock_t * sockp)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_ACCEPT);
    
    addr_len = sizeof(struct sockaddr_in);
    fd = accept(listener->fd, (struct sockaddr *) &addr, &addr_len);
    if (fd >= 0)
    {
	struct sock * sock;
	long flags;
	int nodelay;
	int rc;

	sock_errno = socki_sock_alloc(sock_set, &sock);
	if (sock_errno != SOCK_SUCCESS)
	{
	    goto fn_exit;
	}
	    
	flags = fcntl(fd, F_GETFL, 0);
	assert(flags != -1);
	rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	assert(rc != -1);
	    
	nodelay = 1;
	rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
	assert(rc == 0);

	sock->fd = fd;
	sock->pollfd->fd = fd;
	sock->pollfd->events = 0;
	sock->pollfd->revents = 0;
	sock->pollinfo->user_ptr = user_ptr;
	sock->pollinfo->state = SOCK_STATE_CONNECTED;

	*sockp = sock;
    }
    else
    {
	sock_errno = SOCK_FAIL;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_read
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_read(sock_t sock, void * buf, int len, int * num_read)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READ);
    
    assert ((sock->pollfd->events & POLLIN) == 0);
    
    do
    {
	nb = read(sock->fd, buf, len);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	sock_errno = SOCK_EOF;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else
	{
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READ);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_readv
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_readv(sock_t sock, SOCK_IOV * iov, int n, int * num_read)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READV);
    
    assert ((sock->pollfd->events & POLLIN) == 0);
    
    do
    {
	nb = readv(sock->fd, iov, n);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	sock_errno = SOCK_EOF;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else
	{
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READV);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_write
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_write(sock_t sock, void * buf, int len, int * num_written)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITE);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    do
    {
	nb = write(sock->fd, buf, len);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = nb;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_written = 0;
	}
	else
	{
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_writev
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_writev(sock_t sock, SOCK_IOV * iov, int n, int * num_written)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITEV);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    do
    {
	nb = writev(sock->fd, iov, n);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = nb;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_written = 0;
	}
	else
	{
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_getid
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_getid(sock_t sock)
{
    return sock->fd;
}

#undef FUNCNAME
#define FUNCNAME socki_handle_accept
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_handle_accept(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_ACCEPT);
    
    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_ACCEPT, 0, pollinfo->user_ptr, SOCK_SUCCESS);
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_ACCEPT);
}

#undef FUNCNAME
#define FUNCNAME socki_handle_connect
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_CONNECT);
    
    rc = getpeername(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == 0)
    {
	socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_CONNECT, 0, pollinfo->user_ptr, SOCK_SUCCESS);
	pollinfo->state = SOCK_STATE_CONNECTED;
    }
    else
    {
	/* FIXME: if getpeername() returns ENOTCONN, then we can now use getsockopt() to get the errno associated with the failed
	   connect(). */
	assert(errno != ENOTCONN);
	socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_CONNECT, 0, pollinfo->user_ptr, SOCK_ERR_CONN_FAILED);
	pollinfo->state = SOCK_STATE_FAILED;
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_CONNECT);
}

#undef FUNCNAME
#define FUNCNAME socki_handle_read
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_READ);

    assert(pollfd->events & POLLIN);
    
    do
    {
	nb = readv(pollfd->fd, pollinfo->read_iov + pollinfo->read_iov_offset,
		   pollinfo->read_iov_count - pollinfo->read_iov_offset);
    }
    while (nb < 0 && errno == EINTR);

    if (nb > 0)
    {
	pollinfo->read_nb += nb;
		
	if (socki_adjust_iov(nb, pollinfo->read_iov, pollinfo->read_iov_count, &pollinfo->read_iov_offset))
	{
	    pollfd->events &= ~POLLIN;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, SOCK_SUCCESS);
	}
    }
    else if (nb == 0)
    {
	pollfd->events &= ~POLLIN;
	socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, SOCK_EOF);
    }
    else
    {
	if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
	    int sock_errno;

	    sock_errno = socki_errno_to_sock_errno(errno);
	    pollfd->events &= ~POLLIN;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, sock_errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_READ);
}

#undef FUNCNAME
#define FUNCNAME socki_handle_write
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_WRITE);

    do
    {
	nb = writev(pollfd->fd, pollinfo->write_iov + pollinfo->write_iov_offset,
		    pollinfo->write_iov_count - pollinfo->write_iov_offset);
    }
    while (nb < 0 && errno == EINTR);

    if (nb > 0)
    {
	pollinfo->write_nb += nb;
	
	if (socki_adjust_iov(nb, pollinfo->write_iov, pollinfo->write_iov_count, &pollinfo->write_iov_offset))
	{
	    pollfd->events &= ~POLLOUT;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr, SOCK_SUCCESS);
	}
    }
    else
    {
	assert(nb < 0);
	
	if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
	    int sock_errno;

	    sock_errno = socki_errno_to_sock_errno(errno);
	    
	    pollfd->events &= ~POLLOUT;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr, sock_errno);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_WRITE);
}

#undef FUNCNAME
#define FUNCNAME socki_sock_alloc
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static int socki_sock_alloc(struct sock_set * sock_set, struct sock ** sockp)
{
    struct sock * sock;
    int elem;
    struct pollfd * fds;
    struct pollinfo * infos;
    int sock_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_SOCK_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_SOCK_ALLOC);
    
    sock = MPIU_Malloc(sizeof(struct sock));
    if (sock == NULL)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fail;
    }
    
    for (elem = 0; elem < sock_set->poll_arr_sz; elem++)
    {
        if (sock_set->pollinfos[elem].sock == NULL)
        {
            if (elem >= sock_set->poll_n_elem)
            {
                sock_set->poll_n_elem = elem + 1;
            }
	    
	    /* Initialize new sock structure and associated poll structures */
	    sock_set->pollfds[elem].fd = -1;
	    sock_set->pollfds[elem].events = 0;
	    sock_set->pollfds[elem].revents = 0;
	    sock_set->pollinfos[elem].sock = sock;
	    sock->sock_set = sock_set;
	    sock->fd = -1;
	    sock->pollfd = &sock_set->pollfds[elem];
	    sock->pollinfo = &sock_set->pollinfos[elem];

	    *sockp = sock;
	    return SOCK_SUCCESS;
        }
    }

    /* No more pollfd and pollinfo elements.  Resize... */
    fds = MPIU_Malloc((sock_set->poll_arr_sz + SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
    if (fds == NULL)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fail_free_sock;
    }
    infos = MPIU_Malloc((sock_set->poll_arr_sz + SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
    if (infos == NULL)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fail_free_fds;
    }
    
    if (sock_set->poll_arr_sz > 0)
    {
	/* Copy information from old arrays */
        memcpy(fds, sock_set->pollfds, sock_set->poll_arr_sz * sizeof(struct pollfd));
        memcpy(infos, sock_set->pollinfos, sock_set->poll_arr_sz * sizeof(struct pollinfo));

	/* Fix up pollfd pointer in sock structure */
	for (elem = 0; elem < sock_set->poll_arr_sz; elem++)
	{
	    sock_set->pollinfos[elem].sock->pollfd = &sock_set->pollfds[elem];
	}

	/* Free old arrays... */
	MPIU_Free(sock_set->pollfds);
	MPIU_Free(sock_set->pollinfos);
    }
    
    sock_set->poll_n_elem = elem + 1;
    sock_set->poll_arr_sz += SOCK_SET_DEFAULT_SIZE;
    sock_set->pollfds = fds;
    sock_set->pollinfos = infos;
	
    /* Initialize new sock structure and associated poll structures */
    sock_set->pollfds[elem].fd = -1;
    sock_set->pollfds[elem].events = 0;
    sock_set->pollfds[elem].revents = 0;
    sock_set->pollinfos[elem].sock = sock;
    sock->sock_set = sock_set;
    sock->fd = -1;
    sock->pollfd = &sock_set->pollfds[elem];
    sock->pollinfo = &sock_set->pollinfos[elem];
    
    /* Initialize new unallocated elements */
    for (elem = elem + 1; elem < sock_set->poll_arr_sz; elem++)
    {
        fds[elem].fd = -1;
	fds[elem].events = 0;
	fds[elem].revents = 0;
	infos[elem].sock = NULL;
    }

    *sockp = sock;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_SOCK_ALLOC);
    return SOCK_SUCCESS;

  fail_free_fds:
    MPIU_Free(fds);
  fail_free_sock:
    MPIU_Free(sock);
  fail:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_SOCK_ALLOC);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME socki_sock_free
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_sock_free(struct sock * sock)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_SOCK_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_SOCK_FREE);
    
    /* TODO: compress poll array */
    
    sock->pollfd->fd = -1;
    sock->pollfd->events = 0;
    sock->pollfd->revents = 0;
    sock->pollinfo->sock = NULL;
    
    MPIU_Free(sock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_SOCK_FREE);
}

#undef FUNCNAME
#define FUNCNAME socki_event_enqueue
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_event_enqueue(struct sock_set * sock_set, sock_op_t op, sock_size_t num_bytes, void * user_ptr, int error)
{
    struct sock_eventq_elem * eventq_elem;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    eventq_elem = MPIU_Malloc(sizeof(struct sock_eventq_elem)); /* FIXME: use preallocated elements in sock structure */
    if (eventq_elem == NULL)
    {
	int mpi_errno;
	
	mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, FCNAME "() unable to allocate an event queue element");
	MPID_Abort(NULL, mpi_errno);
    }
    eventq_elem->event.op_type = op;
    eventq_elem->event.num_bytes = num_bytes;
    eventq_elem->event.user_ptr = user_ptr;
    eventq_elem->event.error = error;
    eventq_elem->next = NULL;

    /* MT: eventq is not thread safe */
    if (sock_set->eventq_head == NULL)
    { 
	sock_set->eventq_head = eventq_elem;
    }
    else
    {
	sock_set->eventq_tail->next = eventq_elem;
    }
    sock_set->eventq_tail = eventq_elem;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_ENQUEUE);
}

#undef FUNCNAME
#define FUNCNAME socki_event_dequeue
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static int socki_event_dequeue(struct sock_set * sock_set, sock_event_t * eventp)
{
    struct sock_eventq_elem * eventq_elem;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    /* MT: eventq is not thread safe */
    eventq_elem = sock_set->eventq_head;
    if (eventq_elem != NULL)
    {
	sock_set->eventq_head = eventq_elem->next;
	if (eventq_elem->next == NULL)
	{
	    sock_set->eventq_tail = NULL;
	}
	*eventp = eventq_elem->event;
	MPIU_Free(eventq_elem); /* FIXME: return preallocated elements to sock structure */
	
    }
    else
    {
	sock_errno = SOCK_FAIL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_DEQUEUE);
    return sock_errno;
}


/*
 * socki_adjust_iov()
 *
 * Use the specified number of bytes (nb) to adjust the iovec and associated values.  If the iovec has been consumed, return
 * true; otherwise return false.
 */
#undef FUNCNAME
#define FUNCNAME socki_adjust_iov
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static int socki_adjust_iov(MPIDI_msg_sz_t nb, SOCK_IOV * const iov, const int count, int * const offsetp)
{
    int offset = *offsetp;
    
    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (char *) iov[offset].MPID_IOV_BUF + nb;
	    iov[offset].MPID_IOV_LEN -= nb;
	    *offsetp = offset;
	    return FALSE;
	}
    }
    
    *offsetp = offset;
    return TRUE;
}

static int socki_errno_to_sock_errno(int unix_errno)
{
    int sock_errno;
    
    if (unix_errno == EBADF)
    {
	sock_errno = SOCK_ERR_BAD_SOCK;
    }
    else if (unix_errno == EFAULT)
    {
	sock_errno = SOCK_ERR_BAD_BUFFER;
    }
    else if (unix_errno == ECONNRESET || unix_errno == EPIPE)
    {
	sock_errno = SOCK_ERR_CONN_FAILED;
    }
    else if (unix_errno == ENOMEM)
    {
	sock_errno = SOCK_ERR_NOMEM;
    }
    else
    {
	sock_errno = SOCK_FAIL;
    }

    return sock_errno;
}
