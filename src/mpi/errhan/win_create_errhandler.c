/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create_errhandler = PMPI_Win_create_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create_errhandler  MPI_Win_create_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create_errhandler as PMPI_Win_create_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_create_errhandler
#define MPI_Win_create_errhandler PMPI_Win_create_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_create_errhandler

/*@
   MPI_Win_create_errhandler - Create an error handler for use with MPI window
   objects

   Input Parameter:
. function - user defined error handling procedure (function) 

   Output Parameter:
. errhandler - MPI error handler (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Win_create_errhandler(MPI_Win_errhandler_fn *function, 
			      MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Win_create_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_Errhandler *errhan_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_CREATE_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("errhan");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_CREATE_ERRHANDLER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(function, "function", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(errhandler, "errhandler", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */
    
    errhan_ptr = (MPID_Errhandler *)MPIU_Handle_obj_alloc( &MPID_Errhandler_mem );
    MPIU_ERR_CHKANDJUMP1(!errhan_ptr,mpi_errno,MPI_ERR_OTHER,"**nomem",
			 "**nomem %s", "MPI_Errhandler");
    *errhandler		 = errhan_ptr->handle;
    errhan_ptr->language = MPID_LANG_C;
    errhan_ptr->kind	 = MPID_WIN;
    MPIU_Object_set_ref(errhan_ptr,1);
    errhan_ptr->errfn.C_Win_Handler_function = function;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE_ERRHANDLER);
    MPIU_THREAD_SINGLE_CS_EXIT("errhan");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_create_errhandler", 
	    "**mpi_win_create_errhandler %p %p", function, errhandler);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
