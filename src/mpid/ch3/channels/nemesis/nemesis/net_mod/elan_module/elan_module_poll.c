/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include "elan.h"
#include "elan_module.h"
#include "my_papi_defs.h"

/*
void mx__print_queue(MPID_nem_mx_req_queue_ptr_t qhead, int sens)
{
   MPID_nem_mx_cell_ptr_t curr = qhead->head;
   int index = 0;

   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
   
   while(curr != NULL)
     {
	fprintf(stdout,"[%i] -- [CELL %i @%p]: [REQUEST is %i @%p][NEXT @ %p] \n",
                MPID_nem_mem_region.rank,index,curr,curr->mx_request,&(curr->mx_request),curr->next);
	curr = curr->next;
	index++;
     }
   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
}
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_elan_module_send_from_queue()
{
   int mpi_errno = MPI_SUCCESS;
   
   if (MPID_nem_module_elan_pendings_sends > 0)   
     {	
     }
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_elan_module_recv()
{
   int                 mpi_errno = MPI_SUCCESS;

   /*
   if (MPID_nem_module_mx_recv_outstanding_request_num > 0)
     {
     }

   if (MPID_nem_module_mx_recv_outstanding_request_num == 0)
     {	
     }
    */ 
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_poll(MPID_nem_poll_dir_t in_or_out)
{
   int mpi_errno = MPI_SUCCESS;
   
   if (in_or_out == MPID_NEM_POLL_OUT)
     {
	MPID_nem_elan_module_send_from_queue();
	MPID_nem_elan_module_recv();
     }
   else
     {
	MPID_nem_elan_module_recv();
	MPID_nem_elan_module_send_from_queue();
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}