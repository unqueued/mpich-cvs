#include "mpidi_ch3_impl.h"
#include "sctp_common.h"




void print_SCTP_event(struct MPIDU_Sctp_event * eventp);

int inline MPIDU_Sctp_post_writev(MPIDI_VC_t* vc, MPID_Request* sreq, int offset,
				  MPIDU_Sock_progress_update_func_t fn, int stream_no);

inline static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb)
{
    MPID_IOV * const iov = *iovp;
    const int count = *countp;
    int offset = 0;
    
    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) 
							   iov[offset].MPID_IOV_BUF + nb);
	    iov[offset].MPID_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_event_enqueue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sctp_event_enqueue(MPIDU_Sctp_op_t op, MPIU_Size_t num_bytes, 
				    sctp_rcvinfo* sri, int fd, void * user_ptr, void* user_ptr2, int value, int error)
{
  struct MPIDU_Sctp_eventq_elem * eventq_elem;
  struct MPIDU_Sctp_event* new_event;
  int index;
  sctp_rcvinfo bogus;

  int mpi_errno = MPI_SUCCESS;
 
  // myct: eventq_head hasn't been initialized yet
  if(eventq_head == NULL) {
    eventq_head = MPIU_Malloc(sizeof(struct MPIDU_Sctp_eventq_elem));

    if(eventq_head == NULL) {
      MPIDI_DBG_PRINTF((50, FCNAME, "Malloc Failed!\n"));
      mpi_errno = -1;
      goto fn_exit;
    }

    eventq_head-> size = 0;
    eventq_head-> head = 0;
    eventq_head-> tail = 0;
    eventq_head-> next = NULL;
    eventq_tail = eventq_head;
  }

  // myct: tail is full, need to allocate a new one
  if(eventq_tail-> size == MPIDU_SCTP_EVENTQ_POOL_SIZE){
    eventq_elem = MPIU_Malloc(sizeof(struct MPIDU_Sctp_eventq_elem));

    if(eventq_elem == NULL) {
      MPIDI_DBG_PRINTF((50, FCNAME, "Malloc Failed!\n"));
      mpi_errno = -1;
      goto fn_exit;
    }

    eventq_elem-> size = 0;
    eventq_elem-> head = 0;
    eventq_elem-> tail = 0;
     eventq_elem-> next = NULL;
    eventq_tail->next = eventq_elem;
    eventq_tail = eventq_elem;
  }

  // myct: let's put the event in
  index = eventq_tail->tail;
  new_event = &eventq_tail->event[index];
  new_event-> op_type = op;
  new_event-> num_bytes = num_bytes;
  new_event-> fd = fd;
  new_event-> sri = (sri == NULL)? bogus : *sri;
  new_event-> user_ptr = user_ptr;
  new_event-> user_ptr2 = user_ptr2;
  new_event-> user_value = value;
  new_event-> error = error;

  eventq_tail->size++;
  eventq_tail->tail = (index+1) % MPIDU_SCTP_EVENTQ_POOL_SIZE;

 fn_exit:
  return mpi_errno;
}
/* end MPIDU_Socki_event_enqueue() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_event_dequeue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sctp_event_dequeue(struct MPIDU_Sctp_event * eventp)
{
    struct MPIDU_Sctp_eventq_elem* eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    
    // myct: check if queue is empty
    if(eventq_head == NULL || eventq_head->size <= 0) {
      mpi_errno = -1;
      eventp->op_type = 100;
      return mpi_errno;
    }

    *eventp = eventq_head->event[eventq_head->head];
    eventq_head->size--;
    eventq_head->head = (eventq_head->head + 1) % MPIDU_SCTP_EVENTQ_POOL_SIZE;

    // myct: demalloc eventq_head if it's empty, however, always maintain one
    if(eventq_head->size == 0 && eventq_tail != eventq_head) {
      eventq_elem = eventq_head;
      
      eventq_head = eventq_head-> next;
      MPIU_Free(eventq_elem);
    }
    
    return mpi_errno;
}
/* end MPIDU_Socki_event_dequeue() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_free_eventq_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void inline MPIDU_Sctp_free_eventq_mem(void)
{
  struct MPIDU_Sctp_eventq_elem* eventq_elem;
  
  while (eventq_head) {
    eventq_elem = eventq_head;
    eventq_head = eventq_head->next;
    MPIU_Free(eventq_elem);
  }

  eventq_head = NULL;
  eventq_tail = NULL;

}

static MPID_Request* create_request(MPID_IOV * iov, int iov_count, int iov_offset, MPIU_Size_t nb)
{
    MPID_Request * sreq;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST);
    /*MPIDI_STATE_DECL(MPID_STATE_MEMCPY);*/

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST);
    
    sreq = MPID_Request_create();
    /* --BEGIN ERROR HANDLING-- */
    if (sreq == NULL)
	return NULL;
    /* --END ERROR HANDLING-- */
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    
    /*
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sreq->dev.iov, iov, iov_count * sizeof(MPID_IOV));
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
    */
    for (i = 0; i < iov_count; i++)
    {
	sreq->dev.iov[i] = iov[i];
    }
    if (iov_offset == 0)
    {
	/*
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(&sreq->ch.pkt, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	*/
	MPIU_Assert(iov[0].MPID_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));
	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &sreq->ch.pkt;
    }
    sreq->dev.iov[iov_offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[iov_offset].MPID_IOV_BUF + nb);
    sreq->dev.iov[iov_offset].MPID_IOV_LEN -= nb;
    sreq->dev.iov_count = iov_count;
    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;

    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}


