/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_f90_integer */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_f90_integer = PMPI_Type_create_f90_integer
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_f90_integer  MPI_Type_create_f90_integer
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_f90_integer as PMPI_Type_create_f90_integer
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_f90_integer PMPI_Type_create_f90_integer

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of 
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
int MPIR_Type_create_f90_integer_util( int a, MPID_Comm *comm )
{
...
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_f90_integer

/*@
   MPI_Type_create_f90_integer - Return a predefined type that matches 
   the specified range

   Input Arguments:
.  range - Decimal range desired

   Output Arguments:
.  newtype - A predefine MPI Datatype that matches the range.

   Notes:
If there is no corresponding type for the specified range, the call is 
erroneous.  This implementation sets 'newtype' to 'MPI_DATATYPE_NULL' and
returns an error of class 'MPI_ERR_ARG'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_create_f90_integer( MPI_Comm comm, MPI_Datatype dataype, int a ) 
{
    static const char FCNAME[] = "MPI_Type_create_f90_integer";
    int mpi_errno = MPI_SUCCESS;
    static int f90_integer_model = MPIR_F90_INTEGER_MODEL;
    static int f90_integer_map[] = { MPIR_F90_INTEGER_MODEL_MAP, 0, 0 };
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_F90_INTEGER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_F90_INTEGER);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    if (mpi_errno) {
		return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_F90_INTEGER);
    return MPI_SUCCESS;
}
