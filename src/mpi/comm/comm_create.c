/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create = PMPI_Comm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create  MPI_Comm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create as PMPI_Comm_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_create PMPI_Comm_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create

/*@
   MPI_Comm_create - create a new communicator

   Arguments:
+  MPI_Comm comm - communicator
.  MPI_Group group - group
-  MPI_Comm *newcomm - new communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    static const char FCNAME[] = "MPI_Comm_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int i, j, n, *mapping = 0, new_context_id;
    MPID_Comm *newcomm_ptr;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Group_get_ptr( group, group_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    /* Check the group ptr */
	    MPID_Group_valid_ptr( group_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Create a new communicator from the specified group members */

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id. 
       Creating the context id is collective over the *input* communicator,
       so it must be created before we decide if this process is a 
       member of the group */
    new_context_id = MPIR_Get_contextid( comm_ptr->handle );
    if (new_context_id == 0) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**toomanycomm", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    
    /* Make sure that the processes for this group are contained within
       the input communicator.  Also identify the mapping from the ranks of 
       the old communicator to the new communicator.
       We do this by matching the lpids of the members of the group
       with the lpids of the members of the input communicator.
       It is an error if the group contains a reference to an lpid that 
       does not exist in the communicator.
       
       An important special case is groups (and communicators) that
       are subsets of MPI_COMM_WORLD.  This this case, the lpids are
       exactly the same as the ranks in comm world.  Currently, we
       don't take this into account, but if the code to handle the general 
       case is too messy, we'll add this in.
    */
    if (group_ptr->rank != MPI_UNDEFINED) {
	n = group_ptr->size;
	/*printf( "group size = %d comm size = %d\n", n, 
	  comm_ptr->remote_size ); */
	mapping = (int *)MPIU_Malloc( n * sizeof(int) );
	if (!mapping) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE );
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	for (i=0; i<n; i++) {
	    /* Mapping[i] is the rank in the communicator of the process that
	       is the ith element of the group */
	    /* FIXME - BUBBLE SORT */
	    /* FIXME - NEEDS COMM_WORLD SPECIALIZATION */
	    mapping[i] = -1;
	    for (j=0; j<comm_ptr->remote_size; j++) {
		int comm_lpid;
		MPID_VCR_Get_lpid( comm_ptr->vcr[j], &comm_lpid );
		/*printf( "commlpid = %d, group[%d]lpid = %d\n",
		  comm_lpid, i, group_ptr->lrank_to_lpid[i].lpid ); */
		if (comm_lpid == group_ptr->lrank_to_lpid[i].lpid) {
		    mapping[i] = j;
		    break;
		}
	    }
	    if (mapping[i] == -1) {
		/*printf( "failed for %d entry (pid=%d)\n", i,
		  group_ptr->lrank_to_lpid[i].lpid); */
		mpi_errno = MPIR_Err_create_code( MPI_ERR_GROUP, 
						  "**groupnotincomm", 
						  "**groupnotincomm %d", i );
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE );
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	}

	/* Get the new communicator structure and context id */
	newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
	if (!newcomm_ptr) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	MPIU_Object_set_ref( newcomm_ptr, 1 );
	newcomm_ptr->attributes  = 0;
	newcomm_ptr->context_id  = new_context_id;
	newcomm_ptr->remote_size = newcomm_ptr->local_size = n;
	newcomm_ptr->rank        = group_ptr->rank;
	newcomm_ptr->comm_kind   = MPID_INTRACOMM;
	/* Since the group has been provided, let the new communicator know
	   about the group */
	newcomm_ptr->local_group  = group_ptr;
	newcomm_ptr->remote_group = group_ptr;
	MPIU_Object_add_ref( group_ptr );
	MPIU_Object_add_ref( group_ptr );

	newcomm_ptr->coll_fns = 0;

	/* Setup the communicator's vc table */
	MPID_VCRT_Create( n, &newcomm_ptr->vcrt );
	MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
	for (i=0; i<n; i++) {
	    /* For rank i in the new communicator, find the corresponding
	       rank in the input communicator */
	    MPID_VCR_Dup( comm_ptr->vcr[mapping[i]], &newcomm_ptr->vcr[i] );
	    
	    /* printf( "[%d] mapping[%d] = %d\n", comm_ptr->rank, i, mapping[i] ); */
	}

	/* Notify the device of this new communicator */
	/*printf( "about to notify device\n" ); */
	MPID_Dev_comm_create_hook( newcomm_ptr );
	/*printf( "about to return from comm_create\n" ); */
	
	*newcomm = newcomm_ptr->handle;
    }
    else {
	/* This process is not in the group */
	*newcomm = MPI_COMM_NULL;
    }
    /* ... end of body of routine ... */

    /* mpi_errno = MPID_Comm_create(); */
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
	return MPI_SUCCESS;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}