/* myct: May4 
 * PRE-CONDITION: stream state = MPIDI_CH3I_VC_STATE_UNCONNECTED
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_enqueue_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void inline MPIDU_Sctp_stream_init(MPIDI_VC_t* vc, MPID_Request* req, int stream){

  MPID_Request* conn_req = NULL;

  // myct: stream hasn't been used yet, need to enqueue a connection packet
  if(SEND_CONNECTED(vc, stream) != MPIDI_CH3I_VC_STATE_CONNECTING ){
    conn_req = create_request(VC_IOV(vc, stream), 2, 0, 0);

    if(conn_req)
    {
      /* myct: don't put it in sendQ, but directly in Global_SendQ */
      SEND_CONNECTED(vc, stream) = MPIDI_CH3I_VC_STATE_CONNECTING;

      // should be the connection request 
      MPIDU_Sctp_post_writev(vc, conn_req, 0, NULL, stream);
      
      
    } else {
      /* malloc failed */
      printf("CONN req malloc failed\n");
    }
    
  } 
  
  if(req) {
    MPIDI_CH3I_SendQ_enqueue_x(vc, req, stream);
  }
}


/* returns number of bytes written. chunks long messages. writes until
 *  done or it comes across an EAGAIN 
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sctp_writev(MPIDI_VC_t* vc, struct iovec* ldata,int iovcnt, int stream, int ppid, 
		      MPIU_Size_t* nb) {

  int byte_sent, r, nwritten =0, i, sz = 0;
  int mpi_errno = MPI_SUCCESS;
  static struct iovec cdata[MPID_IOV_LIMIT];
  struct iovec *data = cdata;
  struct sockaddr_in* to = (struct sockaddr_in *) &(vc->ch.to_address);

  MPIU_Assert(iovcnt > 0);

  memcpy(cdata, ldata, sizeof(struct iovec) *iovcnt);

  for(i = 0; i < iovcnt; i++) {
    sz += ldata[i].iov_len;
  }
  
  do {
    if(sz <= CHUNK)/*MPIDI_CH3_EAGER_MAX_MSG_SIZE)  FIXME */ {
      /* a short/eager message */
      r = sctp_writev(vc->ch.fd, data, iovcnt,
		      (struct sockaddr *) to, sizeof(*to), ppid, 0, stream, 0, 0);
    } else {
      byte_sent = (CHUNK > data->iov_len) ? data->iov_len : CHUNK;
      
      r = sctp_sendmsg(vc->ch.fd, data->iov_base, byte_sent, (struct sockaddr *) to,
		       sizeof(*to), ppid, 0, stream, 0, 0);
      
    }
    
    /* update iov's (adjust_iov is static and does not handle errors and "errors" (EAGAIN)) */
    
    if (r < 0) {  /* error */
      if (errno == EAGAIN)
	break;
      
      if (errno != EINTR) {
	perror("MPIDU_Sctp_writev");
	nwritten = 0;
	/* invalidate socket/association/VC? */
	break;
      }
      
    } else if (r == 0) {	       /* eof */
      perror("MPIDU_Sctp_writev");
      
      if (iovcnt > 0) {
	/* invalidate socket/association/VC? */
	nwritten = 0;
      }
      break;
      
    } else {  /* r > 0 */
      nwritten += r;
      adjust_iov(&data, &iovcnt, r);
    }
    
  } while(iovcnt > 0);
  
  *nb = nwritten;
  
  return (nwritten >= 0)? MPI_SUCCESS : -1;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sctp_write(MPIDI_VC_t* vc, void* buf, MPIU_Size_t len, 
		     int stream_no, int ppid, MPIU_Size_t* num_written){

  int err = MPI_SUCCESS;
  ssize_t nb;

  do {

    nb = my_sctp_send(vc->ch.fd, buf, len, &(vc->ch.to_address), stream_no, ppid);
    
  }while(nb == -1 && errno == EINTR);

  if(nb > 0)
    *num_written = nb;
  else if(errno == EAGAIN || errno == EWOULDBLOCK)
    *num_written = 0;

  return (nb >= 0)? MPI_SUCCESS: -1;
}

