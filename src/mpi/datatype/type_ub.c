/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_ub */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_ub = PMPI_Type_ub
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_ub  MPI_Type_ub
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_ub as PMPI_Type_ub
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_ub
#define MPI_Type_ub PMPI_Type_ub

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_ub
#undef FCNAME
#define FCNAME "MPI_Type_ub"

/*@
    MPI_Type_ub - Returns the upper bound of a datatype

Input Parameters:
. datatype - datatype (handle) 

Output Parameter:
. displacement - displacement of upper bound from origin, 
                             in bytes (integer) 

.N Deprecated
The replacement for this routine is 'MPI_Type_get_extent'

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_UB);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_UB);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
	*displacement = MPID_Datatype_get_basic_size(datatype);
    else
	*displacement = datatype_ptr->ub;

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_UB);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_type_ub",
	    "**mpi_type_ub %D %p", datatype, displacement);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
