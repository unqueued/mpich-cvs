/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* #define DEBUG(a) a;fflush(stdout)  */
#define DEBUG(a) 

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_create = PMPI_Intercomm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_create  MPI_Intercomm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_create as PMPI_Intercomm_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Intercomm_create PMPI_Intercomm_create

/* 128 allows us to handle up to 4k processes */
#ifdef HAVE_ERROR_CHECKING
#define MAX_LPID32_ARRAY 128
PMPI_LOCAL int MPIR_CheckDisjointLpids( int [], int, int [], int );
PMPI_LOCAL int MPIR_CheckDisjointLpids( int lpids1[], int n1, 
					 int lpids2[], int n2 )
{
    static const char FCNAME[] = "MPIR_CheckDisjointLpids";
    int i, maxi, idx, bit, maxlpid = -1;
    int mpi_errno;
    int32_t lpidmask[MAX_LPID32_ARRAY];

    /* Find the max lpid */
    for (i=0; i<n1; i++) {
	if (lpids1[i] > maxlpid) maxlpid = lpids1[i];
    }
    for (i=0; i<n2; i++) {
	if (lpids2[i] > maxlpid) maxlpid = lpids2[i];
    }
    /* --BEGIN ERROR HANDLING-- */
    if (maxlpid >= MAX_LPID32_ARRAY * 32) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**intern",
				  "**intern %s", 
				  "Too many processes in intercomm_create" );
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    /* Compute the max index and zero the pids array */
    maxi = (maxlpid + 31) / 32;
    for (i=0; i<maxi; i++) lpidmask[i] = 0;

    /* Set the bits for the first array */
    for (i=0; i<n1; i++) {
	idx = lpids1[i] / 32;
	bit = lpids1[i] % 32;
	lpidmask[idx] = lpidmask[idx] | (1 << bit);
    }    

    /* Look for any duplicates in the second array */
    for (i=0; i<n2; i++) {
	idx = lpids2[i] / 32;
	bit = lpids2[i] % 32;
	/* --BEGIN ERROR HANDLING-- */
	if (lpidmask[idx] & (1 << bit)) {
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
		       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, 
					      "**dupprocesses", 
					      "**dupprocesses %d", lpids2[i] );
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
	/* Add a check on duplicates *within* group 2 */
	lpidmask[idx] = lpidmask[idx] | (1 << bit);
    }
    
    return 0;
}
#endif /* HAVE_ERROR_CHECKING */

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_create

/*@

MPI_Intercomm_create - Creates an intercommuncator from two intracommunicators

Input Parameters:
+ local_comm - Local (intra)communicator
. local_leader - Rank in local_comm of leader (often 0)
. peer_comm - Communicator used to communicate between a 
              designated process in the other communicator.  
              Significant only at the process in 'local_comm' with
	      rank 'local_leader'.
. remote_leader - Rank in peer_comm of remote leader (often 0)
- tag - Message tag to use in constructing intercommunicator; if multiple
  'MPI_Intercomm_creates' are being made, they should use different tags (more
  precisely, ensure that the local and remote leaders are using different
  tags for each 'MPI_intercomm_create').

Output Parameter:
. comm_out - Created intercommunicator

Notes:
   'peer_comm' is significant only for the process designated the 
   'local_leader' in the 'local_comm'.

  The MPI 1.1 Standard contains two mutually exclusive comments on the
  input intracommunicators.  One says that their repective groups must be
  disjoint; the other that the leaders can be the same process.  After
  some discussion by the MPI Forum, it has been decided that the groups must
  be disjoint.  Note that the `reason` given for this in the standard is
  `not` the reason for this choice; rather, the `other` operations on 
  intercommunicators (like 'MPI_Intercomm_merge') do not make sense if the
  groups are not disjoint.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TAG
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_RANK

.seealso: MPI_Intercomm_merge, MPI_Comm_free, MPI_Comm_remote_group, 
          MPI_Comm_remote_size

@*/
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
			 MPI_Comm peer_comm, int remote_leader, int tag, 
			 MPI_Comm *newintercomm)
{
    static const char FCNAME[] = "MPI_Intercomm_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *peer_comm_ptr = NULL;
    int context_id, final_context_id;
    int remote_size, *remote_lpids=0;
    int local_size, *local_lpids;
    int comm_info[3];
    int is_low_group = 0;
    int i;
    MPID_Comm *newcomm_ptr, *commworld_ptr;
    MPIU_CHKLMEM_DECL(2);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INTERCOMM_CREATE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INTERCOMM_CREATE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( local_comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    if (comm_ptr) {
		/*  Only check if comm_ptr valid */
		MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno );
		if ((local_leader < 0 || 
		     local_leader >= comm_ptr->local_size)) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_RANK, 
					  "**ranklocal", 
					  "**ranklocal %d %d", 
					  local_leader, comm_ptr->local_size );
		}
	    }
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /*
     * Error checking for this routine requires care.  Because this
     * routine is collective over two different sets of processes,
     * it is relatively easy for the user to try to create an 
     * intercommunicator from two overlapping groups of processes.
     * This is made more likely by inconsistencies in the MPI-1
     * specification (clarified in MPI-2) that seemed to allow
     * the groups to overlap.  Because of that, we first check that the
     * groups are in fact disjoint before performing any collective 
     * operations.  
     */

    if (comm_ptr->rank == local_leader) {

	MPID_Comm_get_ptr( peer_comm, peer_comm_ptr );
#       ifdef HAVE_ERROR_CHECKING
	{
	    MPID_BEGIN_ERROR_CHECKS;
	    {
		MPID_Comm_valid_ptr( peer_comm_ptr, mpi_errno );
		/* FIXME: In MPI 1.0, peer_comm was restricted to 
		   intracommunicators.  In 1.1, it may be any communicator */
		/* peer comm must be an intracommunicator */
/*		
	        if( peer_comm_ptr) {
		    MPIR_ERRTEST_COMM_INTRA(peer_comm_ptr, mpi_errno );
		}
*/
		/* In checking the rank of the remote leader, 
		   allow the peer_comm to be in intercommunicator
		   by checking against the remote size */
		if (!mpi_errno && peer_comm_ptr && 
		    (remote_leader < 0 || 
		     remote_leader >= peer_comm_ptr->remote_size)) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_RANK, 
						      "**rankremote", 
					  "**rankremote %d %d", 
					  local_leader, comm_ptr->local_size );
		}
		/* Check that the local leader and the remote leader are 
		   different processes.  This test requires looking at
		   the lpid for the two ranks in their respective 
		   communicators */
/*		if (local_leader == remote_leader) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_RANK,
						      "**ranksdistinct", 0 );
						      }*/
		if (mpi_errno) goto fn_fail;
	    }
	    MPID_END_ERROR_CHECKS;
	}
