/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "mpidi_ch3i_shm_conf.h"
#include "mpid_locksconf.h"

/*#define MPICH_DBG_OUTPUT*/

#define SHM_SUCCESS    0
#define SHM_FAIL      -1

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int size;
    struct MPIDI_VC * vc_table;
    int nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
    int nShmRndvLimit;
#endif
    void *addr;
    int rank;
#ifdef HAVE_SHMGET
    int key;
    int id;
#elif defined (HAVE_MAPVIEWOFFILE)
    char key[MAX_PATH];
    HANDLE id;
#else
#error *** No shared memory mapping variables specified ***
#endif
    int nShmWaitSpinCount;
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

/* This structure requires the iovec structure macros to be defined */
typedef struct MPIDI_CH3I_SHM_Buffer_t
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iovlen;
    int index;
    int total;
} MPIDI_CH3I_SHM_Buffer_t;

typedef struct MPIDI_CH3I_VC
{
    MPIDI_CH3I_Process_group_t * pg;
    int pg_rank;
    struct MPIDI_CH3I_SHM_Queue_t * shm;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    MPIDI_CH3I_VC_state_t state;
    MPIDI_CH3I_SHM_Buffer_t read;
    MPIDI_CH3I_SHM_Buffer_t write;
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    HANDLE hSharedProcessHandle;
#else
    int nSharedProcessID;
    int nSharedProcessFileDescriptor;
#endif
#endif
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC shm;

/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 *
 * MPIDI_CH3I_CA_HANDLE_PKT - The completion of a packet request (send or
 * receive) needs to be handled.
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_SHM,


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
} shm;

#define MPID_STATE_LIST_CH3 \
MPID_STATE_MPIDI_COMM_SPAWN, \
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
MPID_STATE_MPIDI_CH3_CANCEL_SEND, \
MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV, \
MPID_STATE_POST_PKT_RECV, \
MPID_STATE_POST_PKT_SEND, \
MPID_STATE_POST_QUEUED_SEND, \
MPID_STATE_UPDATE_REQUEST, \
MPID_STATE_MPIDI_CH3U_BUFFER_COPY, \
MPID_STATE_HANDLE_ERROR, \
MPID_STATE_MPIDI_CH3I_SHM_POST_READ, \
MPID_STATE_MPIDI_CH3I_SHM_POST_READV, \
MPID_STATE_MPIDI_CH3I_SHM_WRITE, \
MPID_STATE_MPIDI_CH3I_SHM_WRITEV, \
MPID_STATE_MPIDI_CH3I_SHM_READ, \
MPID_STATE_MPIDI_CH3I_SHM_READV, \
MPID_STATE_MPIDU_COMPARE_SWAP, \
MPID_STATE_MPIDU_PROCESS_LOCK_INIT, \
MPID_STATE_MPIDU_PROCESS_LOCK_FREE, \
MPID_STATE_MPIDU_PROCESS_LOCK, \
MPID_STATE_MPIDU_PROCESS_UNLOCK, \
MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT, \
MPID_STATE_MPIDI_CH3I_SHM_ALLOC, \
MPID_STATE_MPIDI_CH3I_SHM_FREE, \
MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC, \
MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM,

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
