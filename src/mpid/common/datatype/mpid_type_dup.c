/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPID_Type_dup - create a copy of a datatype
 
  Input Parameters:
- oldtype - handle of original datatype

  Output Parameters:
. newtype - handle of newly created copy of datatype

  NOTE: This call assumes that oldtype is not a builtin!!!

  SHOULD WE RENAME THIS TO TYPE COPY OR SOMETHING?

  Return Value:
  0 on success, -1 on failure.
@*/
int MPID_Type_dup(MPI_Datatype oldtype,
		  MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_Datatype *new_dtp, *old_dtp;
    struct MPID_Dataloop *dlp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPID_Type_dup", MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* get pointer to old datatype from handle */
    MPID_Datatype_get_ptr(oldtype, old_dtp); /* fills in old_dtp */

    /* fill in datatype */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_contig     = old_dtp->is_contig;
    new_dtp->size          = old_dtp->size;
    new_dtp->extent        = old_dtp->extent;
    new_dtp->ub            = old_dtp->ub;
    new_dtp->lb            = old_dtp->lb;
    new_dtp->true_ub       = old_dtp->true_ub;
    new_dtp->true_lb       = old_dtp->true_lb;
    new_dtp->alignsize     = old_dtp->alignsize;
    new_dtp->has_sticky_ub = old_dtp->has_sticky_ub;
    new_dtp->has_sticky_lb = old_dtp->has_sticky_lb;
    new_dtp->is_permanent  = old_dtp->is_permanent;
    new_dtp->is_committed  = old_dtp->is_committed;
    new_dtp->attributes    = NULL; /* ??? */
    new_dtp->cache_id      = -1; /* ??? */
    /*    new_dtp->name          =  ??? */
    new_dtp->n_elements    = old_dtp->n_elements;
    new_dtp->element_size  = old_dtp->element_size;
    new_dtp->eltype        = old_dtp->eltype;

    /* copy dataloop */
    dlp = (struct MPID_Dataloop *) MPIU_Malloc(old_dtp->loopsize);
    if (dlp == NULL) assert(0);

    new_dtp->loopinfo       = dlp;
    new_dtp->loopinfo_depth = old_dtp->loopinfo_depth;
    new_dtp->loopsize       = old_dtp->loopsize;

    /* copy old dataloop */
    MPID_Dataloop_copy(dlp, old_dtp->loopinfo, old_dtp->loopsize);
    /* NOTE: new_dtp->handle is filled in by MPIU_Handle_obj_alloc() */
    dlp->handle                = new_dtp->handle;

    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("dup type %x created.\n", new_dtp->handle);
#endif
    return mpi_errno;
}
