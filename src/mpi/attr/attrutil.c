/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"
/*
 * Keyvals.  These are handled just like the other opaque objects in MPICH
 * The predefined keyvals (and their associated attributes) are handled 
 * separately, without using the keyval 
 * storage
 */

#ifndef MPID_KEYVAL_PREALLOC 
#define MPID_KEYVAL_PREALLOC 16
#endif

/* Preallocated keyval objects */
MPID_Keyval MPID_Keyval_direct[MPID_KEYVAL_PREALLOC];
MPIU_Object_alloc_t MPID_Keyval_mem = { 0, 0, 0, 0, MPID_KEYVAL, 
					    sizeof(MPID_Keyval), 
					    MPID_Keyval_direct,
					    MPID_KEYVAL_PREALLOC, };

void MPID_Keyval_free(MPID_Keyval *keyval_ptr)
{
    MPIU_Handle_obj_free(&MPID_Keyval_mem, keyval_ptr);
}

#ifndef MPID_ATTR_PREALLOC 
#define MPID_ATTR_PREALLOC 32
#endif

/* Preallocated keyval objects */
MPID_Attribute MPID_Attr_direct[MPID_ATTR_PREALLOC];
MPIU_Object_alloc_t MPID_Attr_mem = { 0, 0, 0, 0, MPID_ATTR, 
					    sizeof(MPID_Attribute), 
					    MPID_Attr_direct,
					    MPID_ATTR_PREALLOC, };

void MPID_Attr_free(MPID_Attribute *attr_ptr)
{
    MPIU_Handle_obj_free(&MPID_Attr_mem, attr_ptr);
}

/*
  This function deletes a single attribute.
  It is called by both the function to delete a list and attribute set/put 
  val.  Return the return code from the delete function; 0 if there is no
  delete function.
*/
int MPIR_Comm_call_attr_delete( MPI_Comm comm, MPID_Attribute *attr_p )
{
    MPID_Delete_function delfn;
    MPID_Lang_t          language;
    int                  mpi_errno=0;
    
    delfn    = attr_p->keyval->delfn;
    language = attr_p->keyval->language;
    switch (language) {
    case MPID_LANG_C: 
	if (delfn.C_CommDeleteFunction) {
	    mpi_errno = delfn.C_CommDeleteFunction( comm, 
						    attr_p->keyval->handle, 
						    attr_p->value, 
						    attr_p->keyval->extra_state );
	}
	break;

#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX: 
	if (delfn.C_CommDeleteFunction) {
	    mpi_errno = (*MPIR_Process.cxx_call_delfn)( (int)comm, 
						attr_p->keyval->handle, 
						attr_p->value,
						attr_p->keyval->extra_state, 
				(void (*)(void)) delfn.C_CommDeleteFunction );
	}
	break;
#endif

#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN: 
	{
	    MPI_Fint fcomm, fkeyval, fvalue, *fextra, ierr;
	    if (delfn.F77_DeleteFunction) {
		fcomm   = (MPI_Fint) (comm);
		fkeyval = (MPI_Fint) (attr_p->keyval->handle);
		fvalue  = (MPI_Fint) (attr_p->value);
		fextra  = (MPI_Fint*) (attr_p->keyval->extra_state);
		delfn.F77_DeleteFunction( &fcomm, &fkeyval, &fvalue, 
					  fextra, &ierr );
		if (ierr) mpi_errno = (int)ierr;
	    }
	}
	break;
    case MPID_LANG_FORTRAN90: 
	{
	    MPI_Fint fcomm, fkeyval, ierr;
	    MPI_Aint fvalue, *fextra;
	    if (delfn.F90_DeleteFunction) {
		fcomm   = (MPI_Fint) (comm);
		fkeyval = (MPI_Fint) (attr_p->keyval->handle);
		fvalue  = (MPI_Aint) (attr_p->value);
		fextra  = (MPI_Aint*) (attr_p->keyval->extra_state );
		delfn.F90_DeleteFunction( &fcomm, &fkeyval, &fvalue, 
					  fextra, &ierr );
		if (ierr) mpi_errno = (int)ierr;
	    }
	}
	break;
#endif
    }
    return mpi_errno;
}

