/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/**********************************************************************************************************************************
						   BEGIN VC OBJECT MANAGEMENT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_vc_destruct_fn(mpig_vc_t * vc);
MPIG_STATIC const char * mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_states_t vc_state);

#define mpig_cm_xio_vc_construct(vc_)						\
{										\
    mpig_cm_xio_vc_set_state((vc_), MPIG_CM_XIO_VC_STATE_UNCONNECTED);		\
    (vc_)->cm.xio.state = MPIG_CM_XIO_VC_STATE_UNCONNECTED;			\
    (vc_)->cm.xio.cs = NULL;							\
    (vc_)->cm.xio.df = -1;							\
    (vc_)->cm.xio.handle = NULL;						\
    (vc_)->cm.xio.active_sreq = NULL;						\
    (vc_)->cm.xio.active_rreq = NULL;						\
    mpig_cm_xio_sendq_construct(vc_);						\
    (vc_)->cm.xio.msg_hdr_size = 0;						\
    mpig_databuf_construct((vc_)->cm.xio.msgbuf, MPIG_CM_XIO_VC_MSGBUF_SIZE);	\
    (vc_)->cm.xio.list_prev = NULL;						\
    (vc_)->cm.xio.list_next = NULL;						\
										\
    mpig_vc_set_cm_type((vc_), MPIG_CM_TYPE_XIO);				\
    mpig_vc_set_cm_funcs((vc_), &mpig_cm_xio_vc_funcs);				\
}

#define mpig_cm_xio_vc_destruct(vc_)					\
{									\
    mpig_cm_xio_vc_set_state((vc_), MPIG_CM_XIO_VC_STATE_UNDEFINED);	\
    MPIU_Free((vc_)->cm.xio.cs);					\
    (vc_)->cm.xio.df = -1;						\
    (vc_)->cm.xio.handle = NULL;					\
    (vc_)->cm.xio.active_sreq = NULL;					\
    (vc_)->cm.xio.active_rreq = NULL;					\
    mpig_cm_xio_sendq_destruct(vc_);					\
    (vc_)->cm.xio.msg_hdr_size = 0;					\
    mpig_databuf_destruct((vc_)->cm.xio.msgbuf);			\
    (vc_)->cm.xio.list_prev = NULL;					\
    (vc_)->cm.xio.list_next = NULL;					\
									\
    mpig_vc_set_cm_type((vc_), MPIG_CM_TYPE_UNDEFINED);			\
    mpig_vc_set_cm_funcs((vc_), NULL);					\
}

#define mpig_cm_xio_vc_inc_ref_count(vc_, was_inuse_flag_p_)					\
{												\
    *(was_inuse_flag_p_) = ((vc_)->ref_count++) ? TRUE : FALSE;					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,				\
		       "VC - XIO increment ref count: vc=" MPIG_HANDLE_FMT ", ref_count=%d",	\
		       (MPIG_PTR_CAST) (vc_), (vc_)->ref_count));				\
}

#define mpig_cm_xio_vc_dec_ref_count(vc_, inuse_flag_p_)					\
{												\
    *(inuse_flag_p_) = (--(vc_)->ref_count) ? TRUE : FALSE;					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,				\
		       "VC - XIO decrement ref count: vc=" MPIG_HANDLE_FMT ", ref_count=%d",	\
		       (MPIG_PTR_CAST) (vc_), (vc_)->ref_count));				\
}

#define mpig_cm_xio_vc_set_state(vc_, state_)								\
{													\
    (vc_)->cm.xio.state = (state_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC,								\
		       "VC - setting XIO state: vc=" MPIG_HANDLE_FMT ", state=%s",			\
		       (MPIG_PTR_CAST) (vc_), mpig_cm_xio_vc_state_get_string((vc_)->cm.xio.state)));	\
}

#define mpig_cm_xio_vc_get_state(vc_) ((vc_)->cm.xio.state)

#define mpig_cm_xio_vc_set_endian(vc_, endian_)										\
{															\
    (vc_)->cm.xio.endian = (endian_);											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC,										\
		       "VC - setting XIO endian: vc=" MPIG_PTR_FMT ", endian=%s",					\
		       (MPIG_PTR_CAST) (vc_), ((vc_)->cm.xio.endian == MPIG_ENDIAN_LITTLE) ? "little" : "big"));	\
}

#define mpig_cm_xio_vc_get_endian(vc_) ((vc_)->cm.xio.endian)

#define mpig_cm_xio_vc_set_data_format(vc_, df_)									\
{															\
    (vc_)->cm.xio.df = (df_);												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC,										\
		       "VC - setting XIO VC data format: vc=" MPIG_PTR_FMT ", df=%d", (MPIG_PTR_CAST) (vc_), (df_)));	\
}

#define mpig_cm_xio_vc_get_data_format(vc_) ((vc_)->cm.xio.df)

#define mpig_cm_xio_vc_get_state_class(vc_)								\
    ((mpig_cm_xio_vc_state_classes_t)									\
     (((vc_)->cm.xio.state & MPIG_CM_XIO_VC_STATE_CLASS_MASK) >> MPIG_CM_XIO_VC_STATE_CLASS_SHIFT))

#define mpig_cm_xio_vc_is_undefined(vc_)						\
    ((mpig_cm_xio_vc_get_state(vc_) == MPIG_CM_XIO_VC_STATE_UNDEFINED) ? TRUE : FALSE)

#define mpig_cm_xio_vc_validate_undefined_state(vc_)					\
    ((mpig_cm_xio_vc_get_state(vc_) == MPIG_CM_XIO_VC_STATE_UNDEFINED) ? TRUE : FALSE)

#define mpig_cm_xio_vc_is_unconnected(vc_)							\
    ((mpig_cm_xio_vc_get_state(vc_) == MPIG_CM_XIO_VC_STATE_UNCONNECTED) ? TRUE : FALSE)

#define mpig_cm_xio_vc_validate_unconnected_state(vc_)						\
    ((mpig_cm_xio_vc_get_state(vc_) == MPIG_CM_XIO_VC_STATE_UNCONNECTED) ? TRUE : FALSE)

#define mpig_cm_xio_vc_is_connecting(vc_)								\
    ((mpig_cm_xio_vc_get_state_class(vc_) == MPIG_CM_XIO_VC_STATE_CLASS_CONNECTING) ? TRUE : FALSE)

#define mpig_cm_xio_vc_validate_connecting_state(vc_)						\
    ((mpig_cm_xio_vc_get_state(vc_) > MPIG_CM_XIO_VC_STATE_CONNECTING_FIRST &&			\
      mpig_cm_xio_vc_get_state(vc_) < MPIG_CM_XIO_VC_STATE_CONNECTING_LAST) ? TRUE : FALSE)
    
#define mpig_cm_xio_vc_is_connected(vc_)								\
    ((mpig_cm_xio_vc_get_state_class(vc_) == MPIG_CM_XIO_VC_STATE_CLASS_CONNECTED) ? TRUE : FALSE)

#define mpig_cm_xio_vc_validate_connected_state(vc_)						\
    ((mpig_cm_xio_vc_get_state(vc_) > MPIG_CM_XIO_VC_STATE_CONNECTED_FIRST &&			\
      mpig_cm_xio_vc_get_state(vc_) < MPIG_CM_XIO_VC_STATE_CONNECTED_LAST) ? TRUE : FALSE)
    
#define mpig_cm_xio_vc_is_disconnecting(vc_)								\
    ((mpig_cm_xio_vc_get_state_class(vc_) == MPIG_CM_XIO_VC_STATE_CLASS_DISCONNECTING) ? TRUE : FALSE)

#define mpig_cm_xio_vc_validate_disconnecting_state(vc_)				\
    ((mpig_cm_xio_vc_get_state(vc_) > MPIG_CM_XIO_VC_STATE_DISCONNECTING_FIRST &&		\
      mpig_cm_xio_vc_get_state(vc_) < MPIG_CM_XIO_VC_STATE_DISCONNECTING_LAST) ? TRUE : FALSE)
    
#define mpig_cm_xio_vc_has_failed(vc_)								\
    ((mpig_cm_xio_vc_get_state_class(vc_) == MPIG_CM_XIO_VC_STATE_CLASS_FAILED) ? TRUE : FALSE)

#define mpig_cm_xio_vc_validate_failed_state(vc_)					\
    ((mpig_cm_xio_vc_get_state(vc_) > MPIG_CM_XIO_VC_STATE_FAILED_FIRST &&		\
      mpig_cm_xio_vc_get_state(vc_) < MPIG_CM_XIO_VC_STATE_FAILED_LAST) ? TRUE : FALSE)

#define mpig_cm_xio_vc_is_temporary(vc_) (((vc_)->cm.xio.cs == NULL) ? TRUE : FALSE)


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_vc_destruct_fn([IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_destruct_fn
MPIG_STATIC void mpig_cm_xio_vc_destruct_fn(mpig_vc_t * vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);

    MPIG_UNUSED_VAR(fcname);

    mpig_cm_xio_vc_destruct(vc);
}
/* void mpig_cm_xio_vc_destruct_fn() */


