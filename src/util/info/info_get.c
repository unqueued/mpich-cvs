/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get = PMPI_Info_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get  MPI_Info_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get as PMPI_Info_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_get
#define MPI_Info_get PMPI_Info_get
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get

/*@
    MPI_Info_get - Retrieves the value associated with a key

Input Parameters:
+ info - info object (handle)
. key - key (string)
- valuelen - length of value argument (integer)

Output Parameters:
+ value - value (string)
- flag - true if key defined, false if not (boolean)


.N ThreadSafeInfoRead

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
.N MPI_ERR_INFO_KEY
.N MPI_ERR_ARG
.N MPI_ERR_INFO_VALUE
@*/
int MPI_Info_get(MPI_Info info, char *key, int valuelen, char *value, 
		 int *flag)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Info_get";
#endif
    MPID_Info *curr_ptr, *info_ptr=0;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("info");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET);
    
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

	    /* Check value arguments */
	    MPIR_ERRTEST_ARGNEG(valuelen, "valuelen", mpi_errno);
	    MPIU_ERR_CHKANDSTMT((!value), mpi_errno, MPI_ERR_INFO_VALUE,;, 
				"**infovalnull");
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
	if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
	    MPIU_Strncpy(value, curr_ptr->value, valuelen);
	    /* The following is problematic - if the user passes the
	       declared length, then this will access memory one
	       passed that point */
	    /* FIXME: The real fix is to change MPIU_Strncpy to 
	       set the null at the end (always!) and return an error
	       if it had to truncate the result. */
	    /* value[valuelen] = '\0'; */
	    *flag = 1;
	    break;
	}
	curr_ptr = curr_ptr->next;
    }
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET);
    MPIU_THREAD_SINGLE_CS_EXIT("info");
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE,
	    FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_info_get",
	    "**mpi_info_get %I %s %d %p %p", info, key, valuelen, value, flag);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
