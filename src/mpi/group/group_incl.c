/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_incl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_incl = PMPI_Group_incl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_incl  MPI_Group_incl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_incl as PMPI_Group_incl
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_incl PMPI_Group_incl

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_incl

/*@

MPI_Group_incl - Produces a group by reordering an existing group and taking
        only listed members

Input Parameters:
+ group - group (handle) 
. n - number of elements in array 'ranks' (and size of newgroup ) (integer) 
- ranks - ranks of processes in 'group' to appear in 'newgroup' (array of 
integers) 

Output Parameter:
. newgroup - new group derived from above, in the order defined by 'ranks' 
(handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_ARG
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_RANK

.seealso: MPI_Group_free
@*/
int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_incl";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL, *new_group_ptr = NULL;
    int i;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_INCL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_INCL);
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
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (n == 0) {
	*newgroup = MPI_GROUP_EMPTY;
	return MPI_SUCCESS;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create( n, &new_group_ptr );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    new_group_ptr->rank = MPI_UNDEFINED;
    for (i=0; i<n; i++)
    {
	new_group_ptr->lrank_to_lpid[i].lrank = i;
	new_group_ptr->lrank_to_lpid[i].lpid  = 
	    group_ptr->lrank_to_lpid[ranks[i]].lpid;
	if (ranks[i] == group_ptr->rank) new_group_ptr->rank = i;
    }
    new_group_ptr->size = n;
    new_group_ptr->idx_of_first_lpid = -1;

    *newgroup = new_group_ptr->handle;
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_INCL);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_group_incl", "**mpi_group_incl %G %d %p %p", group, n, ranks, newgroup);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_INCL);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

