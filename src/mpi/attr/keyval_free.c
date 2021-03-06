/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Keyval_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Keyval_free  MPI_Keyval_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Keyval_free as PMPI_Keyval_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Keyval_free
#define MPI_Keyval_free PMPI_Keyval_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Keyval_free

/*@

MPI_Keyval_free - Frees an attribute key for communicators

Input Parameter:
. keyval - Frees the integer key value (integer) 

Note:
Key values are global (they can be used with any and all communicators)

.N Deprecated
The replacement for this routine is 'MPI_Comm_free_keyval'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_PERM_KEY

.seealso: MPI_Keyval_create, MPI_Comm_free_keyval
@*/
int MPI_Keyval_free(int *keyval)
{
    static const char FCNAME[] = "MPI_Keyval_free";
    int mpi_errno = MPI_SUCCESS;
    MPIU_THREADPRIV_DECL;                                       \
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_KEYVAL_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("attr");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_KEYVAL_FREE);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(keyval, "keyval", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIU_THREADPRIV_GET;                                        \
    MPIR_Nest_incr();
    mpi_errno = NMPI_Comm_free_keyval( keyval );
    MPIR_Nest_decr();
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit: 
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_FREE);
    MPIU_THREAD_SINGLE_CS_EXIT("attr");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_keyval_free", "**mpi_keyval_free %p", keyval);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