void print_SCTP_event(struct MPIDU_Sctp_event * eventp){

  //MPIDU_Sctp_event_t* ptr = eventq_head;

  /* while(ptr) { */
/*     printf("DEBUG sctp_event: type %d\n num_bytes %d\n fd %d\n stream_no %d\n",  */
/* 	 eventp->op_type, eventp->num_bytes, eventp->fd, eventp->stream_no); */
/*     ptr = ptr-> next; */
/*   } */

}


/* myct: read from user buffer */
int read_from_advbuf(char* from, char* to, int nbytes, int offset) {

  int r = nbytes;

  if(r > 0){
    memcpy(to, from+offset, r);
  } else
    r = -1;

  return r;
}

/* myct: readv from advance buffer 
 * preliminary implementation 
 */
int readv_from_advbuf(MPID_IOV* iovp, int iov_cnt, char* from, int bytes_read) {

  int offset = 0;
  int cur_iov_len = 0;
  int actual_read_len;
  int remain_bytes;
  int agg_read = 0;
  int i = 0;

  remain_bytes = bytes_read;

  while(i < iov_cnt && remain_bytes > 0) {
    
    cur_iov_len = iovp[i].MPID_IOV_LEN;

    actual_read_len = (remain_bytes > cur_iov_len)?
      cur_iov_len : remain_bytes;

    read_from_advbuf(from, iovp[i].MPID_IOV_BUF, actual_read_len, offset);

    remain_bytes -= actual_read_len;
    agg_read += actual_read_len;
    offset += actual_read_len;

    if(actual_read_len >= cur_iov_len)
      i++;
  }

  return agg_read;
}

