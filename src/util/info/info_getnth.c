/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_nthkey */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_nthkey = PMPI_Info_get_nthkey
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_nthkey  MPI_Info_get_nthkey
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_nthkey as PMPI_Info_get_nthkey
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_get_nthkey
#define MPI_Info_get_nthkey PMPI_Info_get_nthkey
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get_nthkey

/*@
    MPI_Info_get_nthkey - Returns the nth defined key in info

Input Parameters:
+ info - info object (handle)
- n - key number (integer)

Output Parameters:
. keys - key (string).  The maximum number of characters is 'MPI_MAX_INFO_KEY'.

.N ThreadSafeInfoRead

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
.N MPI_ERR_ARG
@*/
int MPI_Info_get_nthkey( MPI_Info info, int n, char *key )
{
    MPID_Info *curr_ptr, *info_ptr=0;
    int       nkeys;
    static const char FCNAME[] = "MPI_Info_get_nthkey";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET_NTHKEY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("info");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET_NTHKEY);

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
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

	    MPIU_ERR_CHKANDJUMP((!key), mpi_errno, MPI_ERR_INFO_KEY, "**infokeynull");
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    curr_ptr = info_ptr->next;
    nkeys = 0;
    while (curr_ptr && nkeys != n)
    {
	curr_ptr = curr_ptr->next;
	nkeys++;
    }

    /* very that n is valid */
    MPIU_ERR_CHKANDJUMP2((!curr_ptr), mpi_errno, MPI_ERR_ARG, "**infonkey", "**infonkey %d %d", n, nkeys);

    MPIU_Strncpy( key, curr_ptr->key, MPI_MAX_INFO_KEY+1 );
    /* Eventually, we could remember the location of this key in 
       the head using the key/value locations (and a union datatype?) */

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NTHKEY);
    MPIU_THREAD_SINGLE_CS_EXIT("info");
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_info_get_nthkey",
	    "**mpi_info_get_nthkey %I %d %p", info, n, key);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
