/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME: This header should contain only the definitions exported to the
   mpiimpl.h level */

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#define MPICH_MPIDPRE_H_INCLUDED

#include "mpidi_ch3_conf.h"

/* Tell the compiler that we're going to declare struct MPID_Request later */
struct MPID_Request;

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

typedef MPI_Aint MPIDI_msg_sz_t;

#include "mpid_dataloop.h"
#if 0
struct MPID_Datatype; 
#endif

/* Include definitions from the channel which must exist before items in this 
   file (mpidpre.h) or the file it includes (mpiimpl.h) can be defined. */
#include "mpidi_ch3_pre.h"

/* FIXME: Who defines this name */
/* As of 8/1/06, no-one defined MSGS_UNORDERED.  We should consider 
   moving support for unordered messages to a different part of the code
   However, note that sequence numbers may be useful in other contexts, 
   including identifying messages when multithreaded (for better profiling
   tools) and handling cancellations (rather than relying on unique 
   request ids) 
*/
#if defined (MPIDI_CH3_MSGS_UNORDERED)
#define MPID_USE_SEQUENCE_NUMBERS
#endif

#if defined(MPID_USE_SEQUENCE_NUMBERS)
typedef unsigned long MPID_Seqnum_t;
#endif

#include "mpichconf.h"

typedef struct MPIDI_Message_match
{
    int32_t tag;
    int16_t rank;
    int16_t context_id;
}
MPIDI_Message_match;
#define MPIDI_TAG_UB (0x7fffffff)

/* Packet types are defined in mpidpkt.h .  The intent is to remove the
   need for packet definitions outside of the device directories.
   Currently, the request contains a block of storage in which a 
   packet header can be copied in the event that a message cannot be
   send immediately.  
*/
typedef struct MPIDI_CH3_PktGeneric { int32_t kind; int32_t *pktptrs[1]; int32_t pktwords[6]; } 
    MPIDI_CH3_PktGeneric_t;

/*
 * THIS IS OBSOLETE AND UNUSED, BUT RETAINED FOR ITS DESCRIPTIONS OF THE
 * VARIOUS STATES.  Note that this is not entirely accurate, as the 
 * CA_COMPLETE state could depend on the packet type (e.g., for RMA 
 * operations).
 *
 * MPIDI_CA_t
 *
 * An enumeration of the actions to perform when the requested I/O operation 
 * has completed.
 *
 * MPIDI_CH3_CA_COMPLETE - The last operation for this request has completed.
 * The completion counter should be decremented.  If
 * it has reached zero, then the request should be released by calling 
 * MPID_Request_release().
 *
 * MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE - This is a special case of the 
 * MPIDI_CH3_CA_COMPLETE.  The data for an unexpected
 * eager messaage has been stored into a temporary buffer and needs to be 
 * copied/unpacked into the user buffer before the
 * completion counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE - This is a special case of the 
 * MPIDI_CH3_CA_COMPLETE.  The data from the completing
 * read has been stored into a temporary send/receive buffer and needs to be 
 * copied/unpacked into the user buffer before the
 * completion counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_RELOAD_IOV - This request contains more segments of data than 
 * the IOV or buffer space allow.  Since the
 * previously request operation has completed, the IOV in the request should 
 * be reload at this time.
 *
 * MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV - This is a special case of the 
 * MPIDI_CH3_CA_RELOAD_IOV.  The data from the
 * completing read operation has been stored into a temporary send/receive 
 * buffer and needs to be copied/unpacked into the user
 * buffer before the IOV is reloaded.
 *
 * MPIDI_CH3_CA_END_CH3 - This not a real action, but rather a marker.  
 * All actions numerically less than MPID_CA_END are defined
 * by channel device.  Any actions numerically greater than MPIDI_CA_END are 
 * internal to the channel instance and must be handled
 * by the channel instance.
 */

#ifndef HAVE_MPIDI_VCRT
#define HAVE_MPIDI_VCRT
typedef struct MPIDI_VCRT * MPID_VCRT;
typedef struct MPIDI_VC * MPID_VCR;
#endif

#ifndef DEFINED_REQ
#define DEFINED_REQ
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_REQUEST_SEQNUM	\
        MPID_Seqnum_t seqnum;
#else
#   define MPIDI_REQUEST_SEQNUM
#endif

#define MPIDI_DEV_WIN_DECL                                               \
    volatile int my_counter;  /* completion counter for operations       \
                                 targeting this window */                \
    void **base_addrs;     /* array of base addresses of the windows of  \
                              all processes */                           \
    int *disp_units;      /* array of displacement units of all windows */\
    MPI_Win *all_win_handles;    /* array of handles to the window objects\
                                          of all processes */            \
    struct MPIDI_RMA_ops *rma_ops_list; /* list of outstanding RMA requests */  \
    volatile int lock_granted;  /* flag to indicate whether lock has     \
                                   been granted to this process (as source) for         \
                                   passive target rma */                 \
    volatile int current_lock_type;   /* current lock type on this window (as target)   \
                              * (none, shared, exclusive) */             \
    volatile int shared_lock_ref_cnt;                                    \
    struct MPIDI_Win_lock_queue volatile *lock_queue;  /* list of unsatisfied locks */  \
                                                                         \
    int *pt_rma_puts_accs;  /* array containing the no. of passive target\
                               puts/accums issued from this process to other \
                               processes. */                             \
    volatile int my_pt_rma_puts_accs;  /* no. of passive target puts/accums  \
                                          that this process has          \
                                          completed as target */
 