#undef FUNCNAME
#define FUNCNAME Req_Stream_from_pkt_and_req
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq)
{
    int stream;
    
    switch(pkt->type)
    {
        /* FIXME account for each MPIDI_CH3_Pkt_type */

        /* brad : these types are internally identical */
        case MPIDI_CH3_PKT_EAGER_SEND :
        case MPIDI_CH3_PKT_EAGER_SYNC_SEND :
        case MPIDI_CH3_PKT_READY_SEND :
        case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
        case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND :
        {
	  MPIU_Assert(pkt->eager_send.match.context_id <= 2048);
	  MPIU_Assert(pkt->eager_send.match.context_id >= 0);
	  stream = Req_Stream_from_match(pkt->eager_send.match);
        }
        break;
        
        case MPIDI_CH3_PKT_RNDV_SEND :
        {
	  MPIU_Assert(sreq);
	  MPIU_Assert(sreq->dev.match.context_id <= 2048);
	  MPIU_Assert(sreq->dev.match.context_id >= 0);
	  stream = Req_Stream_from_match(sreq->dev.match);
        }
        break;
        
        default :
        case MPIDI_CH3_PKT_CLOSE :
        case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND :    /* brad : CTS has unset values here, so this case is arbitrary,
                                                  *    in other words, the stream returned is random at the moment
                                                  *    in order to avoid a segfault.
                                                  *
                                                  *  FIXME : need to be smart about assigning the CTS stream so it
                                                  *    doesn't overlap an already active stream
                                                  */
        {
	  stream = MPICH_SCTP_CTL_STREAM;
        }
        break;
    }
    MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,"stream %d returned",stream );

    return stream;        
}

/* myct: mar27 
 * MPIDI_Sctp_post_writev
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_post_writev
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline int MPIDU_Sctp_post_writev(MPIDI_VC_t* vc, MPID_Request* sreq, int offset,
				  MPIDU_Sock_progress_update_func_t fn, int stream_no)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV* iov = sreq->dev.iov + offset;
    int iov_n = sreq->dev.iov_count - offset;

    Global_SendQ_enqueue(vc, sreq, stream_no);

    SCTP_IOV* iov_ptr = &(vc->ch.posted_iov[stream_no]);
   
    POST_IOV(iov_ptr) = iov;
    POST_IOV_CNT(iov_ptr) = iov_n;
    POST_IOV_OFFSET(iov_ptr) = 0;
    POST_IOV_FLAG(iov_ptr) = TRUE;

    /* POST_UPDATE_FN(iov_ptr) = fn; */
 
  fn_exit:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_post_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
inline int MPIDU_Sctp_post_write(MPIDI_VC_t* vc, MPID_Request* sreq, MPIU_Size_t minlen, 
			  MPIU_Size_t maxlen, MPIDU_Sock_progress_update_func_t fn, int stream_no) {
  
  int mpi_errno = MPI_SUCCESS;
  void* buf = sreq->dev.iov[0].MPID_IOV_BUF;
  
  Global_SendQ_enqueue(vc, sreq, stream_no);

  SCTP_IOV* iov_ptr = &(vc->ch.posted_iov[stream_no]);

  POST_IOV_FLAG(iov_ptr) = FALSE;
  POST_BUF(iov_ptr) = buf;
  POST_BUF_MIN(iov_ptr) = minlen;
  POST_BUF_MAX(iov_ptr) = maxlen;
  
  /* POST_UPDATE_FN(iov_ptr) = fn; */

 fn_exit:
  return mpi_errno;
}


/* BUFFER Management routine */

static BufferNode_t* BufferList = NULL;

inline void BufferList_init(BufferNode_t* node) {
  BufferList = node;
  buf_init(BufferList);
}

inline int buf_init(BufferNode_t* node) {
  node-> free_space = READ_AMOUNT;
  node-> buf_ptr = node-> buffer;

  node-> next = NULL;
  node-> dynamic = FALSE;

  return MPI_SUCCESS;
}

inline int buf_clean(BufferNode_t* node) {
  return MPI_SUCCESS;
}

inline char* request_buffer(int size, BufferNode_t** bf_node) {
  BufferNode_t* node = BufferList;
  if(node-> free_space >= size) {
    *bf_node = node;
    return node-> buf_ptr;
  } else { 
    *bf_node = NULL;
    return NULL;
  }
}