#       endif /* HAVE_ERROR_CHECKING */
	
	MPIR_Nest_incr();
	/* First, exchange the group information.  If we were certain
	   that the groups were disjoint, we could exchange possible 
	   context ids at the same time, saving one communication.
	   But experience has shown that that is a risky assumption.
	*/
	/* Exchange information with my peer.  Use sendrecv */
	local_size = comm_ptr->local_size;

	DEBUG(printf( "rank %d sendrecv to rank %d\n", peer_comm_ptr->rank, remote_leader ));
	mpi_errno = NMPI_Sendrecv( &local_size,  1, MPI_INT, 
				   remote_leader, tag,
				   &remote_size, 1, MPI_INT, 
				   remote_leader, tag, 
				   peer_comm, MPI_STATUS_IGNORE );

	DEBUG(printf( "local size = %d, remote size = %d\n", local_size, 
		      remote_size ));
	/* With this information, we can now send and receive the 
	   local process ids from the peer.  This works only
	   for local process ids within MPI_COMM_WORLD, so this
	   will need to be fixed for the MPI2 version - FIXME */
	
	remote_size  = remote_size;
	MPIU_CHKLMEM_MALLOC(remote_lpids,int*,remote_size*sizeof(int),
			    mpi_errno,"remote_lpids");
	MPIU_CHKLMEM_MALLOC(local_lpids,int*,local_size*sizeof(int),
			    mpi_errno,"local_lpids");

	for (i=0; i<comm_ptr->local_size; i++) {
	    (void)MPID_VCR_Get_lpid( comm_ptr->vcr[i], &local_lpids[i] );
	}
	
	/* Exchange the lpid arrays */
	NMPI_Sendrecv( local_lpids, local_size, MPI_INT, 
		       remote_leader, tag,
		       remote_lpids, remote_size, MPI_INT, 
		       remote_leader, tag, peer_comm, MPI_STATUS_IGNORE );

#       ifdef HAVE_ERROR_CHECKING
	{
	    MPID_BEGIN_ERROR_CHECKS;
	    {
		/* Now that we have both the local and remote processes,
		   check for any overlap */
		mpi_errno = MPIR_CheckDisjointLpids( local_lpids, local_size,
						   remote_lpids, remote_size );
		if (mpi_errno)
		{
		    MPIR_Nest_decr();
		    goto fn_fail;
		}	    
	    }
	    MPID_END_ERROR_CHECKS;
	}