#ifdef MPIDI_CH3_WIN_DECL
#define MPID_DEV_WIN_DECL \
MPIDI_DEV_WIN_DECL \
MPIDI_CH3_WIN_DECL
#else
#define MPID_DEV_WIN_DECL \
MPIDI_DEV_WIN_DECL
#endif


typedef struct MPIDI_Request {
    MPIDI_Message_match match;

    /* user_buf, user_count, and datatype needed to process 
       rendezvous messages. */
    void        *user_buf;
    int          user_count;
    MPI_Datatype datatype;

    /* segment, segment_first, and segment_size are used when processing 
       non-contiguous datatypes */
    /*    MPID_Segment   segment; */
    struct MPID_Segment *segment_ptr;
    MPIDI_msg_sz_t segment_first;
    MPIDI_msg_sz_t segment_size;

    /* Pointer to datatype for reference counting purposes */
    struct MPID_Datatype * datatype_ptr;

    /* iov and iov_count define the data to be transferred/received.  
       iov_offset points to the current head eleemnt in the IOV */
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iov_count;
    int iov_offset;

#if 0
    /* FIXME: RDMA values are specific to some channels? */
    /* FIXME: Remove these (obsolete)? */
    MPID_IOV rdma_iov[MPID_IOV_LIMIT];
    int rdma_iov_count;
    int rdma_iov_offset;
    MPI_Request rdma_request;
#endif
    /* OnDataAvail is the action to take when data is now available.
       For example, when an operation described by an iov has 
       completed.  This replaces the MPIDI_CA_t (completion action)
       field used through MPICH2 1.0.4. */
    int (*OnDataAvail)( struct MPIDI_VC *, struct MPID_Request *, int * );
    /* OnFinal is used in the following case:
       OnDataAvail is set to a function, and that function has processed
       all of the data.  At that point, the OnDataAvail function can
       reset OnDataAvail to OnFinal.  This is normally used when processing
       non-contiguous data, where there is one more action to take (such
       as a get-response) when processing of the non-contiguous data 
       completes. This value need not be initialized unless OnDataAvail
       is set to a non-null value (and then only in certain cases) */
    int (*OnFinal)( struct MPIDI_VC *, struct MPID_Request *, int * );

    /* tmpbuf and tmpbuf_sz describe temporary storage used for things like 
       unexpected eager messages and packing/unpacking
       buffers.  tmpuf_off is the current offset into the temporary buffer. */
    void          *tmpbuf;
    int            tmpbuf_off;
    MPIDI_msg_sz_t tmpbuf_sz;

    MPIDI_msg_sz_t recv_data_sz;
    MPI_Request    sender_req_id;

    unsigned int   state;
    int            cancel_pending;
    int            recv_pending_count;

    /* The next 8 are for RMA */
    MPI_Op op;
    /* For accumulate, since data is first read into a tmp_buf */
    void *real_user_buf;
    /* For derived datatypes at target */
    struct MPIDI_RMA_dtype_info *dtype_info;
    void *dataloop;
    /* req. handle needed to implement derived datatype gets  */
    MPI_Request request_handle;
    MPI_Win     target_win_handle;
    MPI_Win     source_win_handle;
    int single_op_opt;   /* to indicate a lock-put-unlock optimization case */
    struct MPIDI_Win_lock_queue *lock_queue_entry; /* for single lock-put-unlock optimization */

    MPIDI_REQUEST_SEQNUM

    /* Occasionally, when a message cannot be sent, we need to cache the
       data that is required.  The fields above (such as userbuf and tmpbuf)
       are used for the message data.  However, we also need space for the
       message packet. This field provide a generic location for that.
       Question: do we want to make this a link instead of reserving 
       a fixed spot in the request? */
    MPIDI_CH3_PktGeneric_t pending_pkt;
    struct MPID_Request * next;
} MPIDI_Request;
#define MPID_REQUEST_DECL MPIDI_Request dev;

#if defined(MPIDI_CH3_REQUEST_DECL)
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL				\
MPIDI_CH3_REQUEST_DECL
#else
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL
#endif

#ifdef MPIDI_CH3_REQUEST_KIND_DECL
#define MPID_DEV_REQUEST_KIND_DECL MPIDI_CH3_REQUEST_KIND_DECL
#endif

#endif

/* FIXME: This ifndef test is a temp until mpidpre is cleaned of
   all items that do not belong (e.g., all items not needed by the
   top layers of MPICH2) */
/* FIXME: The progress routines will be made into ch3-common definitions, not
   channel specific.  Channels that need more will need to piggy back or 
   otherwise override */
#ifndef MPID_PROGRESS_STATE_DECL
#if defined(MPIDI_CH3_PROGRESS_STATE_DECL)
#   define MPID_PROGRESS_STATE_DECL MPIDI_CH3_PROGRESS_STATE_DECL
#else
#   define MPID_PROGRESS_STATE_DECL int foo;
#endif
#endif


/* Tell Intercomm create and friends that the GPID routines have been
   implemented */
#define HAVE_GPID_ROUTINES

/* Tell initthread to prepare a private comm_world */
#define MPID_NEEDS_ICOMM_WORLD

#endif /* !defined(MPICH_MPIDPRE_H_INCLUDED) */
