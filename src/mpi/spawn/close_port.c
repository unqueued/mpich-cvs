/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Close_port */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Close_port = PMPI_Close_port
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Close_port  MPI_Close_port
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Close_port as PMPI_Close_port
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Close_port PMPI_Close_port

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Close_port

/*@
   MPI_Close_port - close port

   Input Parameter:
.  port_name - a port name (string)

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Close_port(char *port_name)
{
    static const char FCNAME[] = "MPI_Close_port";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CLOSE_PORT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CLOSE_PORT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno)
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Close_port(port_name);
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CLOSE_PORT);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_close_port", "**mpi_close_port %s", port_name);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CLOSE_PORT);
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}
