/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 8
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem = {
    0, 0, 0, 0, MPID_REQUEST, sizeof(MPID_Request), MPID_Request_direct,
    MPID_REQUEST_PREALLOC };

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Request_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3_Request_create()
{
#ifdef MPICH_DBG_OUTPUT
    int mpi_errno;
#endif
    MPID_Request * req;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQUEST_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQUEST_CREATE);
    
    req = MPIU_Handle_obj_alloc(&MPID_Request_mem);
    if (req != NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "allocated request, handle=0x%08x",
			  req->handle));
#ifdef MPICH_DBG_OUTPUT
	/*MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);*/
	if (HANDLE_GET_MPI_KIND(req->handle) != MPID_REQUEST)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_handle", "**invalid_handle %d", req->handle);
	    MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
	}
#endif
	MPIDI_CH3U_Request_create(req);
    }
    else
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "unable to allocate a request"));
	req = NULL;
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQUEST_CREATE);
    return req;
}

#if !defined(MPIDI_CH3_Request_add_ref)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Request_add_ref
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Request_add_ref(MPID_Request * req)
{
#ifdef MPICH_DBG_OUTPUT
    int mpi_errno;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF);
#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);*/
    if (HANDLE_GET_MPI_KIND(req->handle) != MPID_REQUEST)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_handle", "**invalid_handle %d", req->handle);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
#endif
    MPIU_Object_add_ref(req);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF);
}
#endif

#if !defined(MPIDI_CH3_Request_release_ref)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Request_release_ref
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Request_release_ref(MPID_Request * req, int * ref_count)
{
#ifdef MPICH_DBG_OUTPUT
    int mpi_errno;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF);
#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);*/
    if (HANDLE_GET_MPI_KIND(req->handle) != MPID_REQUEST)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_handle", "**invalid_handle %d", req->handle);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
#endif
    MPIU_Object_release_ref(req, ref_count);
#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(req->ref_count >= 0);*/
    if (req->ref_count < 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_refcount", "**invalid_refcount %d", req->ref_count);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Request_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Request_destroy(MPID_Request * req)
{
#ifdef MPICH_DBG_OUTPUT
    int mpi_errno;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQUEST_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQUEST_DESTROY);
    MPIDI_DBG_PRINTF((60, FCNAME, "freeing request, handle=0x%08x",
		      req->handle));
#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);*/
    if (HANDLE_GET_MPI_KIND(req->handle) != MPID_REQUEST)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_handle", "**invalid_handle %d", req->handle);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
    /*MPIU_Assert(req->ref_count == 0);*/
    if (req->ref_count != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_refcount", "**invalid_refcount %d", req->ref_count);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
#endif
    MPIDI_CH3U_Request_destroy(req);
    MPIU_Handle_obj_free(&MPID_Request_mem, req);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQUEST_DESTROY);
}