/*
 * char * mpig_cm_xio_vc_state_get_string([IN] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_state_get_string
MPIG_STATIC const char * mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_states_t vc_state)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * str;
    
    MPIG_UNUSED_VAR(fcname);

    switch(vc_state)
    {
	case MPIG_CM_XIO_VC_STATE_UNDEFINED:
	    str = "MPIG_CM_XIO_VC_STATE_UNDEFINED";
	    break;
	case MPIG_CM_XIO_VC_STATE_UNCONNECTED:
	    str = "MPIG_CM_XIO_VC_STATE_UNCONNECTED";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECTING:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECTING";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPTING:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPTING";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECT_OPENING:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECT_OPENING";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_MAGIC:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_MAGIC";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_MAGIC:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_MAGIC";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_OPEN_REQ:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_OPEN_REQ";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_OPEN_RESP:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECT_CLOSING_NAK:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECT_CLOSING_NAK";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_OPEN:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_OPEN";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_MAGIC:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_MAGIC";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_MAGIC:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_MAGIC";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_OPEN_REQ:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_OPEN_REQ";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ACK:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ACK";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_NAK:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_NAK";
	    break;
	case MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR:
	    str = "MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECTED:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECTED";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECTED_RECEIVED_CLOSE_REQ:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECTED_RECEIVED_CLOSE_REQ";
	    break;
	case MPIG_CM_XIO_VC_STATE_CONNECTED_SENT_CLOSE_REQ:
	    str = "MPIG_CM_XIO_VC_STATE_CONNECTED_SENT_CLOSE_REQ";
	    break;
	case MPIG_CM_XIO_VC_STATE_DISCONNECTING_SENDING_ACK:
	    str = "MPIG_CM_XIO_VC_STATE_DISCONNECTING_SENDING_ACK";
	    break;
	case MPIG_CM_XIO_VC_STATE_DISCONNECTING_RECEIVING_ACK:
	    str = "MPIG_CM_XIO_VC_STATE_DISCONNECTING_RECEIVING_ACK";
	    break;
	case MPIG_CM_XIO_VC_STATE_DISCONNECTING_LAST:
	    str = "MPIG_CM_XIO_VC_STATE_DISCONNECTING_LAST";
	    break;
	case MPIG_CM_XIO_VC_STATE_FAILED_CONNECTING:
	    str = "MPIG_CM_XIO_VC_STATE_FAILED_CONNECTING";
	    break;
	case MPIG_CM_XIO_VC_STATE_FAILED_CONNECTION:
	    str = "MPIG_CM_XIO_VC_STATE_FAILED_CONNECTION";
	    break;
	case MPIG_CM_XIO_VC_STATE_FAILED_DISCONNECTING:
	    str = "MPIG_CM_XIO_VC_STATE_FAILED_DISCONNECTING:";
	    break;
	default:
	    str = "(unrecognized vc state)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_vc_state_get_string() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						     END VC OBJECT MANAGMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN VC FUNCTION TABLE
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

/* prototypes for VC function table functions */
MPIG_STATIC int mpig_cm_xio_adi3_isend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_xio_adi3_irsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_xio_adi3_issend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_xio_adi3_recv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status,
    MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_xio_adi3_irecv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_xio_adi3_cancel_send(MPID_Request * rreq);

