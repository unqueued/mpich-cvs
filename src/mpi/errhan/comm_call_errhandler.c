/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_call_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_call_errhandler = PMPI_Comm_call_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_call_errhandler  MPI_Comm_call_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_call_errhandler as PMPI_Comm_call_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_call_errhandler
#define MPI_Comm_call_errhandler PMPI_Comm_call_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_call_errhandler
#undef FCNAME
#define FCNAME "MPI_Comm_call_errhandler"

/*@
   MPI_Comm_call_errhandler - Call the error handler installed on a 
   communicator

 Input Parameters:
+ comm - communicator with error handler (handle) 
- errorcode - error code (integer) 

 Note:
 Assuming the input parameters are valid, when the error handler is set to
 MPI_ERRORS_RETURN, this routine will always return MPI_SUCCESS.
 
.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    
    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);

    MPIU_THREADPRIV_GET;
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr; if comm_ptr is not value, it will be reset
	       to null */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (comm_ptr->errhandler) {
		MPIR_ERRTEST_ERRHANDLER(comm_ptr->errhandler->handle,mpi_errno);
		if (mpi_errno) goto fn_fail;
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Check for predefined error handlers */
    if (!comm_ptr->errhandler || 
	comm_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	mpi_errno = MPIR_Err_return_comm( comm_ptr, "MPI_Comm_call_errhandler", errorcode );
	goto fn_exit;
    }

    if (comm_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
	/* MPI_ERRORS_RETURN should always return MPI_SUCCESS */
	goto fn_exit;
    }

    /* The user error handler may make calls to MPI routines, so the nesting
     * counter must be incremented before the handler is called */
    MPIR_Nest_incr();
    
    /* Process any user-defined error handling function */
    switch (comm_ptr->errhandler->language) {
    case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
#endif
	(*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
	    &comm_ptr->handle, &errorcode );
	break;
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	(*comm_ptr->errhandler->errfn.F77_Handler_function)( 
	    (MPI_Fint *)&comm_ptr->handle, &errorcode );
	break;
#endif
    }

    MPIR_Nest_decr();
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_comm_call_errhandler",
	    "**mpi_comm_call_errhandler %C %d", comm, errorcode);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
