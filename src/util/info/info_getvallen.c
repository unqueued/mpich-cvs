/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_valuelen */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_valuelen = PMPI_Info_get_valuelen
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_valuelen  MPI_Info_get_valuelen
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_valuelen as PMPI_Info_get_valuelen
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_get_valuelen
#define MPI_Info_get_valuelen PMPI_Info_get_valuelen
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get_valuelen

/*@
    MPI_Info_get_valuelen - Retrieves the length of the value associated with 
    a key

    Input Parameters:
+ info - info object (handle)
- key - key (string)

    Output Parameters:
+ valuelen - length of value argument (integer)
- flag - true if key defined, false if not (boolean)

.N ThreadSafeInfoRead

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO_KEY
.N MPI_ERR_OTHER
@*/
int MPI_Info_get_valuelen( MPI_Info info, char *key, int *valuelen, int *flag )
{
    MPID_Info *curr_ptr, *info_ptr=0;
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Info_get_valuelen";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET_VALUELEN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("info");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET_VALUELEN);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INFO(info, mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Info_get_ptr( info, info_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int keylen;

            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
	    
	    /* Check key */
	    MPIU_ERR_CHKANDJUMP((!key), mpi_errno, MPI_ERR_INFO_KEY, 
				"**infokeynull");
	    keylen = (int)strlen(key);
	    MPIU_ERR_CHKANDJUMP((keylen > MPI_MAX_INFO_KEY), mpi_errno, 
				MPI_ERR_INFO_KEY, "**infokeylong");
	    MPIU_ERR_CHKANDJUMP((keylen == 0), mpi_errno, MPI_ERR_INFO_KEY, 
				"**infokeyempty");

	    MPIR_ERRTEST_ARGNULL(valuelen, "valuelen", mpi_errno);
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
	if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
	    *valuelen = (int)strlen(curr_ptr->value);
	    *flag = 1;
	    break;
	}
	curr_ptr = curr_ptr->next;
    }
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_VALUELEN);
    MPIU_THREAD_SINGLE_CS_EXIT("info");
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_info_get_valuelen",
	    "**mpi_info_get_valuelen %I %s %p %p", info, key, valuelen, flag);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#   endif
}