MPIG_STATIC void mpig_cm_xio_vc_recv_any_source(
    mpig_vc_t * vc, MPID_Request * rreq, MPID_Comm * comm, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_vc_dec_ref_count_and_close(
    mpig_vc_t * vc, bool_t * inuse, int * mpi_errno_p, bool_t * failed_p);


/* VC function table definition */
MPIG_STATIC mpig_vc_cm_funcs_t mpig_cm_xio_vc_funcs =
{
    mpig_cm_xio_adi3_isend,
    mpig_cm_xio_adi3_isend,
    mpig_cm_xio_adi3_irsend,
    mpig_cm_xio_adi3_irsend,
    mpig_cm_xio_adi3_issend,
    mpig_cm_xio_adi3_issend,
    mpig_cm_xio_adi3_recv,
    mpig_cm_xio_adi3_irecv,
    mpig_adi3_cancel_recv,
    mpig_cm_xio_adi3_cancel_send,
    mpig_cm_xio_vc_recv_any_source,
    NULL, /* vc_inc_ref_count */
    mpig_cm_xio_vc_dec_ref_count_and_close,
    mpig_cm_xio_vc_destruct_fn
};


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * int mpig_cm_xio_adi3_isend([IN] buf, [IN] cnt, [IN] dt, [IN] rank, [IN] tag, [IN/MOD] comm, [IN] ctxoff, [OUT] sreqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_isend
MPIG_STATIC int mpig_cm_xio_adi3_isend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    MPID_Request * sreq = NULL;
    mpig_vc_t * vc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_isend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    mpig_comm_get_vc(comm, rank, &vc);
    MPIU_Assert(mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO);

    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 2, 1, (void *) buf, cnt, dt, rank, tag, ctx, comm, vc, &sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_cm_xio_request_set_cc(sreq, 1);
    
    mpig_cm_xio_send_enq_isend(vc, sreq, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_isend");

    *sreqp = sreq;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_isend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (sreq != NULL)
    {
	mpig_request_destroy(sreq);
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_adi3_isend() */


/*
 * int mpig_cm_xio_adi3_irsend([IN] buf, [IN] cnt, [IN] dt, [IN] rank, [IN] tag, [IN/MOD] comm, [IN] ctxoff, [OUT] sreqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_irsend
MPIG_STATIC int mpig_cm_xio_adi3_irsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    MPID_Request * sreq = NULL;
    mpig_vc_t * vc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_irsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_irsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm="
		       MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    mpig_comm_get_vc(comm, rank, &vc);
    MPIU_Assert(mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO);

    mpig_request_create_isreq(MPIG_REQUEST_TYPE_RSEND, 2, 1, (void *) buf, cnt, dt, rank, tag, ctx, comm, vc, &sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_cm_xio_request_set_cc(sreq, 1);

    mpig_cm_xio_send_enq_isend(vc, sreq, &mpi_errno, & failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_isend");

    *sreqp = sreq;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_irsend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (sreq != NULL)
    {
	mpig_request_destroy(sreq);
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_adi3_irsend() */


/*
 * int mpig_cm_xio_adi3_issend([IN] buf, [IN] cnt, [IN] dt, [IN] rank, [IN] tag, [IN/MOD] comm, [IN] ctxoff, [OUT] sreqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_issend
MPIG_STATIC int mpig_cm_xio_adi3_issend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    MPID_Request * sreq = NULL;
    mpig_vc_t * vc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_issend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_issend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm="
		       MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    mpig_comm_get_vc(comm, rank, &vc);
    MPIU_Assert(mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO);
    
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SSEND, 2, 1, (void *) buf, cnt, dt, rank, tag, ctx, comm, vc, &sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_cm_xio_request_set_cc(sreq, 2);

    mpig_cm_xio_send_enq_isend(vc, sreq, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_isend");

    *sreqp = sreq;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_issend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (sreq != NULL)
    {
	mpig_request_destroy(sreq);
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_adi3_issend() */


/*
 * int mpig_cm_xio_adi3_recv([IN] buf, [IN] cnt, [IN] dt, [IN] rank, [IN] tag, [IN/MOD] comm, [IN] ctxoff, [OUT] status,
 *			     [OUT] rreqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_recv
MPIG_STATIC int mpig_cm_xio_adi3_recv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPI_Status * const status, MPID_Request ** const rreqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_recv);

    mpi_errno = mpig_cm_xio_adi3_irecv(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp);
    /* the status will be extracted by MPI_Recv() once the request is complete */
    
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_recv);
    return mpi_errno;
}
/* mpig_cm_xio_adi3_recv() */


/*
 * int mpig_cm_xio_adi3_irecv([IN] buf, [IN] cnt, [IN] dt, [IN] rank, [IN] tag, [IN/MOD] comm, [IN] ctxoff, [OUT] rreqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_irecv
MPIG_STATIC int mpig_cm_xio_adi3_irecv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int ctx = comm->context_id + ctxoff;
    MPID_Request * rreq;
    struct mpig_cm_xio_request * rreq_cm = NULL;
    bool_t rreq_complete = FALSE;
    bool_t rreq_found;
    mpig_vc_t * vc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_irecv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_irecv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm="
		       MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    /* get the associated with the remote process */
    mpig_comm_get_vc(comm, rank, &vc);
    MPIU_Assert(mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO);
	
    rreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, comm->context_id + ctxoff, &rreq_found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
    rreq_cm = &rreq->cm.xio;

    if (rreq_found)
    {
	/* message was found in the unexepected queue */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "request found in unexpected queue: rreq= " MPIG_HANDLE_FMT ", rreqp="  MPIG_PTR_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq));

	/* finish filling in the request fields */
	mpig_request_set_buffer(rreq, buf, cnt, dt);
	mpig_request_add_comm_ref(rreq, comm); /* used by MPI layer and ADI3 selection mechanism (in mpidpost.h), most notably
						  by the receive cancel routine */
		
	if (mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA)
	{
	    /* the request contains an eager message. */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			       "eager protocol used to send message: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
			       rreq->handle, (MPIG_PTR_CAST) rreq));

	    /* if this is a synchronous send, then we need to send an acknowledgement back to the sender. */
	    if (mpig_cm_xio_request_get_sreq_type(rreq) == MPIG_REQUEST_TYPE_SSEND)
	    {
		mpig_cm_xio_send_enq_ssend_ack_msg(vc, mpig_request_get_remote_req_id(rreq), &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_ssend_ack_msg");
	    }
	    
	    if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE)
	    {
		/* All of the data has arrived, we need to unpack the data and then free the buffer and the request. */
		mpig_cm_xio_stream_rreq_init(rreq, &mpi_errno, &failed);
		MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_init");

		mpig_iov_reset(rreq_cm->iov, 0);
		mpig_cm_xio_stream_rreq_unpack(rreq, &mpi_errno, &failed);
		MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_unpack");
		MPIU_Assert(mpig_iov_get_num_bytes(rreq_cm->iov) == 0);
		/* mpig_databuf_destroy(rreq_cm->databuf); -- destroyed by unpack routine */
		/* rreq->status.count is set by unpack routines */
		mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
	    }
	    else if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA)
	    {
		/* the data is still being transfered across the net.  the progress engine will unpack it once the entire message
		   has arrived. associate the communicator and datatype with the request so they are avaiable for later use. */
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED);
		mpig_request_add_dt_ref(rreq, dt);
	    }
	    else if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_RSEND_DATA)
	    {
		/* an rsend arrived without a posted request.  the data is still being drained from the network.  the completion
		   counter is set to two.  when the drain completes, the count is decrement.  we decrement it here so that when
		   both operations complete the request will complete without an futher explicit coordination. */
		mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
	    }
	}
	else if (mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS)
	{
	    /* message sent using the rendezvous protocol */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			       "rendezvous protocol used to send message: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
			       rreq->handle, (MPIG_PTR_CAST) rreq));

	    /* send a clear-to-send message to the remote process */
	    mpig_cm_xio_send_enq_rndv_cts_msg(vc, rreq->handle, mpig_request_get_remote_req_id(rreq), &mpi_errno, &failed);
	    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_rndv_cts_msg");

	    /* associate the communicator and datatype with the request so they are avaiable for later use */
	    mpig_request_add_dt_ref(rreq, dt);

	    if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA)
	    {
		/* unpack the data in the unexpected buffer, and prepare IOV for when the RNDV_DATA message arrives */
		mpig_cm_xio_stream_rreq_init(rreq, &mpi_errno, &failed);
		MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_init");

		mpig_iov_reset(rreq_cm->iov, 0);
		mpig_cm_xio_stream_set_max_pos(rreq, mpig_databuf_get_remaining_bytes(rreq_cm->databuf));
		mpig_cm_xio_stream_rreq_unpack(rreq, &mpi_errno, &failed);
		MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_unpack");
		MPIU_Assert(mpig_iov_get_num_bytes(rreq_cm->iov) == 0);
	    }
	    else
	    {
		MPIU_Assert(mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA);
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED);
	    }
	}
	else
	{
	    /* UNKNOWN MESSAGE TYPE */
	    MPIU_Assertp(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA ||
			 mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);
	}
    }
    else
    {
	/* message has yet to arrived.  the request has been placed on the list of posted receive requests and populated with
           information supplied in the arguments. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "request allocated in posted queue: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq));
	
	mpig_request_construct_irreq(rreq, 2, 1, buf, cnt, dt, rank, tag, ctx, comm, vc);
	mpig_cm_xio_request_construct(rreq);
	mpig_cm_xio_request_set_cc(rreq, 1);
	mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED);
    }

    *rreqp = rreq;
    
  fn_return:
    if (rreq != NULL)
    {
	/* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
	mpig_request_mutex_unlock(rreq);
	
        /* if all tasks associated with the request have completed, then added it to the completion queue */
	if (rreq_complete == TRUE) mpig_cm_xio_rcq_enq(rreq);
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(rreq), (MPIG_PTR_CAST) rreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_irecv);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_adi3_irecv() */


