/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPIG_CM_XIO_H_INCLUDED)
#define MPIG_CM_XIO_H_INCLUDED

#include "globus_xio.h"

/*
 * user tunable parameters
 */
#if !defined(MPIG_CM_XIO_IOV_NUM_ENTRIES)
#define MPIG_CM_XIO_IOV_NUM_ENTRIES 32
#endif

#if !defined(MPIG_CM_XIO_REQUEST_MSGBUF_SIZE)
#define MPIG_CM_XIO_REQUEST_MSGBUF_SIZE 256
#endif

#if !defined(MPIG_CM_XIO_VC_MSGBUF_SIZE)
#define MPIG_CM_XIO_VC_MSGBUF_SIZE 256
#endif


/*
 * expose the communication module's vtable so that it is accessible to other modules in the device
 */
extern const mpig_cm_vtable_t mpig_cm_xio_vtable;


/*									\
 * define the communication module structure to be included in a VC	\
 */									\
#define MPIG_VC_CM_XIO_DECL						\
struct mpig_cm_xio_vc							\
{									\
    /* state of the connection */					\
    mpig_cm_xio_vc_state_t state;					\
									\
    /* contact string for remote process listener */			\
    char * cs;								\
									\
    /* data format of remote machine */					\
    int df;								\
    mpig_endian_t endian;						\
									\
    /* handle to the XIO connection */					\
    globus_xio_handle_t handle;						\
									\
    /* connection sequence number */					\
    unsigned conn_seqnum;						\
									\
    /* active send and receive requests */				\
    struct MPID_Request * active_sreq;					\
    struct MPID_Request * active_rreq;					\
									\
    /* send queue */							\
    struct MPID_Request * sendq_head;					\
    struct MPID_Request * sendq_tail;					\
									\
    /* header size of message being receive */				\
    unsigned msg_hdr_size;						\
									\
    /* internal buffer for headers and small messages */		\
    MPIG_DATABUF_DECL(msgbuf, MPIG_CM_XIO_VC_MSGBUF_SIZE);		\
									\
    /* VC list pointers */						\
    struct mpig_vc * list_prev;						\
    struct mpig_vc * list_next;						\
}									\
xio;


/*
 * define the communication module structure to be included in a request
 */
#define MPIG_REQUEST_CM_XIO_DECL												\
struct mpig_cm_xio_request													\
{																\
    /* state of the message being sent or received */										\
    mpig_cm_xio_req_state_t state;												\
																\
    /* type of message being sent or received */										\
    mpig_cm_xio_msg_type_t msg_type;												\
																\
    /* internal completion counter.  since the main completion counter is accessed outside of a lock, we must use a separate	\
       counter within the xio communication module */										\
    int cc;															\
																\
    /* information about the user buffer being sent/received.  the segment object is used to help process buffers that are	\
       noncontiguous or data that is heterogeneous. */										\
    MPIU_Size_t buf_size;													\
    mpig_cm_xio_userbuf_type_t buf_type;											\
    MPI_Aint buf_true_lb;													\
    MPIU_Size_t stream_size;													\
    MPIU_Size_t stream_pos;													\
    MPIU_Size_t stream_max_pos;													\
    MPID_Segment seg;														\
																\
    /* the I/O vector defines the data to be transferred/received */								\
    MPIG_IOV_DECL(iov, MPIG_CM_XIO_IOV_NUM_ENTRIES);										\
																\
    /* callback function to be given to globus_xio_register_*() when request is processed */					\
    globus_xio_iovec_callback_t gcb;												\
																\
    /* request type associated with incoming message */										\
    mpig_request_type_t sreq_type;												\
																\
    /* space for a message header, and perhaps a small amount of data */							\
    MPIG_DATABUF_DECL(msgbuf, MPIG_CM_XIO_REQUEST_MSGBUF_SIZE);									\
																\
    /* format of the message data in databuf */											\
    int df;															\
																\
    /* temporary data storage used for things like unexpected eager messages and packing/unpacking buffers */			\
    struct mpig_databuf * databuf;												\
																\
    /* next entry in the send queue.  the request completion queue (RCQ) already uses the dev.next field, so a separate send	\
       queue next pointer field is required. */											\
    struct MPID_Request * sendq_next;												\
}																\
xio;


/*
 * VC state
 */
