/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define _XOPEN_SOURCE

#include "mpidu_sock.h"
#include "mpiimpl.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE__SW_INCLUDE_SYS_POLL_H
#include "/sw/include/sys/poll.h"
#endif
#include <netdb.h>
#include <errno.h>

/*
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
*/

#ifdef USE_SELECT_FOR_POLL
#include "mpidu_sock_poll.h"
#endif

#if !defined(MPIDU_SOCK_SET_DEFAULT_SIZE)
#define MPIDU_SOCK_SET_DEFAULT_SIZE 1
#endif

#if !defined(MPIDU_SOCK_EVENTQ_POOL_SIZE)
#define MPIDU_SOCK_EVENTQ_POOL_SIZE 1
#endif


enum MPIDU_Socki_state
{
    MPIDU_SOCKI_STATE_FIRST = 0,
    MPIDU_SOCKI_STATE_CONNECTING,
    MPIDU_SOCKI_STATE_CONNECTED_RW,
    MPIDU_SOCKI_STATE_CONNECTED_RO,
    MPIDU_SOCKI_STATE_DISCONNECTED,
    MPIDU_SOCKI_STATE_CLOSING,
    MPIDU_SOCKI_STATE_LAST
};

enum MPIDU_Socki_type
{
    MPIDU_SOCKI_TYPE_FIRST = 0,
    MPIDU_SOCKI_TYPE_COMMUNICATION,
    MPIDU_SOCKI_TYPE_LISTENER,
    MPIDU_SOCKI_TYPE_INTERRUPTER,
    MPIDU_SOCKI_TYPE_LAST
};

/*
 * struct pollinfo
 * 
 * sock_id - an integer id comprised of the sock_set id and the element number in the pollfd/info arrays
 * 
 * sock_set - a pointer to the sock set to which this connection belongs
 * 
 * elem - the element number of this connection in the pollfd/info arrays
 * 
 * sock - at present this is only used to free the sock structure when the close is completed by MPIDIU_Sock_wait()
 * 
 * fd - this file descriptor is used whenever the file descriptor is needed.  this descriptor remains open until the sock, and
 * thus the socket, are actually closed.  the fd in the pollfd structure should only be used for telling poll() if it should
 * check for events on that descriptor.
 * 
 * user_ptr - a user supplied pointer that is included with event associated with this connection
 * 
 * state - state of the connection
 *
 */
struct pollinfo
{
    int sock_id;
    struct MPIDU_Sock_set * sock_set;
    int elem;
    struct MPIDU_Sock * sock;
    int fd;
    void * user_ptr;
    enum MPIDU_Socki_type type;
    enum MPIDU_Socki_state state;
    int os_errno;
    union
    {
	struct
	{
	    MPID_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    MPIU_Size_t min;
	    MPIU_Size_t max;
	} buf;
    } read;
    int read_iov_flag;
    MPIU_Size_t read_nb;
    MPIDU_Sock_progress_update_func_t read_progress_update_fn;
    union
    {
	struct
	{
	    MPID_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    MPIU_Size_t min;
	    MPIU_Size_t max;
	} buf;
    } write;
    int write_iov_flag;
    MPIU_Size_t write_nb;
    MPIDU_Sock_progress_update_func_t write_progress_update_fn;
};

struct MPIDU_Socki_eventq_elem
{
    struct MPIDU_Sock_event event;
    int set_elem;
    struct MPIDU_Socki_eventq_elem * next;
};

struct MPIDU_Sock_set
{
    int id;
    int poll_arr_sz;
    int poll_n_elem;
    int starting_elem;
    struct pollfd * pollfds;
    struct pollinfo * pollinfos;
    int intr_fds[2];
    struct MPIDU_Sock * intr_sock;
    struct MPIDU_Socki_eventq_elem * eventq_head;
    struct MPIDU_Socki_eventq_elem * eventq_tail;
};

struct MPIDU_Sock
{
    struct MPIDU_Sock_set * sock_set;
    int elem;
};


int MPIDU_Socki_initialized = 0;

static struct MPIDU_Socki_eventq_elem * MPIDU_Socki_eventq_pool = NULL;

/* MT: needs to be atomically incremented */
static int MPIDU_Socki_set_next_id = 0;


#include "socki_util.i"

#include "sock_init.i"
#include "sock_set.i"
#include "sock_post.i"
#include "sock_immed.i"
#include "sock_misc.i"
#include "sock_wait.i"
