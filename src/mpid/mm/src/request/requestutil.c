/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 8
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem = { 0, 0, 0, 0, 0,
					 sizeof(MPID_Request), MPID_Request_direct,
					 MPID_REQUEST_PREALLOC, };

MPID_Request * mm_request_alloc()
{
    MPID_Request *p;

    MM_ENTER_FUNC(MM_REQUEST_ALLOC);

    p = MPIU_Handle_obj_alloc(&MPID_Request_mem);
    if (p == NULL)
    {
	MM_EXIT_FUNC(MM_REQUEST_ALLOC);
	return p;
    }
    p->cc = 0;
    p->cc_ptr = &p->cc;
    p->mm.rcar[0].freeme = FALSE;
    p->mm.rcar[1].freeme = FALSE;
    p->mm.wcar[0].freeme = FALSE;
    p->mm.wcar[1].freeme = FALSE;
    p->mm.next_ptr = NULL;

    MM_EXIT_FUNC(MM_REQUEST_ALLOC);
    return p;
}

void mm_request_free(MPID_Request *request_ptr)
{
    MM_ENTER_FUNC(MM_REQUEST_FREE);

    /* free the buffer attached to this request */
    if (request_ptr->mm.release_buffers)
	request_ptr->mm.release_buffers(request_ptr);

    /* free the request */
    MPIU_Handle_obj_free(&MPID_Request_mem, request_ptr);

    MM_EXIT_FUNC(MM_REQUEST_FREE);
}

void MPID_Request_free(MPID_Request *request_ptr)
{
    MM_ENTER_FUNC(MPID_REQUEST_FREE);

    if (request_ptr == NULL)
    {
	MM_EXIT_FUNC(MPID_REQUEST_FREE);
	return;
    }

    MPID_Request_free(request_ptr->mm.next_ptr);
    mm_request_free(request_ptr);

    MM_EXIT_FUNC(MPID_REQUEST_FREE);
}
