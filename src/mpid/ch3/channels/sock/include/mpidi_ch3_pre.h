/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "sock.h"

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int size;
    struct MPIDI_VC * vc_table;
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SOCK_OPEN_REQ,			\
MPIDI_CH3I_PKT_SOCK_OPEN_RESP,			\
MPIDI_CH3I_PKT_SOCK_CLOSE

#define MPIDI_CH3_PKT_DEFS													  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
    /* FIXME - We need a little security here to avoid having a random port scan crash the process.  Perhaps a "secret" value for \
       each process could be published in the key-val space and subsequently sent in the open pkt. */				  \
																  \
    /* FIXME - We need some notion of a global process group ID so that we can tell the remote process which process is		  \
       connecting to it */													  \
    int pg_id;															  \
    int pg_rank;														  \
}																  \
MPIDI_CH3I_Pkt_sock_open_req_t;													  \
																  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
    int ack;															  \
}																  \
MPIDI_CH3I_Pkt_sock_open_resp_t;												  \
																  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
}																  \
MPIDI_CH3I_Pkt_sock_close_t;

#define MPIDI_CH3_PKT_DECL			\
MPIDI_CH3I_Pkt_sock_open_req_t sock_open_req;	\
MPIDI_CH3I_Pkt_sock_open_resp_t sock_open_resp;	\
MPIDI_CH3I_Pkt_sock_close_t sock_close;

typedef enum
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

#define MPIDI_CH3_VC_DECL			\
struct MPIDI_CH3I_VC				\
{						\
    MPIDI_CH3I_Process_group_t * pg;		\
    int pg_rank;				\
    struct MPID_Request * sendq_head;		\
    struct MPID_Request * sendq_tail;		\
    MPIDI_CH3I_VC_state_t state;		\
    sock_t sock;                                \
} channel;


/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 *
 * MPIDI_CH3I_CA_HANDLE_PKT - The completion of a packet request (send or
 * receive) needs to be handled.
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_CHANNEL,


/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    /* iov_offset points to the current head element in the IOV */	\
    int iov_offset;							\
    									\
    /*  pkt is used to temporarily store a packet header associated	\
       with this request */						\
    MPIDI_CH3_Pkt_t pkt;						\
} channel;

#define MPID_STATE_LIST_CH3 \
MPID_STATE_MAKE_PROGRESS, \
MPID_STATE_HANDLE_POLLIN, \
MPID_STATE_HANDLE_POLLOUT, \
MPID_STATE_MPIDI_CH3_IREAD, \
MPID_STATE_MPIDI_CH3_ISEND, \
MPID_STATE_MPIDI_CH3_ISENDV, \
MPID_STATE_MPIDI_CH3_ISTARTMSG, \
MPID_STATE_MPIDI_CH3_ISTARTMSGV, \
MPID_STATE_MPIDI_CH3_IWRITE, \
MPID_STATE_MPIDI_CH3_PROGRESS, \
MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3_PROGRESS_POKE, \
MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_CREATE, \
MPID_STATE_MPIDI_CH3_REQUEST_DESTROY, \
MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF, \
MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT, \
MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV, \
MPID_STATE_MPIDI_CH3I_POST_CONNECT, \
MPID_STATE_MPIDI_CH3I_POST_READ, \
MPID_STATE_MPIDI_CH3I_POST_WRITE, \
MPID_STATE_POST_PKT_RECV, \
MPID_STATE_POST_PKT_SEND, \
MPID_STATE_POST_QUEUED_SEND, \
MPID_STATE_UPDATE_REQUEST, \
MPID_STATE_POLL, \
MPID_STATE_READ, \
MPID_STATE_READV, \
MPID_STATE_WRITE, \
MPID_STATE_WRITEV, \
MPID_STATE_MPIDI_CH3U_BUFFER_COPY, \
MPID_STATE_MPIDI_CH3_COMM_SPAWN,

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
