/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create_keyval = PMPI_Comm_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create_keyval  MPI_Comm_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create_keyval as PMPI_Comm_create_keyval
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create_keyval
#define MPI_Comm_create_keyval PMPI_Comm_create_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create_keyval

/*@
   MPI_Comm_create_keyval - Create a new attribute key 

Input Parameters:
+ copy_fn - Copy callback function for 'keyval' 
. delete_fn - Delete callback function for 'keyval' 
- extra_state - Extra state for callback functions 

Output Parameter:
. comm_keyval - key value for future access (integer) 

Notes:
Key values are global (available for any and all communicators).

Default copy and delete functions are available.  These are
+ MPI_COMM_NULL_COPY_FN   - empty copy function
. MPI_COMM_NULL_DELETE_FN - empty delete function
- MPI_COMM_DUP_FN         - simple dup function

There are subtle differences between C and Fortran that require that the
copy_fn be written in the same language from which 'MPI_Comm_create_keyval'
is called.
This should not be a problem for most users; only programers using both 
Fortran and C in the same program need to be sure that they follow this rule.

.N AttrErrReturn

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso MPI_Comm_free_keyval
@*/
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
			   MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
			   int *comm_keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Comm_create_keyval";
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("attr");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE_KEYVAL);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(comm_keyval, "comm_keyval", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    keyval_ptr = (MPID_Keyval *)MPIU_Handle_obj_alloc( &MPID_Keyval_mem );
    MPIU_ERR_CHKANDJUMP(!keyval_ptr,mpi_errno,MPI_ERR_OTHER,"**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
	MPIR_Process.attr_dup  = MPIR_Attr_dup_list;
	MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
       field */
    keyval_ptr->handle           = (keyval_ptr->handle & ~(0x03c00000)) |
	(MPID_COMM << 22);
    *comm_keyval		 = keyval_ptr->handle;
    MPIU_Object_set_ref(keyval_ptr,1);
    keyval_ptr->language         = MPID_LANG_C;
    keyval_ptr->kind	         = MPID_COMM;
    keyval_ptr->extra_state      = extra_state;
    keyval_ptr->copyfn.C_CopyFunction  = comm_copy_attr_fn;
    keyval_ptr->delfn.C_DeleteFunction = comm_delete_attr_fn;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
    MPIU_THREAD_SINGLE_CS_EXIT("attr");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_create_keyval",
	    "**mpi_comm_create_keyval %p %p %p %p", comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
