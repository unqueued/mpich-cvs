/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Register_datarep */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Register_datarep = PMPI_Register_datarep
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Register_datarep  MPI_Register_datarep
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Register_datarep as PMPI_Register_datarep
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Register_datarep PMPI_Register_datarep

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Register_datarep

/*@
   MPI_Register_datarep - register datarep

   Input Parameters:
+ datarep - data representation identifier (string) 
. read_conversion_fn - function invoked to convert from file representation to native representation (function) 
. write_conversion_fn - function invoked to convert from native representation to file representation (function) 
. dtype_file_extent_fn - function invoked to get the extent of a datatype as represented in the file (function) 
- extra_state - extra state 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Register_datarep(char *datarep, MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state)
{
    static const char FCNAME[] = "MPI_Register_datarep";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REGISTER_DATAREP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REGISTER_DATAREP);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REGISTER_DATAREP);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* FIXME UNIMPLEMENTED */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REGISTER_DATAREP);
    return MPI_SUCCESS;
}
