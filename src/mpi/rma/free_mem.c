/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Free_mem */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Free_mem = PMPI_Free_mem
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Free_mem  MPI_Free_mem
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Free_mem as PMPI_Free_mem
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Free_mem
#define MPI_Free_mem PMPI_Free_mem

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Free_mem

/*@
   MPI_Free_mem - Free memory allocated with MPI_Alloc_mem

   Input Parameter:
.  base - initial address of memory segment allocated by 'MPI_ALLOC_MEM' 
       (choice) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Free_mem(void *base)
{
#if 0
    static const char FCNAME[] = "MPI_Free_mem";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FREE_MEM);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("rma");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FREE_MEM);

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Free_mem(base);

    /* ... end of body of routine ... */

#if 0    
  fn_exit:
#endif    
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FREE_MEM);
    MPIU_THREAD_SINGLE_CS_EXIT("rma");
    return mpi_errno;

    /* There should never be any fn_fail case; this suppresses warnings from
       compilers that object to unused labels */
#if 0 
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_free_mem", "**mpi_free_mem %p", base);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif
}
