/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_version */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_version = PMPI_Get_version
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_version  MPI_Get_version
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_version as PMPI_Get_version
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Get_version PMPI_Get_version

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_version

/*@
   MPI_Get_version - Return the version number of MPI

   Output Parameters:
+  version - Version of MPI
-  subversion - Subversion of MPI

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get_version( int *version, int *subversion )
{
    static const char FCNAME[] = "MPI_Get_version";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_VERSION);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_VERSION);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Note that this routine may be called before MPI_Init */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *version    = MPI_VERSION;
    *subversion = MPI_SUBVERSION;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_VERSION);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_get_version", "**mpi_get_version %p %p", version, subversion);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_VERSION);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