inline int update_size(BufferNode_t* node, int size) {
  node-> free_space -= size;
  node-> buf_ptr += size;
}





/* END Buffer routines */

/* get rid of SOCK util */


#define MPIDI_CH3I_HOST_DESCRIPTION_KEY  "description"
#define MPIDI_CH3I_PORT_KEY              "port"
#define MPIDI_CH3I_IFNAME_KEY            "ifname"

#undef FUNCNAME
#define FUNCNAME my_MPIDI_CH3I_BCInit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int my_MPIDI_CH3I_BCInit( int pg_rank, 
		       char **publish_bc_p, char **bc_key_p,
		       char **bc_val_p, int *val_max_sz_p )
{
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
    int key_max_sz;

    /*
     * Publish the contact information (a.k.a. business card) for this 
     * process into the PMI keyval space associated with this process group.
     */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != 0)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_key_length_max", 
			     "**pmi_kvs_get_key_length_max %d", pmi_errno);
    }

    /* This memroy is returned by this routine */
    *bc_key_p = MPIU_Malloc(key_max_sz);
    if (*bc_key_p == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
    }

    pmi_errno = PMI_KVS_Get_value_length_max(val_max_sz_p);
    if (pmi_errno != 0)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_kvs_get_value_length_max",
			     "**pmi_kvs_get_value_length_max %d", pmi_errno);
    }

    /* This memroy is returned by this routine */
    *bc_val_p = MPIU_Malloc(*val_max_sz_p);
    if (*bc_val_p == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
    }
    *publish_bc_p = *bc_val_p;  /* need to keep a pointer to the front of the 
				   front of this buffer to publish */

    /* Create the bc key but not the value */ 
    mpi_errno = MPIU_Snprintf(*bc_key_p, key_max_sz, "P%d-businesscard", 
			      pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**snprintf",
			     "**snprintf %d", mpi_errno);
    }
    mpi_errno = MPI_SUCCESS;
    
    
  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME my_MPIDU_Sock_get_host_description
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int my_MPIDU_Sock_get_host_description(char * host_description, int len)
{
    char * env_hostname;
    int rc;
    int mpi_errno = MPI_SUCCESS;
   
    /* --BEGIN ERROR HANDLING-- */
    if (len < 0)
    {
      mpi_errno = -1;
      goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* FIXME: Is this documented?  How does it work if the process manager
       cannot give each process a different value for an environment
       name?  What if a different interface is needed? */
    /* Use hostname supplied in environment variable, if it exists */
    env_hostname = getenv("MPICH_INTERFACE_HOSTNAME");
#if 0
    if (!env_hostname) {
	/* FIXME: Try to get the environment variable that uses the rank 
	   in comm world, i.e., MPICH_INTERFACE_HOSTNAME_R_%d.  For 
	   this, we'll need to know the rank for this process. */
    }
#endif
    if (env_hostname != NULL)
    {
	rc = MPIU_Strncpy(host_description, env_hostname, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc != 0)
	{
	  mpi_errno = -1;
	}
	/* --END ERROR HANDLING-- */
    }
    else {
	rc = gethostname(host_description, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc == -1)
	{
	  mpi_errno = -1;
	}
	/* --END ERROR HANDLING-- */
    }

  fn_exit:
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME my_MPIDI_CH3U_Get_business_card_sock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int my_MPIDI_CH3U_Get_business_card_sock(char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int port;
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    
    mpi_errno = my_MPIDU_Sock_get_host_description(host_description, MAX_HOST_DESCRIPTION_LEN);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_description");
    }

    port = MPIDI_CH3I_listener_port;
    mpi_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, port);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* Look up the interface address cooresponding to this host description */
    /* FIXME: We should start switching to getaddrinfo instead of 
       gethostbyname */
    /* FIXME: We don't make use of the ifname in Windows in order to 
       provide backward compatibility with the (undocumented) host
       description string used by the socket connection routine 
       MPIDU_Sock_post_connect.  We need to change to an interface-address
       (already resolved) based description for better scalability and
       to eliminate reliance on fragile DNS services. Note that this is
       also more scalable, since the DNS server may serialize address 
       requests.  On most systems, asking for the host info of yourself
       is resolved locally (i.e., perfectly parallel).
    */
#ifndef HAVE_WINDOWS_H
    {
	struct hostent *info;
	char ifname[256];
	unsigned char *p;
	info = gethostbyname( host_description );
	if (info && info->h_addr_list) {
	    p = (unsigned char *)(info->h_addr_list[0]);
	    MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", 
			   p[0], p[1], p[2], p[3] );
	    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"ifname = %s",ifname );
	    mpi_errno = MPIU_Str_add_string_arg( bc_val_p, 
						 val_max_sz_p, 
						 MPIDI_CH3I_IFNAME_KEY,
						 ifname );
	    if (mpi_errno != MPIU_STR_SUCCESS) {
		if (mpi_errno == MPIU_STR_NOMEM) {
		    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
		}
		else {
		    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**buscard");
		}
	    }
	}
    }
