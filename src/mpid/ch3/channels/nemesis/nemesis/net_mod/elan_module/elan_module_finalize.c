/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include "elan.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_finalize()
{
   int mpi_errno = MPI_SUCCESS;
 
    if (MPID_nem_mem_region.ext_procs > 0)
     {
	while(MPID_nem_module_elan_pendings_sends > 0)
	  {
	     MPID_nem_elan_module_poll(MPID_NEM_POLL_OUT);
	  }
	
	elan_disable_network(elan_base->state);
     }
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_ckpt_shutdown ()
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;
}

