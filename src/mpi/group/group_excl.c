/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_excl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_excl = PMPI_Group_excl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_excl  MPI_Group_excl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_excl as PMPI_Group_excl
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_excl PMPI_Group_excl

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_excl

/*@
   MPI_Group_excl - group_excl

   Arguments:
+  MPI_Group group - group
.  int n - n
.  int *ranks - ranks
-  MPI_Group *newgroup - new group

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_excl";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL, *new_group_ptr;
    int size, i;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_EXCL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_EXCL);
    /* Get handles to MPI objects. */
    MPID_Group_get_ptr( group, group_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */
	    if (group_ptr) {
		mpi_errno = MPIR_Group_check_valid_ranks( group_ptr, 
							  ranks, n );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_EXCL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Allocate a new group and lrank_to_lpid array */
    size = group_ptr->size;
    mpi_errno = MPIR_Group_create( size - n, &new_group_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_EXCL);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    new_group_ptr->rank = MPI_UNDEFINED;
    /* Use flag fields to mark the members to *exclude* .
       Note: except in a THREAD_MULTIPLE case, and if error checking
       is turned on, the flag fields are already set in the error checking
       section.  Because of the low use of this function, this 
       optimization has not been taken. */
    MPID_Common_thread_lock();
    {
	int newi;

	for (i=0; i<size; i++) {
	    group_ptr->lrank_to_lpid[i].flag = 0;
	}
	for (i=0; i<n; i++) {
	    group_ptr->lrank_to_lpid[ranks[i]].flag = 1;
	}
	
	newi = 0;
	for (i=0; i<size; i++) {
	    if (group_ptr->lrank_to_lpid[i].flag == 0) {
		new_group_ptr->lrank_to_lpid[newi].lrank = newi;
		new_group_ptr->lrank_to_lpid[newi].lpid = 
		    group_ptr->lrank_to_lpid[i].lpid;
		if (group_ptr->rank == i) new_group_ptr->rank = newi;
		newi++;
	    }
	}
    }
    MPID_Common_thread_unlock();

    new_group_ptr->size = size - n;
    new_group_ptr->idx_of_first_lpid = -1;

    *newgroup = new_group_ptr->handle;
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_EXCL);

    return MPI_SUCCESS;
}
