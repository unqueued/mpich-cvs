/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_resized */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_resized = PMPI_Type_create_resized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_resized  MPI_Type_create_resized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_resized as PMPI_Type_create_resized
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_resized PMPI_Type_create_resized

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_resized

/*@
   MPI_Type_create_resized - create resized datatype

   Input Parameters:
+ oldtype - input datatype (handle) 
. lb - new lower bound of datatype (integer) 
- extent - new extent of datatype (integer) 

   Output Parameter:
. newtype - output datatype (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_resized";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_Datatype *new_dtp;
    MPI_Aint aints[2];
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_RESIZED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_RESIZED);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_create_resized(oldtype, lb, extent, newtype);
    if (mpi_errno != MPI_SUCCESS)
	goto fn_fail;

    aints[0] = lb;
    aints[1] = extent;

    MPID_Datatype_get_ptr(*newtype, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_RESIZED,
				           0,
				           2, /* Aints */
				           1,
				           NULL,
				           aints,
				           &oldtype);

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_RESIZED);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_type_create_resized", "**mpi_type_create_resized %D %d %d %p", oldtype, lb, extent, newtype);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_RESIZED);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}