/* Routine to duplicate an attribute list */
int MPIR_Comm_attr_dup_list( MPID_Comm *comm_ptr, MPID_Attribute **new_attr )
{
    MPID_Attribute     *p, *new_p, **next_new_attr_ptr = new_attr;
    MPID_Copy_function copyfn;
    MPID_Lang_t        language;
    void               *new_value;
    int                flag;
    int                mpi_errno = 0;

    p = comm_ptr->attributes;
    while (p) {
	/* Run the attribute delete function first */
/* Why is this here?
	mpi_errno = MPIR_Comm_call_attr_delete( comm_ptr->handle, p );
	if (mpi_errno) 
	    return mpi_errno;
*/

	/* Now call the attribute copy function (if any) */
	copyfn   = p->keyval->copyfn;
	language = p->keyval->language;
	flag = 0;
	switch (language) {
	case MPID_LANG_C: 
	    if (copyfn.C_CommCopyFunction) {
		mpi_errno = copyfn.C_CommCopyFunction( comm_ptr->handle, 
						p->keyval->handle, 
						p->keyval->extra_state, 
						p->value, &new_value, &flag );
	    }
	    break;
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX: 
	    break;
#endif
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN: 
	    {
		if (copyfn.F77_CopyFunction) {
		    MPI_Fint fcomm, fkeyval, fvalue, *fextra, fflag, fnew, ierr;
		    fcomm   = (MPI_Fint) (comm_ptr->handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = (MPI_Fint) (p->value);
		    fextra  = (MPI_Fint*) (p->keyval->extra_state );
		    copyfn.F77_CopyFunction( &fcomm, &fkeyval, fextra,
					     &fvalue, &fnew, &fflag, &ierr );
		    if (ierr) mpi_errno = (int)ierr;
		    flag      = fflag;
		    new_value = (void *)fnew;
		}
	    }
	    break;
	case MPID_LANG_FORTRAN90: 
	    {
		if (copyfn.F90_CopyFunction) {
		    MPI_Fint fcomm, fkeyval, fflag, ierr;
		    MPI_Aint fvalue, fnew, *fextra;
		    fcomm   = (MPI_Fint) (comm_ptr->handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = (MPI_Aint) (p->value);
		    fextra  = (MPI_Aint*) (p->keyval->extra_state );
		    copyfn.F90_CopyFunction( &fcomm, &fkeyval, fextra,
					     &fvalue, &fnew, &fflag, &ierr );
		    if (ierr) mpi_errno = (int)ierr;
		    flag = fflag;
		    new_value = (void *)fnew;
		}
	    }
	    break;
#endif
	}
	
	/* If flag was returned as true and there was no error, then
	   insert this attribute into the new list (new_attr) */
	if (flag && !mpi_errno) {
	    /* duplicate the attribute by creating new storage, copying the
	       attribute value, and invoking the copy function */
	    new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	    if (!new_p) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
		return mpi_errno;
	    }
	    if (!*new_attr) { 
		*new_attr = new_p;
	    }
	    new_p->keyval        = p->keyval;
	    *(next_new_attr_ptr) = new_p;
	    
	    new_p->pre_sentinal  = 0;
	    new_p->value 	 = new_value;
	    new_p->post_sentinal = 0;
	    new_p->next	         = 0;
	    next_new_attr_ptr    = &(new_p->next);
	}
	else if (mpi_errno) 
	    return mpi_errno;

	p = p->next;
    }
    return mpi_errno;
}

/* Routine to delete an attribute list */
int MPIR_Comm_attr_delete_list( MPID_Comm *comm_ptr, MPID_Attribute *attr )
{
    MPID_Attribute *p, *new_p;
    int mpi_errno = MPI_SUCCESS;

    p = attr;
    while (p) {
	/* delete the attribute by first executing the delete routine, if any,
	   determine the the next attribute, and recover the attributes 
	   storage */
	new_p = p->next;
	
	/* Check the sentinals first */
	if (p->pre_sentinal != 0 || p->post_sentinal != 0) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, 
					      "**attrsentinal", 0 );
	    /* We could keep trying to free the attributes, but for now
	       we'll just bag it */
	    return mpi_errno;
	}
	/* For this attribute, find the delete function for the 
	   corresponding keyval */
	/* Still to do: capture any error returns but continue to 
	   process attributes */
	mpi_errno = MPIR_Comm_call_attr_delete( comm_ptr->handle, p );
	
	MPIU_Handle_obj_free( &MPID_Attr_mem, p );
	
	p = new_p;
    }
    return mpi_errno;
}

#ifdef HAVE_FORTRAN_BINDING
/* This routine is used by the Fortran binding to change the language of a
   keyval to Fortran 
*/
void MPIR_Keyval_set_fortran( int keyval )
{
    MPID_Keyval *keyval_ptr;

    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    if (keyval_ptr) 
	keyval_ptr->language = MPID_LANG_FORTRAN;
}
#endif
#ifdef HAVE_CXX_BINDING
/* This function allows the C++ interface to provide the routines that use
   a C interface that invoke the C++ attribute copy and delete functions.
*/
void MPIR_Keyval_set_cxx( int keyval, void (*delfn)(void), void (*copyfn)(void) )
{
    MPID_Keyval *keyval_ptr;

    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    
    keyval_ptr->language	 = MPID_LANG_CXX;
    MPIR_Process.cxx_call_delfn	 = delfn;
    MPIR_Process.cxx_call_copyfn = copyfn;
}
#endif
