/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

/*@
   packer_make_progress - make progress

   Notes:
@*/
int packer_make_progress()
{
    MM_Car *car_ptr, *car_tmp_ptr;
    MM_Segment_buffer *buf_ptr;
    BOOL finished;
    MM_Car *next_qhead;

    /* assign car_ptr to the first non-empty qhead, either
     * readq_head or writeq_head.  Then set next_qhead to
     * the other available qhead or NULL if empty
     */
    car_ptr = MPID_Process.packer_vc_ptr->readq_head;
    if (car_ptr == NULL)
    {
	if (MPID_Process.packer_vc_ptr->writeq_head == NULL)
	{
	    /* shortcut out if the queues are empty */
	    return MPI_SUCCESS;
	}
	car_ptr = MPID_Process.packer_vc_ptr->writeq_head;
	next_qhead = NULL;
    }
    else
    {
	next_qhead = MPID_Process.packer_vc_ptr->writeq_head;
    }

    /* Process either or both qheads */
    do
    {
	while (car_ptr)
	{
	    finished = FALSE;
	    buf_ptr = car_ptr->buf_ptr;
	    switch (buf_ptr->type)
	    {
	    case MM_NULL_BUFFER:
		err_printf("error, cannot pack from a null buffer\n");
		break;
	    case MM_TMP_BUFFER:
		if (buf_ptr->tmp.buf == NULL)
		{
		    /* the buffer is empty so get a tmp buffer */
		    car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
		    /* set the first variable to zero */
		    car_ptr->data.packer.first = 0;
		}
		/* set the last variable to the end of the segment */
		car_ptr->data.packer.last = buf_ptr->tmp.len;
		/* pack the buffer */
		MPID_Segment_pack(
		    &car_ptr->request_ptr->mm.segment,
		    car_ptr->data.packer.first,
		    &car_ptr->data.packer.last,
		    car_ptr->request_ptr->mm.buf.tmp.buf
		    );
		/* update the number of bytes read */
		buf_ptr->tmp.num_read += (car_ptr->data.packer.last - car_ptr->data.packer.first);

		/* if the entire buffer is packed then break */
		if (car_ptr->data.packer.last == car_ptr->request_ptr->mm.last)
		{
		    finished = TRUE;
		    break;
		}
		/* otherwise there is more packing needed so update the first variable */
		/* The last variable will be updated the next time through this function */
		car_ptr->data.packer.first = car_ptr->data.packer.last;
		break;
	    case MM_VEC_BUFFER:
		if (buf_ptr->vec.num_cars_outstanding == 0)
		{
		    car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
		    buf_ptr->vec.num_read = buf_ptr->vec.last - buf_ptr->vec.first;
		    buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
		}
		if (buf_ptr->vec.last == car_ptr->request_ptr->mm.last)
		    finished = TRUE;
		break;
#ifdef WITH_METHOD_SHM
	    case MM_SHM_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_VIA
	    case MM_VIA_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	    case MM_VIA_RDMA_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_NEW
	    case MM_NEW_METHOD_BUFFER:
		break;
#endif
	    default:
		err_printf("illegal buffer type: %d\n", car_ptr->request_ptr->mm.buf.type);
		break;
	    }
	    if (finished)
	    {
		car_tmp_ptr = car_ptr;
		car_ptr = car_ptr->qnext_ptr;
		packer_car_dequeue(MPID_Process.packer_vc_ptr, car_tmp_ptr);
		mm_cq_enqueue(car_tmp_ptr);
	    }
	    else
	    {
		car_ptr = car_ptr->qnext_ptr;
	    }
	}

	car_ptr = next_qhead;
	next_qhead = NULL;
    } while (car_ptr);

    return MPI_SUCCESS;
}
