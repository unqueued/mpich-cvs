/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_choose_buffer - choose the best buffer for the operation

   Parameters:
+  MPID_Request *request_ptr - request pointer

   Notes:
@*/
int mm_choose_buffer(MPID_Request *request_ptr)
{
    MM_Car *car_ptr;
    /* look at the read car and all of the write cars */
    /* pick the best buffer type that everyone can handle */
    /* if there are incompatible cars, allocate other requests and 
       connect them together with copier methods */
    /* pat your head and rub your tummy */

    /* but for now always choose the vector buffer type */
    if (request_ptr->mm.rcar[0].type != MM_NULL_CAR)
    {
	/* Should this code be moved into a function like vec_buffer_init? */
	/* This would lead nicely into shm_buffer_init and via_buffer_init */
	request_ptr->mm.buf.type = MM_VEC_BUFFER;
	request_ptr->mm.buf.vec.vec_size = MPID_VECTOR_LIMIT;
	request_ptr->mm.buf.vec.msg_size = 0;
	request_ptr->mm.buf.vec.num_read = 0;
	request_ptr->mm.buf.vec.min_num_written = 0;
	request_ptr->mm.buf.vec.first = 0;
	request_ptr->mm.buf.vec.last = 0;
	request_ptr->mm.buf.vec.num_cars_outstanding = 0;
	request_ptr->mm.buf.vec.num_cars = 0;
	/* count the data cars */
	car_ptr = request_ptr->mm.wcar;
	while (car_ptr)
	{
	    if (!(car_ptr->type & MM_HEAD_CAR))
		request_ptr->mm.buf.vec.num_cars++;
	    car_ptr = car_ptr->opnext_ptr;
	}

	request_ptr->mm.get_buffers = mm_get_buffers_vec;
    }
    else
    {
	request_ptr->mm.buf.type = MM_NULL_BUFFER;
	request_ptr->mm.get_buffers = NULL;
    }

    return MPI_SUCCESS;
}
