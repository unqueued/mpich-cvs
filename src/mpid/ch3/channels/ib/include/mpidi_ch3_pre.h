/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

/*#define MPICH_DBG_OUTPUT*/

#include "ibu.h"
#include "blockallocator.h"

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int size;
    struct MPIDI_VC * vc_table;
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PKT_ENUM
#define MPIDI_CH3_PKT_DEFS
#define MPIDI_CH3_PKT_DECL

typedef enum
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
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
    ibu_t ibu;                                  \
    struct MPID_Request * send_active;          \
    struct MPID_Request * recv_active;          \
    struct MPID_Request * req;                  \
    MPIDI_CH3I_VC_state_t state;		\
} ib;


/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 *
 * MPIDI_CH3I_CA_HANDLE_PKT - The completion of a packet request (send or
 * receive) needs to be handled.
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_IB,


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
} ib;

#define MPID_STATE_LIST_CH3 \
MPID_STATE_IBU_CREATE_QP, \
MPID_STATE_IBU_INIT, \
MPID_STATE_IBU_FINALIZE, \
MPID_STATE_IBU_CREATE_SET, \
MPID_STATE_IBU_DESTROY_SET, \
MPID_STATE_IBU_WAIT, \
MPID_STATE_IBU_SET_USER_PTR, \
MPID_STATE_IBU_POST_READ, \
MPID_STATE_IBU_POST_READV, \
MPID_STATE_IBU_POST_WRITE, \
MPID_STATE_IBU_POST_WRITEV, \
MPID_STATE_MAKE_PROGRESS, \
MPID_STATE_HANDLE_READ, \
MPID_STATE_HANDLE_WRITTEN, \
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
MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS, \
MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV, \
MPID_STATE_MPIDI_CH3I_IB_POST_READ, \
MPID_STATE_MPIDI_CH3I_IB_POST_WRITE, \
MPID_STATE_POST_PKT_RECV, \
MPID_STATE_POST_PKT_SEND, \
MPID_STATE_POST_QUEUED_SEND, \
MPID_STATE_UPDATE_REQUEST, \
MPID_STATE_MPIDI_CH3U_BUFFER_COPY, \
MPID_STATE_MPIDI_CH3_COMM_SPAWN, \
MPID_STATE_HANDLE_ERROR,
#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
