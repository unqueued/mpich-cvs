/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   unpacker_car_enqueue - enqueue a car in a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int unpacker_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* enqueue the write car in the vc_ptr write queue */
	if (vc_ptr->writeq_tail != NULL)
	    vc_ptr->writeq_tail->mnext_ptr = car_ptr;
	else
	    vc_ptr->writeq_head = car_ptr;
	vc_ptr->writeq_tail = car_ptr;
    }
    else if (car_ptr->type & MM_READ_CAR)
    {
	/* enqueue the read car in the vc_ptr read queue */
	if (vc_ptr->readq_tail != NULL)
	    vc_ptr->readq_tail->mnext_ptr = car_ptr;
	else
	    vc_ptr->readq_head = car_ptr;
	vc_ptr->readq_tail = car_ptr;
    }

    car_ptr->mnext_ptr = NULL;

    return MPI_SUCCESS;
}

/*@
   packer_car_dequeue - dequeue a car from a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int unpacker_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* dequeue the car from the vc_ptr write queue */
	if (vc_ptr->writeq_head == NULL)
	    return MPI_SUCCESS;
	if (vc_ptr->writeq_head == car_ptr)
	{
	    vc_ptr->writeq_head = vc_ptr->writeq_head->mnext_ptr;
	    if (vc_ptr->writeq_head == NULL)
		vc_ptr->writeq_tail = NULL;
	}
	else 
	{
	    iter_ptr = vc_ptr->writeq_head;
	    while (iter_ptr->mnext_ptr)
	    {
		if (iter_ptr->mnext_ptr == car_ptr)
		{
		    if (iter_ptr->mnext_ptr == vc_ptr->writeq_tail)
			vc_ptr->writeq_tail = iter_ptr;
		    iter_ptr->mnext_ptr = iter_ptr->mnext_ptr->mnext_ptr;
		    break;
		}
		iter_ptr = iter_ptr->mnext_ptr;
	    }
	}
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* dequeue the car from the vc_ptr read queue */
	if (vc_ptr->readq_head == NULL)
	    return MPI_SUCCESS;
	if (vc_ptr->readq_head == car_ptr)
	{
	    vc_ptr->readq_head = vc_ptr->readq_head->mnext_ptr;
	    if (vc_ptr->readq_head == NULL)
		vc_ptr->readq_tail = NULL;
	}
	else
	{
	    iter_ptr = vc_ptr->readq_head;
	    while (iter_ptr->mnext_ptr)
	    {
		if (iter_ptr->mnext_ptr == car_ptr)
		{
		    if (iter_ptr->mnext_ptr == vc_ptr->readq_tail)
			vc_ptr->readq_tail = iter_ptr;
		    iter_ptr->mnext_ptr = iter_ptr->mnext_ptr->mnext_ptr;
		    break;
		}
		iter_ptr = iter_ptr->mnext_ptr;
	    }
	}
    }

    car_ptr->mnext_ptr = NULL;

    return MPI_SUCCESS;
}
