/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sctp_module_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_finalize ()
{
    int mpi_errno = MPI_SUCCESS;
    

    mpi_errno = MPID_nem_sctp_module_ckpt_shutdown();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* free associationID -> vc hash table */
    hash_free(MPID_nem_sctp_assocID_table);

    mpi_errno = MPID_nem_sctp_module_send_finalize();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_ckpt_shutdown ()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

