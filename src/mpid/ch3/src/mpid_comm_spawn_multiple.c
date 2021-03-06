/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: Correct description of function */
/*@
   MPID_Comm_spawn_multiple - 

   Input Arguments:
+  int count - count
.  char *array_of_commands[] - commands
.  char* *array_of_argv[] - arguments
.  int array_of_maxprocs[] - maxprocs
.  MPI_Info array_of_info[] - infos
.  int root - root
-  MPI_Comm comm - communicator

   Output Arguments:
+  MPI_Comm *intercomm - intercommunicator
-  int array_of_errcodes[] - error codes

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_spawn_multiple(int count, char *array_of_commands[], 
			     char ** array_of_argv[], int array_of_maxprocs[],
			     MPID_Info * array_of_info_ptrs[], int root, 
			     MPID_Comm * comm_ptr, MPID_Comm ** intercomm,
			     int array_of_errcodes[]) 
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    /* We allow an empty implementation of this function to 
       simplify building MPICH2 on systems that have difficulty
       supporing process creation */
#   ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
    mpi_errno = MPIDI_Comm_spawn_multiple(count, array_of_commands, 
					  array_of_argv, array_of_maxprocs,
					  array_of_info_ptrs,
					  root, comm_ptr, intercomm, 
					  array_of_errcodes);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }
#   else
    MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER, "**notimpl",
		  "**notimpl %s", FCNAME);
#   endif
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}
