/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_keyval = PMPI_Type_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_keyval  MPI_Type_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_keyval as PMPI_Type_create_keyval
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_keyval PMPI_Type_create_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_keyval

/*@
   MPI_Type_create_keyval - create keyval

   Arguments:
+  MPI_Type_copy_attr_function *type_copy_attr_fn - copy function
.  MPI_Type_delete_attr_function *type_delete_attr_fn - delete function
.  int *type_keyval - keyval
-  void *extra_state - extra state

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Type_create_keyval";
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
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
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
	MPIR_Process.attr_dup  = MPIR_Attr_dup_list;
	MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
       field */
    keyval_ptr->handle           = (keyval_ptr->handle & ~(0x03c00000)) |
	(MPID_DATATYPE << 22);
    *type_keyval		 = keyval_ptr->handle;
    MPIU_Object_set_ref(keyval_ptr,1);
    keyval_ptr->language         = MPID_LANG_C;
    keyval_ptr->kind	         = MPID_DATATYPE;
    keyval_ptr->extra_state      = extra_state;
    keyval_ptr->copyfn.C_CopyFunction  = type_copy_attr_fn;
    keyval_ptr->delfn.C_DeleteFunction = type_delete_attr_fn;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
    return MPI_SUCCESS;
}
