/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_attr = PMPI_Win_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_attr  MPI_Win_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_attr as PMPI_Win_get_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_attr
#define MPI_Win_get_attr PMPI_Win_get_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_attr

/*@
   MPI_Win_get_attr - Get attribute cached on an MPI window object

   Input Parameters:
+ win - window to which the attribute is attached (handle) 
- win_keyval - key value (integer) 

   Output Parameters:
+ attribute_val - attribute value, unless flag is false 
- flag - false if no attribute is associated with the key (logical) 

   Notes:
   The following attributes are predefined for all MPI Window objects\:

+ MPI_WIN_BASE - window base address. 
. MPI_WIN_SIZE - window size, in bytes. 
- MPI_WIN_DISP_UNIT - displacement unit associated with the window. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
.N MPI_ERR_OTHER
@*/
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, 
		     int *flag)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Win_get_attr";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("attr");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
	    MPIR_ERRTEST_KEYVAL(win_keyval, MPID_WIN, "window", mpi_errno);
#           ifdef NEEDS_POINTER_ALIGNMENT_ADJUST
            /* A common user error is to pass the address of a 4-byte
	       int when the address of a pointer (or an address-sized int)
	       should have been used.  We can test for this specific
	       case.  Note that this code assumes sizeof(MPI_Aint) is 
	       a power of 2. */
	    if ((MPI_Aint)attribute_val & (sizeof(MPI_Aint)-1)) {
		MPIU_ERR_SET(mpi_errno,MPI_ERR_ARG,"**attrnotptr");
	    }
#           endif
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
	MPID_BEGIN_ERROR_CHECKS;
	{
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(attribute_val, "attribute_val", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
	MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */    

    /* ... body of routine ...  */
    
    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
       to copy the values from the per-process block and pass the user
       a pointer to a copy */
    /* Note that if we are called from Fortran, we must return the values,
       not the addresses, of these attributes */
    if (HANDLE_GET_KIND(win_keyval) == HANDLE_KIND_BUILTIN) {
	int attr_idx = win_keyval & 0x0000000f;
	void **attr_val_p = (void **)attribute_val;
#ifdef HAVE_FORTRAN_BINDING
	/* Note that this routine only has a Fortran 90 binding,
	   so the attribute value is an address-sized int */
	MPI_Aint  *attr_int = (MPI_Aint *)attribute_val;
#endif
	*flag = 1;

	/* 
	 * The C versions of the attributes return the address of a 
	 * *COPY* of the value (to prevent the user from changing it)
	 * and the Fortran versions provide the actual value (as an Fint)
	 */
	switch (attr_idx) {
	case 1: /* WIN_BASE */
	    *attr_val_p = win_ptr->base;
	    break;
	case 3: /* SIZE */
	    win_ptr->copySize = win_ptr->size;
	    *attr_val_p = &win_ptr->copySize;
	    break;
	case 5: /* DISP_UNIT */
	    win_ptr->copyDispUnit = win_ptr->disp_unit;
	    *attr_val_p = &win_ptr->copyDispUnit;
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case 2: /* Fortran BASE */
	    /* The Fortran routine that matches this routine should
	       provide an address-sized integer, not an MPI_Fint */
	    *attr_int = (MPI_Aint)(win_ptr->base);
	    break;
	case 4: /* Fortran SIZE */
	    /* We do not need to copy because we return the value,
	       not a pointer to the value */
	    *attr_int = win_ptr->size;
	    break;
	case 6: /* Fortran DISP_UNIT */
	    /* We do not need to copy because we return the value,
	       not a pointer to the value */
	    *attr_int = win_ptr->disp_unit;
	    break;
#endif
	}
    }
    else {
	MPID_Attribute *p = win_ptr->attributes;

	*flag = 0;
	while (p) {
	    if (p->keyval->handle == win_keyval) {
		*flag = 1;
		(*(void **)attribute_val) = p->value;
		break;
	    }
	    p = p->next;
	}
    }

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_ATTR);
    MPIU_THREAD_SINGLE_CS_EXIT("attr");
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_win_get_attr", 
	    "**mpi_win_get_attr %W %d %p %p", 
	    win, win_keyval, attribute_val, flag);
    }
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