/*
 * int mpig_cm_xio_adi3_cancel_send([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_cancel_send
MPIG_STATIC int mpig_cm_xio_adi3_cancel_send(MPID_Request * const sreq)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t sreq_locked = FALSE;
    mpig_vc_t * const vc = mpig_request_get_vc(sreq);
    bool_t vc_locked = FALSE;
    bool_t sreq_complete = FALSE;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_cancel_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_cancel_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: vc=" MPIG_PTR_FMT ", sreq= " MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) vc, sreq->handle, (MPIG_PTR_CAST) sreq));

    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	mpig_request_mutex_lock(sreq);
	sreq_locked = TRUE;
	{
	    /* first try to locate and remove the request from the send queue */
	    if (vc->cm.xio.active_sreq != sreq && mpig_cm_xio_sendq_find_and_deq(vc, sreq))
	    {
		/* if the request is successfully dequeued, then set it's internal completion counter to zero and enqueue it on the
		   request completion queue. */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
				   "message removed from send queue; completing request: vc=" MPIG_PTR_FMT
				   ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc, sreq->handle,
				   (MPIG_PTR_CAST) sreq));
	    
		sreq->status.cancelled = TRUE;
		mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE);
		mpig_cm_xio_request_set_cc(sreq, 0);
		sreq_complete = TRUE;
	    }
	    else
	    {
		bool_t sreq_was_complete;
		int rank;
		int tag;
		int ctx;

		mpig_request_get_envelope(sreq, &rank, &tag, &ctx);
		
		/* if that fails, send a message to the remote process ask it to remove the message from its unexpected queue */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
				   "sending cancel request to remote process: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
				   ", sreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d", (MPIG_PTR_CAST) vc, sreq->handle,
				   (MPIG_PTR_CAST) sreq, rank, tag, ctx));
	    
		mpig_cm_xio_send_enq_cancel_send_msg(vc, sreq->comm->rank, tag, ctx, sreq->handle, &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_cancel_send_msg");
	
		/* adjust the request's completion counters and reference count to insure the request lives until the response is
		   received is received from the remote process */
		mpig_cm_xio_request_inc_cc(sreq, &sreq_was_complete);
		if (sreq_was_complete == TRUE)
		{
		    bool_t sreq_was_inuse;
		    mpig_request_inc_cc(sreq, &sreq_was_complete);
		    if (sreq_was_complete == TRUE)
		    {
			mpig_request_inc_ref_count(sreq, &sreq_was_inuse);
			if (sreq_was_inuse == FALSE)
			{   /* --BEGIN ERROR HANDLING-- */
			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
					       "ERROR: attempt to cancel completed request; likely a dangling handle, sreqp="
					       MPIG_PTR_FMT, (MPIG_PTR_CAST) sreq));
			    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cancel_completed_sreq",
					  "**globus|cancel_completed_sreq %p", sreq);
			    MPID_Abort(NULL, mpi_errno, 13, NULL);
			}   /* --END ERROR HANDLING-- */
		    }
		}
	    }
	}
	mpig_request_mutex_unlock(sreq);
	sreq_locked = FALSE;
    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;
    
  fn_return:
    /* if the request was succesfully cancelled, then added it to the completion queue */
    if (sreq_complete == TRUE) mpig_cm_xio_rcq_enq(sreq);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       sreq->handle, (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_cancel_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_request_mutex_unlock_conditional(sreq, (sreq_locked));
	mpig_vc_mutex_unlock_conditional(vc, (vc_locked));
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_adi3_cancel_send() */


/*
 * int mpig_cm_xio_vc_recv_any_source([IN/MOD] vc, [IN/MOD] rreq, [IN] comm, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * The request was found in the unexpected queue by the mpig_cm_other_adi3_irecv() routine.  This routine needs to extract the
 * extract the data from the unexpected buffer and place it into the user buffer.  It also must handle any protocol issues for
 * acquiring additional data associated with the message.
 *
 * MT-NOTE: the request's mutex is locked and unlocked by the calling routine.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_recv_any_source
MPIG_STATIC void mpig_cm_xio_vc_recv_any_source(
    mpig_vc_t * const vc, MPID_Request * const rreq, MPID_Comm * const comm, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request * rreq_cm = &rreq->cm.xio;
    bool_t rreq_complete = FALSE;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_recv_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: vc=" MPIG_PTR_FMT ", rreq=" MPIG_PTR_FMT ", mpi_errno_p=0x%08x",
		       (MPIG_PTR_CAST) vc, (MPIG_PTR_CAST) rreq, *mpi_errno_p));
    *failed_p = FALSE;
    
    MPIU_Assert(mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO);
    MPIU_Assert(mpig_request_get_vc(rreq) == vc);

    if (mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA)
    {
	/* the request contains an eager message. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "eager protocol used to send message: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq));

	/* if this is a synchronous send, then we need to send an acknowledgement back to the sender. */
	if (mpig_cm_xio_request_get_sreq_type(rreq) == MPIG_REQUEST_TYPE_SSEND)
	{
	    mpig_cm_xio_send_enq_ssend_ack_msg(vc, mpig_request_get_remote_req_id(rreq), mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_ssend_ack_msg");
	}
	    
	if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE)
	{
	    /* All of the data has arrived, we need to unpack the data and then free the buffer and the request. */
	    mpig_cm_xio_stream_rreq_init(rreq, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDSTMT((failed), *mpi_errno_p, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_init");

	    mpig_iov_reset(rreq_cm->iov, 0);
	    mpig_cm_xio_stream_rreq_unpack(rreq, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDSTMT((failed), *mpi_errno_p, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_unpack");
	    MPIU_Assert(mpig_iov_get_num_bytes(rreq_cm->iov) == 0);
	    /* mpig_databuf_destroy(rreq_cm->databuf); -- destroyed by unpack routine */
	    /* rreq->status.count is set by unpack routines */
	    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
	}
	else if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA)
	{
	    /* the data is still being transfered across the net.  the progress engine will unpack it once the entire message
	       has arrived. associate the communicator and datatype with the request so they are avaiable for later use. */
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED);
	    mpig_request_add_dt_ref(rreq, mpig_request_get_dt(rreq));
	}
	else if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_RSEND_DATA)
	{
	    /* an rsend arrived without a posted request.  the data is still being drained from the network.  the completion
	       counter is set to two.  when the drain completes, the count is decrement.  we decrement it here so that when
	       both operations complete the request will complete without an futher explicit coordination. */
	    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
	}
    }
    else if (mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS)
    {
	/* message sent using the rendezvous protocol */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "rendezvous protocol used to send message: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq));

	/* send a clear-to-send message to the remote process */
	mpig_cm_xio_send_enq_rndv_cts_msg(vc, rreq->handle, mpig_request_get_remote_req_id(rreq), mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_rndv_cts_msg");

	/* associate the communicator and datatype with the request so they are avaiable for later use */
	mpig_request_add_dt_ref(rreq, mpig_request_get_dt(rreq));

	if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA)
	{
	    /* unpack the data in the unexpected buffer, and prepare IOV for when the RNDV_DATA message arrives */
	    mpig_cm_xio_stream_rreq_init(rreq, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDSTMT((failed), *mpi_errno_p, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_init");

	    mpig_iov_reset(rreq_cm->iov, 0);
	    mpig_cm_xio_stream_set_max_pos(rreq, mpig_databuf_get_remaining_bytes(rreq_cm->databuf));
	    mpig_cm_xio_stream_rreq_unpack(rreq, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDSTMT((failed), *mpi_errno_p, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_unpack");
	    MPIU_Assert(mpig_iov_get_num_bytes(rreq_cm->iov) == 0);
	}
	else
	{
	    MPIU_Assert(mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA);
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED);
	}
    }
    else
    {
	/* UNKNOWN MESSAGE TYPE */
	MPIU_Assertp(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA ||
		     mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);
    }
    
  fn_return:
    /* if all tasks associated with the request have completed, then added it to the completion queue */
    if (rreq_complete == TRUE) mpig_cm_xio_rcq_enq(rreq);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: vc=" MPIG_PTR_FMT ", rreq=" MPIG_PTR_FMT ", mpi_errno_p=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) vc, (MPIG_PTR_CAST) rreq, *mpi_errno_p, MPIG_BOOL_STR(failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_recv_any_source);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *failed_p = TRUE;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_vc_recv_any_source() */


/*
 * void mpig_cm_xio_vc_dec_ref_count_and_close([IN/MOD] vc, [OUT] inuse)
 *
 * MT-NOTE: this routine assume that the VC's mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_dec_ref_count_and_close
MPIG_STATIC void mpig_cm_xio_vc_dec_ref_count_and_close(
    mpig_vc_t * const vc, bool_t * const is_inuse_p, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const vc_cm = &vc->cm.xio;

    MPIG_UNUSED_VAR(fcname);

    mpig_cm_xio_vc_dec_ref_count(vc, is_inuse_p);
    if (*is_inuse_p == FALSE && vc_cm->handle != NULL)
    {
	mpig_cm_xio_disconnect(vc, mpi_errno_p, failed_p);
	*is_inuse_p = TRUE;
    }
    else
    {
	*failed_p = FALSE;
    }
}
/* mpig_cm_xio_vc_dec_ref_count_and_close() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						      END VC FUNCTION TABLE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
							BEGIN SEND QUEUE

MT-NOTE: These routines are NOT thread safe.  They assume that the VC mutex is already held.

MT-RC-NOTE: These routines modify the send request.  It is the calling routines responsibility to insure that the appropriate
acquire and releases are performed on machines with release consistent memory models.
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_sendq_construct(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_sendq_destruct(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_sendq_enq_head(mpig_vc_t * vc, MPID_Request * sreq);

MPIG_STATIC void mpig_cm_xio_sendq_enq_head(mpig_vc_t * vc, MPID_Request * sreq);

MPIG_STATIC void mpig_cm_xio_sendq_deq(mpig_vc_t * vc, MPID_Request ** sreqp);

MPIG_STATIC bool_t mpig_cm_xio_sendq_find_and_deq(mpig_vc_t * vc, MPID_Request * sreq);

MPIG_STATIC MPID_Request * mpig_cm_xio_sendq_head(mpig_vc_t * vc);

MPIG_STATIC bool_t mpig_cm_xio_sendq_empty(mpig_vc_t * vc);

#define mpig_cm_xio_sendq_construct(vc_)	\
{						\
    (vc_)->cm.xio.sendq_head = NULL;		\
    (vc_)->cm.xio.sendq_tail = NULL;		\
}

#define mpig_cm_xio_sendq_destruct(vc_)			\
{							\
    MPIU_Assert((vc_)->cm.xio.sendq_head == NULL);	\
    MPIU_Assert((vc_)->cm.xio.sendq_tail == NULL);	\
}

#define mpig_cm_xio_sendq_enq_head(vc_, sreq_)											\
{																\
    (sreq_)->cm.xio.sendq_next = (vc_)->cm.xio.sendq_head;									\
    if ((vc_)->cm.xio.sendq_tail == NULL)											\
    {																\
	(vc_)->cm.xio.sendq_tail = (sreq_);											\
    }																\
    (vc_)->cm.xio.sendq_head = (sreq_);												\
																\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,								\
		       "sendq - req enqueued at head: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,	\
		       (MPIG_PTR_CAST) (vc_), (sreq_)->handle, (MPIG_PTR_CAST) (sreq_)));					\
}

#define mpig_cm_xio_sendq_enq_tail(vc_, sreq_)											\
{																\
    if ((vc_)->cm.xio.sendq_tail != NULL)											\
    {																\
	(vc_)->cm.xio.sendq_tail->cm.xio.sendq_next = (sreq_);									\
    }																\
    else															\
    {																\
	(vc_)->cm.xio.sendq_head = (sreq_);											\
    }																\
    (vc_)->cm.xio.sendq_tail = (sreq_);												\
																\
    (sreq_)->cm.xio.sendq_next = NULL;												\
																\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,								\
		       "sendq - req enqueued at tail: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,	\
		       (MPIG_PTR_CAST) (vc_), (sreq_)->handle, (MPIG_PTR_CAST) (sreq_)));					\
}

#define mpig_cm_xio_sendq_deq(vc_, sreqp_)										\
{															\
    *(sreqp_) = (vc_)->cm.xio.sendq_head;										\
															\
    if ((vc_)->cm.xio.sendq_head != NULL)										\
    {															\
	(vc_)->cm.xio.sendq_head = (vc_)->cm.xio.sendq_head->cm.xio.sendq_next;						\
	if ((vc_)->cm.xio.sendq_head == NULL)										\
	{														\
	    (vc_)->cm.xio.sendq_tail = NULL;										\
	}														\
															\
	(*(sreqp_))->cm.xio.sendq_next = NULL;										\
															\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,						\
			   "sendq - req dequeued: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,	\
			   (MPIG_PTR_CAST) (vc_), (*(sreqp_))->handle, (MPIG_PTR_CAST) *(sreqp_)));			\
    }															\
    else														\
    {															\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,						\
			   "sendq - queue empty; no request dequeued: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (vc_)));	\
    }															\
}

#define mpig_cm_xio_sendq_head(vc_) ((vc_)->cm.xio.sendq_head)

#define mpig_cm_xio_sendq_empty(vc_) (((vc_)->cm.xio.sendq_head == NULL) ? TRUE : FALSE)


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_sendq_find_and_deq([IN/MOD] vc, [IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_sendq_find_and_deq
MPIG_STATIC bool_t mpig_cm_xio_sendq_find_and_deq(mpig_vc_t * vc, MPID_Request * sreq)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * cur_sreq;
    MPID_Request * prev_sreq;
    bool_t found = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_sendq_find_and_deq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_sendq_find_and_deq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,
		       "entering: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) vc, sreq->handle, (MPIG_PTR_CAST) sreq));

    prev_sreq = NULL;
    cur_sreq = vc->cm.xio.sendq_head;

    while (cur_sreq != NULL)
    {
	if (cur_sreq == sreq)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,
			       "sreq found and dequeued: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,
			       (MPIG_PTR_CAST) vc, sreq->handle, (MPIG_PTR_CAST) sreq));
	    
	    if (prev_sreq == NULL)
	    {
		vc->cm.xio.sendq_head = sreq->cm.xio.sendq_next;
	    }
	    else
	    {
		prev_sreq->cm.xio.sendq_next = sreq->cm.xio.sendq_next;
	    }

	    if (vc->cm.xio.sendq_tail == sreq)
	    {
		vc->cm.xio.sendq_tail = sreq->cm.xio.sendq_next;
	    }

	    sreq->cm.xio.sendq_next = NULL;
	    found = TRUE;
	    break;
	}

	prev_sreq = cur_sreq;
	cur_sreq = cur_sreq->cm.xio.sendq_next;
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT | MPIG_DEBUG_LEVEL_VC,
		       "exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", found=%s",
		       (MPIG_PTR_CAST) vc, sreq->handle, (MPIG_PTR_CAST) sreq, MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_sendq_find_and_deq);
    return found;
}
/* mpig_cm_xio_sendq_find_and_deq() */


#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
							END SEND QUEUE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN VC TRACKING LIST
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC mpig_vc_t * mpig_cm_xio_vc_list = NULL;
MPIG_STATIC globus_cond_t mpig_cm_xio_vc_list_cond;

MPIG_STATIC void mpig_cm_xio_vc_list_init(void);

MPIG_STATIC void mpig_cm_xio_vc_list_finalize(void);

MPIG_STATIC void mpig_cm_xio_vc_list_add(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_vc_list_remove(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_vc_list_wait_empty(void);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */

/*
 * void mpig_cm_xio_vc_list_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_list_init
void mpig_cm_xio_vc_list_init()
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_list_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_list_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering"));
    globus_cond_init(&mpig_cm_xio_vc_list_cond, NULL);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_list_init);
    
}
/* mpig_cm_xio_vc_list_init() */


/*
 * void mpig_cm_xio_vc_list_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_list_finalize
void mpig_cm_xio_vc_list_finalize()
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_list_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_list_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering"));
    globus_cond_destroy(&mpig_cm_xio_vc_list_cond);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_list_finalize);
}
/* mpig_cm_xio_vc_list_finalize() */


/*
 * void mpig_cm_xio_vc_list_add([IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes the VC's mutex is already locked by the current context
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_list_add
void mpig_cm_xio_vc_list_add(mpig_vc_t * vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_list_add);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_list_add);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));

    mpig_cm_xio_mutex_lock();
    {
	vc_cm->list_prev = NULL;
	vc_cm->list_next = mpig_cm_xio_vc_list;
	if (mpig_cm_xio_vc_list != NULL)
	{
	    mpig_cm_xio_vc_list->cm.xio.list_prev = vc;
	}
	mpig_cm_xio_vc_list = vc;
    }
    mpig_cm_xio_mutex_unlock();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_list_add);
}
/* mpig_cm_xio_vc_list_add() */


/*
 * void mpig_cm_xio_vc_list_remove([IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes the VC's mutex is already locked by the current context
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_list_remove
void mpig_cm_xio_vc_list_remove(mpig_vc_t * vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_list_remove);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_list_remove);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));

    mpig_cm_xio_mutex_lock();
    {
	if (vc_cm->list_prev == NULL)
	{
	    mpig_cm_xio_vc_list = vc_cm->list_next;
	    if (mpig_cm_xio_vc_list == NULL)
	    {
		globus_cond_signal(&mpig_cm_xio_vc_list_cond);
	    }
	}
	else
	{
	    vc_cm->list_prev->cm.xio.list_next = vc_cm->list_next;
	}
    
	if (vc_cm->list_next != NULL)
	{
	    vc_cm->list_next->cm.xio.list_prev = vc_cm->list_prev;
	}
    }
    mpig_cm_xio_mutex_unlock();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_list_remove);
}
/* mpig_cm_xio_vc_list_remove() */


/*
 * void mpig_cm_xio_vc_list_wait_empty([IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_list_wait_empty
void mpig_cm_xio_vc_list_wait_empty(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_list_wait_empty);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_list_wait_empty);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering"));

    mpig_cm_xio_mutex_lock();
    {
	while (mpig_cm_xio_vc_list != NULL)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC | MPIG_DEBUG_LEVEL_THREADS,
			       "waitng for VC list to empty: head_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) mpig_cm_xio_vc_list));
	    globus_cond_wait(&mpig_cm_xio_vc_list_cond, &mpig_cm_xio_mutex);
	}
    }
    mpig_cm_xio_mutex_unlock();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_list_wait_empty);
}
/* mpig_cm_xio_vc_list_wait_empty() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						      END VC TRACKING LIST
**********************************************************************************************************************************/
