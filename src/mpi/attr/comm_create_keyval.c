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
#define MPI_Comm_create_keyval PMPI_Comm_create_keyval

#ifdef HAVE_CXX_BINDING
void MPIR_Keyval_set_cxx( int keyval )
{
    MPID_Keyval *keyval_ptr;

    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    
    keyval_ptr->language = MPID_LANG_CXX;
}
#endif
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create_keyval

/*@
   MPI_Comm_create_keyval - Create a new attribute key 

   Input Parameters:
+  MPI_Comm_copy_attr_function *comm_copy_attr_fn - copy function
.  MPI_Comm_delete_attr_function *comm_delete_attr_fn - delete function
-  void *extra_state - extra state

   Output Parameters:
.  int *comm_keyval - keyval

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
			   MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
			   int *comm_keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Comm_create_keyval";
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_KEYVAL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    keyval_ptr = (MPID_Keyval *)MPIU_Handle_obj_alloc( &MPID_Keyval_mem );
    if (!keyval_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    /* Initialize the attribute dup function */
    if (!MPIR_Process.comm_attr_dup) {
	MPIR_Process.comm_attr_dup  = MPIR_Comm_attr_dup_list;
	MPIR_Process.comm_attr_free = MPIR_Comm_attr_delete_list;
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
    keyval_ptr->copyfn.C_CommCopyFunction  = comm_copy_attr_fn;
    keyval_ptr->delfn.C_CommDeleteFunction = comm_delete_attr_fn;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
    return MPI_SUCCESS;
}
