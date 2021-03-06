/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Op_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Op_create = PMPI_Op_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Op_create  MPI_Op_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Op_create as PMPI_Op_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Op_create
#define MPI_Op_create PMPI_Op_create

#ifndef MPID_OP_PREALLOC 
#define MPID_OP_PREALLOC 16
#endif

/* Preallocated op objects */
MPID_Op MPID_Op_builtin[MPID_OP_N_BUILTIN] = { {0} };
MPID_Op MPID_Op_direct[MPID_OP_PREALLOC] = { {0} };
MPIU_Object_alloc_t MPID_Op_mem = { 0, 0, 0, 0, MPID_OP, 
					    sizeof(MPID_Op), 
					    MPID_Op_direct,
					    MPID_OP_PREALLOC, };

#ifdef HAVE_CXX_BINDING
void MPIR_Op_set_cxx( MPI_Op op, void (*opcall)(void) )
{
    MPID_Op *op_ptr;
    
    MPID_Op_get_ptr( op, op_ptr );
    op_ptr->language		= MPID_LANG_CXX;
    MPIR_Process.cxx_call_op_fn	= (void (*)(void *, void *, int, 
				    MPI_Datatype, MPI_User_function *))opcall;
}
#endif
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Op_create

/*@
  MPI_Op_create - Creates a user-defined combination function handle

  Input Parameters:
+ function - user defined function (function) 
- commute -  true if commutative;  false otherwise. (logical)

  Output Parameter:
. op - operation (handle) 

  Notes on the user function:
  The calling list for the user function type is
.vb
 typedef void (MPI_User_function) ( void * a, 
               void * b, int * len, MPI_Datatype * ); 
.ve
  where the operation is 'b[i] = a[i] op b[i]', for 'i=0,...,len-1'.  A pointer
  to the datatype given to the MPI collective computation routine (i.e., 
  'MPI_Reduce', 'MPI_Allreduce', 'MPI_Scan', or 'MPI_Reduce_scatter') is also
  passed to the user-specified routine.

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Op_free
@*/
int MPI_Op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
    static const char FCNAME[] = "MPI_Op_create";
    MPID_Op *op_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_OP_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("coll");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_OP_CREATE);

    /* ... body of routine ...  */
    
    op_ptr = (MPID_Op *)MPIU_Handle_obj_alloc( &MPID_Op_mem );
    /* --BEGIN ERROR HANDLING-- */
    if (!op_ptr)
    {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
					  "**nomem %s", "MPI_Op" );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    *op	             = op_ptr->handle;
    op_ptr->language = MPID_LANG_C;
    op_ptr->kind     = commute ? MPID_OP_USER : MPID_OP_USER_NONCOMMUTE;
    op_ptr->function.c_function = (void (*)(const void *, void *, 
				   const int *, const MPI_Datatype *))function;
    MPIU_Object_set_ref(op_ptr,1);
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_OP_CREATE);
    MPIU_THREAD_SINGLE_CS_EXIT("coll");
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_op_create",
	    "**mpi_op_create %p %d %p", function, commute, op);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