#endif
 fn_exit:
    
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME my_MPIDU_Sock_get_conninfo_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int my_MPIDU_Sock_get_conninfo_from_bc( const char *bc, 
				     char *host_description, int maxlen,
				     int *port, MPIDU_Sock_ifaddr_t *ifaddr, 
				     int *hasIfaddr )
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno;
#if !defined(HAVE_WINDOWS_H) && defined(HAVE_INET_PTON)
    char ifname[256];
#endif
    
    str_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_HOST_DESCRIPTION_KEY, 
				 host_description, maxlen);
    if (str_errno != MPIU_STR_SUCCESS) {
	/* --BEGIN ERROR HANDLING */
	if (str_errno == MPIU_STR_FAIL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**argstr_missinghost");
	}
	else {
	    /* MPIU_STR_TRUNCATED or MPIU_STR_NONEM */
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
	}
	/* --END ERROR HANDLING-- */
    }
    str_errno = MPIU_Str_get_int_arg(bc, MPIDI_CH3I_PORT_KEY, port);
    if (str_errno != MPIU_STR_SUCCESS) {
	/* --BEGIN ERROR HANDLING */
	if (str_errno == MPIU_STR_FAIL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_missingport");
	}
	else {
	    /* MPIU_STR_TRUNCATED or MPIU_STR_NONEM */
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port");
	}
	/* --END ERROR HANDLING-- */
    }
    /* ifname is optional */
    /* FIXME: This is a hack to allow Windows to continue to use
       the host description string instead of the interface address
       bytes when posting a socket connection.  This should be fixed 
       by changing the Sock_post_connect to only accept interface
       address.  Note also that Windows does not have the inet_pton 
       routine; the Windows version of this routine will need to 
       be identified or written.  See also channels/sock/ch3_progress.c and
       channels/ssm/ch3_progress_connect.c */
    *hasIfaddr = 0;
#if !defined(HAVE_WINDOWS_H) && defined(HAVE_INET_PTON)
    str_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_IFNAME_KEY, 
					ifname, sizeof(ifname) );
    if (str_errno == MPIU_STR_SUCCESS) {
	/* Convert ifname into 4-byte ip address */
	/* Use AF_INET6 for IPv6 (inet_pton may still be used).
	   An address with more than 3 :'s is an IPv6 address */
	
	int rc = inet_pton( AF_INET, (const char *)ifname, ifaddr->ifaddr );
	if (rc == 0) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ifnameinvalid");
	}
	else if (rc < 0) {
	    /* af_inet not supported */
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**afinetinvalid");
	}
	else {
	    /* Success */
	    *hasIfaddr = 1;
	    ifaddr->len = 4;  /* IPv4 address */
	    ifaddr->type = AF_INET;
	}
    }
#endif
    
 fn_exit:
    
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}