/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Issend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Issend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Issend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_Request * sreq;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ISSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ISSEND);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                 "rank=%d, tag=%d, context=%d", 
                 rank, tag, comm->context_id + context_offset));
    
    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_SSEND, &sreq);
	goto fn_exit;
    }
    
    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SSEND);
    
    if (rank == MPI_PROC_NULL)
    {
	MPIU_Object_set_ref(sreq, 1);
	sreq->cc = 0;
	goto fn_exit;
    }

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    
    MPIDI_Comm_get_vc(comm, rank, &vc);
    
    if (data_sz == 0)
    {
	mpi_errno = MPIDI_CH3_EagerSyncZero( &sreq, rank, tag, comm, 
					     context_offset );
	goto fn_exit;
    }
    
    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_sync_send_t) <= vc->eager_max_msg_sz)
    {
	mpi_errno = MPIDI_CH3_EagerSyncNoncontigSend( &sreq, buf, count,
                                                      datatype, data_sz, 
                                                      dt_contig, dt_true_lb,
                                                      rank, tag, comm, 
                                                      context_offset );
	/* If we're not complete, then add a reference to the datatype */
	if (sreq && sreq->dev.OnDataAvail) {
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPID_Datatype_add_ref(dt_ptr);
	}
    }
    else
    {
	/* Note that the sreq was created above */
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_RNDV_MSG);
	mpi_errno = vc->rndvSend_fn( &sreq, buf, count, datatype, dt_contig,
                                     data_sz, dt_true_lb, rank, tag, comm, 
                                     context_offset );
	
	/* FIXME: fill temporary IOV or pack temporary buffer after send to 
	   hide some latency.  This requires synchronization
           because the CTS packet could arrive and be processed before the 
	   above iStartmsg completes (depending on the progress
           engine, threads, etc.). */
	
	if (sreq && dt_ptr != NULL)
	{
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPID_Datatype_add_ref(dt_ptr);
	}
    }

  fn_exit:
    *request = sreq;
    
    MPIU_DBG_STMT(CH3_OTHER,VERBOSE,
    {
	if (sreq != NULL) {
	    MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
			   "request allocated, handle=0x%08x", sreq->handle);
	}
    }
		  )
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ISSEND);
    return mpi_errno;
}
