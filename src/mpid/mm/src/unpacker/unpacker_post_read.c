/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

int unpacker_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_ENTER_FUNC(UNPACKER_POST_READ);

    unpacker_car_enqueue(vc_ptr, car_ptr);

    MM_EXIT_FUNC(UNPACKER_POST_READ);
    return MPI_SUCCESS;
}

int unpacker_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    MM_ENTER_FUNC(UNPACKER_MERGE_WITH_UNEXPECTED);

    err_printf("packer_merge_with_unexpected: I thought this function would never be called.\n");

    MM_EXIT_FUNC(UNPACKER_MERGE_WITH_UNEXPECTED);
    return MPI_SUCCESS;
}
