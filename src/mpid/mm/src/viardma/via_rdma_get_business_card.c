/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "via_rdmaimpl.h"

int via_rdma_get_business_card(char *value, int length)
{
    MPID_STATE_DECLS;
    MPID_FUNC_ENTER(MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD);

    if (length < 1)
    {
	MPID_FUNC_EXIT(MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD);
	return -1;
    }
    snprintf(value, length, "none");

    MPID_FUNC_EXIT(MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