#define MPIG_CM_XIO_VC_STATE_CLASS_SHIFT 8
#define MPIG_CM_XIO_VC_STATE_CLASS_MASK (~0U << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT)

typedef enum mpig_cm_xio_vc_state_class
{
    MPIG_CM_XIO_VC_STATE_CLASS_FIRST = 0,
    MPIG_CM_XIO_VC_STATE_CLASS_UNDEFINED,
    MPIG_CM_XIO_VC_STATE_CLASS_UNCONNECTED,
    MPIG_CM_XIO_VC_STATE_CLASS_CONNECTING,
    MPIG_CM_XIO_VC_STATE_CLASS_CONNECTED,
    MPIG_CM_XIO_VC_STATE_CLASS_DISCONNECTING,
    MPIG_CM_XIO_VC_STATE_CLASS_CLOSING,
    MPIG_CM_XIO_VC_STATE_CLASS_FAILED,
    MPIG_CM_XIO_VC_STATE_CLASS_LAST
}
mpig_cm_xio_vc_state_class_t;

typedef enum mpig_cm_xio_vc_state
{
    MPIG_CM_XIO_VC_STATE_UNDEFINED = (MPIG_CM_XIO_VC_STATE_CLASS_UNDEFINED << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    
    MPIG_CM_XIO_VC_STATE_UNCONNECTED_FIRST = (MPIG_CM_XIO_VC_STATE_CLASS_UNCONNECTED << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    MPIG_CM_XIO_VC_STATE_UNCONNECTED,
    MPIG_CM_XIO_VC_STATE_UNCONNECTED_LAST,
    
    MPIG_CM_XIO_VC_STATE_CONNECTING_FIRST = (MPIG_CM_XIO_VC_STATE_CLASS_CONNECTING << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    MPIG_CM_XIO_VC_STATE_CLIENT_CONNECTING,
    MPIG_CM_XIO_VC_STATE_SERVER_ACCEPTING,
    /* states for client temporary VC */
    MPIG_CM_XIO_VC_STATE_CLIENT_OPENING_VC,
    MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_VC_MAGIC,
    MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_VC_MAGIC,
    MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_OPEN_VC_REQ,
    MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_OPEN_VC_RESP,
    MPIG_CM_XIO_VC_STATE_CLIENT_OPENING_PORT,
    MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_PORT_MAGIC,
    MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_PORT_MAGIC,
    MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_OPEN_PORT_REQ,
    MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_OPEN_PORT_RESP,
    /* states for server temporary VC */
    MPIG_CM_XIO_VC_STATE_SERVER_OPEN,
    MPIG_CM_XIO_VC_STATE_SERVER_RECEIVING_MAGIC,
    MPIG_CM_XIO_VC_STATE_SERVER_SENDING_MAGIC,
    MPIG_CM_XIO_VC_STATE_SERVER_RECEIVING_OPEN_REQ,
    MPIG_CM_XIO_VC_STATE_SERVER_SENDING_OPEN_RESP,
    MPIG_CM_XIO_VC_STATE_CONNECTING_LAST,
    
    MPIG_CM_XIO_VC_STATE_CONNECTED_FIRST = (MPIG_CM_XIO_VC_STATE_CLASS_CONNECTED << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    MPIG_CM_XIO_VC_STATE_CONNECTED,
    MPIG_CM_XIO_VC_STATE_CONNECTED_LAST,
    
    MPIG_CM_XIO_VC_STATE_DISCONNECTING_FIRST = (MPIG_CM_XIO_VC_STATE_CLASS_DISCONNECTING << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    MPIG_CM_XIO_VC_STATE_DISCONNECT_RECEIVED_CLOSE_REQ,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_AWAITING_CLOSE_REQ,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_CLOSE_REQ,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_ACK,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENT_CLOSE_REQ_AWAITING_CLOSE_REQ,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_RECEIVED_ACK,
    MPIG_CM_XIO_VC_STATE_DISCONNECTING_LAST,
    
    MPIG_CM_XIO_VC_STATE_CLOSING_FIRST = (MPIG_CM_XIO_VC_STATE_CLASS_CLOSING << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    MPIG_CM_XIO_VC_STATE_DISCONNECT_SENT_ACK_AWAITING_ACK,
    MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC,
    MPIG_CM_XIO_VC_STATE_CLOSING_LAST,
    
    MPIG_CM_XIO_VC_STATE_FAILED_FIRST = (MPIG_CM_XIO_VC_STATE_CLASS_FAILED << MPIG_CM_XIO_VC_STATE_CLASS_SHIFT),
    MPIG_CM_XIO_VC_STATE_FAILED_CONNECTING,
    MPIG_CM_XIO_VC_STATE_FAILED_CONNECTION,
    MPIG_CM_XIO_VC_STATE_FAILED_DISCONNECTING,
    MPIG_CM_XIO_VC_STATE_FAILED_LAST
}
mpig_cm_xio_vc_state_t;


/*
 * types of messages that can be sent or received
 */
typedef enum mpig_cm_xio_msg_type
{
    MPIG_CM_XIO_MSG_TYPE_FIRST = 0,
    MPIG_CM_XIO_MSG_TYPE_UNDEFINED,
    MPIG_CM_XIO_MSG_TYPE_EAGER_DATA,
    MPIG_CM_XIO_MSG_TYPE_RNDV_RTS,
    MPIG_CM_XIO_MSG_TYPE_RNDV_CTS,
    MPIG_CM_XIO_MSG_TYPE_RNDV_DATA,
    MPIG_CM_XIO_MSG_TYPE_SSEND_ACK,
    MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND,
    MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP,
    MPIG_CM_XIO_MSG_TYPE_OPEN_VC_REQ,
    MPIG_CM_XIO_MSG_TYPE_OPEN_VC_RESP,
    MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ,
    MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_RESP,
    MPIG_CM_XIO_MSG_TYPE_OPEN_ERROR_RESP,
    MPIG_CM_XIO_MSG_TYPE_CLOSE,
    MPIG_CM_XIO_MSG_TYPE_LAST
}
mpig_cm_xio_msg_type_t;


/*
 * request state
 */
typedef enum mpig_cm_xio_req_state
{
    MPIG_CM_XIO_REQ_STATE_FIRST = 0,
    MPIG_CM_XIO_REQ_STATE_UNDEFINED,
    MPIG_CM_XIO_REQ_STATE_INACTIVE,

    /* send request states */
    MPIG_CM_XIO_REQ_STATE_SEND_DATA,
    MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS,
    MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS_RECVD_CTS,
    MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_CTS,
    MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG,
    MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE,
    
    /* receive request states */
    MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED,
    MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA,
    MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA,
    MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED,
    MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_ERROR,
    MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_SREQ_CANCELLED,
    MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_RSEND_DATA,
    MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA,
    MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE,
    MPIG_CM_XIO_REQ_STATE_LAST
}
mpig_cm_xio_req_state_t;


/*
 * user buffer type
 */
typedef enum mpig_cm_xio_userbuf_type
{
    MPIG_CM_XIO_USERBUF_TYPE_FIRST = 0,
    MPIG_CM_XIO_USERBUF_TYPE_UNDEFINED,
    MPIG_CM_XIO_USERBUF_TYPE_CONTIG,
    MPIG_CM_XIO_USERBUF_TYPE_DENSE,
    MPIG_CM_XIO_USERBUF_TYPE_SPARSE,
    MPIG_CM_XIO_USERBUF_TYPE_LAST
}
mpig_cm_xio_userbuf_type_t;


/*
 * communication module interface function prototypes
 */
void mpig_cm_xio_pe_start(struct MPID_Progress_state * state);

void mpig_cm_xio_pe_end(struct MPID_Progress_state * state);

int mpig_cm_xio_pe_wait(struct MPID_Progress_state * state);

int mpig_cm_xio_pe_test(void);

int mpig_cm_xio_pe_poke(void);


/*
 * macro implementations of CM interface functions
 */
#define mpig_cm_xio_pe_start(state_)	\
{					\
}

#define mpig_cm_xio_pe_end(state_)	\
{					\
}

#define mpig_cm_xio_pe_poke() mpig_cm_xio_progess_test()


/*
 * mpig_connection_t definition needed for MPI_Comm_{connect,accept} functionality
 */
typedef struct mpig_connection
{
    struct mpig_vc * vc;
    int remote_data_size;
    bool_t acceptor;
}
mpig_connection_t;

#endif /* !defined(MPICH2_MPIG_CM_XIO_H_INCLUDED) */