#       endif /* HAVE_ERROR_CHECKING */
	
	/* Make an arbitrary decision about which group of processs is
	   the low group.  The LEADERS do this by comparing the
	   local process ids of the 0th member of the two groups */
	is_low_group = local_lpids[0] < remote_lpids[0];

	/* At this point, we're done with the local lpids; they'll
	   be freed with the other local memory on exit */
    } /* End of the first phase of the leader communication */

    /* 
     * Create the contexts.  Each group will have a context for sending 
     * to the other group. All processes must be involved.  Because 
     * we know that the local and remote groups are disjoint, this 
     * step will complete 
     */
    DEBUG(printf( "About to get contextid (commsize=%d) on %d\n",
		  comm_ptr->local_size, comm_ptr->rank ));
    context_id = MPIR_Get_contextid( local_comm );
    /* --BEGIN ERROR HANDLING-- */
    if (context_id == 0)
    {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**toomanycomm", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    DEBUG(printf( "Got contextid\n" ));
    /* Leaders can now swap context ids and then broadcast the value
       to the local group of processes */
    if (comm_ptr->rank == local_leader) {
	int remote_context_id;

	NMPI_Sendrecv( &context_id, 1, MPI_INT, remote_leader, tag,
		       &remote_context_id, 1, MPI_INT, remote_leader, tag, 
		       peer_comm, MPI_STATUS_IGNORE );
	
	/* We need to do something with the context ids.  For 
	   MPI1, we can just take the min of the two context ids and
	   use that value.  For MPI2, we'll need to have separate
	   send and receive context ids - FIXME */
	if (remote_context_id < context_id) 
	    final_context_id = remote_context_id;
	else 
	    final_context_id = context_id;

	/* Now, send all of our local processes the remote_lpids, 
	   along with the final context id */
	comm_info[0] = remote_size;
	comm_info[1] = final_context_id;
	comm_info[2] = is_low_group;
	DEBUG(printf ("About to bcast on local_comm\n"));
	NMPI_Bcast( comm_info, 3, MPI_INT, local_leader, local_comm );
	NMPI_Bcast( remote_lpids, remote_size, MPI_INT, local_leader, 
		    local_comm );
	DEBUG(printf( "end of bcast on local_comm of size %d\n", 
		      comm_ptr->local_size ));
    }
    else
    {
	/* were the other processes */
	DEBUG(printf ("About to receive bcast on local_comm\n"));
	NMPI_Bcast( comm_info, 3, MPI_INT, local_leader, local_comm );
	remote_size = comm_info[0];
	MPIU_CHKLMEM_MALLOC(remote_lpids,int*,remote_size*sizeof(int),
			    mpi_errno,"remote_lpids");
	NMPI_Bcast( remote_lpids, remote_size, MPI_INT, local_leader, 
		    local_comm );
	final_context_id = comm_info[1];
	is_low_group     = comm_info[2];
    }

    /* If we did not choose this context, free it.  We won't do this
       once we have MPI2 intercomms (at least, not for intercomms that
       are not subsets of MPI_COMM_WORLD) - FIXME */
    if (final_context_id != context_id) {
	MPIR_Free_contextid( context_id );
    }

    /* At last, we now have the information that we need to build the 
       intercommunicator */

    /* All processes in the local_comm now build the communicator */

    newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    /* --BEGIN ERROR HANDLING-- */
    if (!newcomm_ptr)
    {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPIU_Object_set_ref( newcomm_ptr, 1 );
    newcomm_ptr->attributes   = 0;
    newcomm_ptr->context_id   = final_context_id;
    newcomm_ptr->remote_size  = remote_size;
    newcomm_ptr->local_size   = comm_ptr->local_size;
    newcomm_ptr->rank         = comm_ptr->rank;
    newcomm_ptr->local_group  = 0;
    newcomm_ptr->remote_group = 0;
    newcomm_ptr->comm_kind    = MPID_INTERCOMM;
    newcomm_ptr->local_comm   = 0;
    newcomm_ptr->is_low_group = is_low_group;

    /* FIXME: for MPI1, all process ids are relative to MPI_COMM_WORLD.
       For MPI2, we'll need to do something more complex */
    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( remote_size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<remote_size; i++) {
	/* For rank i in the new communicator, find the corresponding
	   rank in the comm world (FIXME FOR MPI2) */
	/* printf( "[%d] Remote rank %d has lpid %d\n", 
	   MPIR_Process.comm_world->rank, i, remote_lpids[i] ); */
	MPID_VCR_Dup( commworld_ptr->vcr[remote_lpids[i]], 
		      &newcomm_ptr->vcr[i] );
	}

    /* Setup the communicator's vc table: local group.  This is
     just a duplicate of the local_comm's group */
    MPID_VCRT_Create( comm_ptr->local_size, &newcomm_ptr->local_vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->local_vcrt, &newcomm_ptr->local_vcr );
    for (i=0; i<comm_ptr->local_size; i++) {
	MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->local_vcr[i] );
    }

    /* Inherit the error handler (if any) */
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
	MPIU_Object_add_ref( comm_ptr->errhandler );
    }
	
    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook( newcomm_ptr );
    
    *newintercomm = newcomm_ptr->handle;

    MPIU_CHKLMEM_FREEALL;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    MPIU_CHKLMEM_FREEALL;
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_intercomm_create", "**mpi_intercomm_create %C %d %C %d %d %p", local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
