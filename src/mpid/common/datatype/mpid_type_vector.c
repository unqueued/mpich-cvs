/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

/*@
  MPID_Type_vector - create a vector datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength - number of elements in each block
. stride - distance in terms of extent of datatype between datatypes
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new vector datatype

  Return Value:
  0 on success, -1 on failure.

  This routine calls MPID_Dataloop_create_struct() to create the loops for this
  new datatype.  It calls MPID_Datatype_new() to allocate space for the new
  datatype.
@*/
int MPID_Type_vector(int count,
		     int blocklength,
		     int stride,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_Datatype *new_dtp;
    struct MPID_Dataloop *dlp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* Note: handle and ref_count, the first two parameters in the datatype
     * structure, are filled in automatically by the handle allocation
     * function.
     */

    new_dtp->combiner     = MPI_COMBINER_VECTOR;
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;

    /* The remaining parameters are filled in based on whether oldtype is 
     * a builtin or not: loopinfo, loopsize, size, extent, has_mpi1_ub,
     * has_mpi1_lb, loopinfo_depth, true_lb, alignsize, n_elements, 
     * element_size.
     *
     * Plus of course the loopinfo itself must still be filled in.
     */

    /* builtins are handled differently than user-defined types because they
     * have no associated dataloop or datatype structure.
     */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN) {
	/* get old values directly from the handle using bit ops */
	int oldsize = MPID_Datatype_get_basic_size(oldtype);

	/* fill in remainder of new datatype */
	new_dtp->size           = oldsize * count * blocklength;
	new_dtp->extent         = ((count-1) * stride + blocklength) * oldsize;
	new_dtp->has_mpi1_ub    = 0;
	new_dtp->has_mpi1_lb    = 0;
	new_dtp->loopinfo_depth = 1;
	new_dtp->true_lb        = 0;
	new_dtp->alignsize      = oldsize;
	new_dtp->n_elements     = count * blocklength;

	/* allocate dataloop */
	new_dtp->loopsize       = sizeof(struct MPID_Dataloop);

	/* TODO: maybe create a dataloop allocation function that understands 
	 * the types???
	 */
	dlp                     = (struct MPID_Dataloop *)MPIU_Malloc(sizeof(struct MPID_Dataloop));
	new_dtp->opt_loopinfo   = dlp;
	new_dtp->loopinfo       = dlp;

	/* fill in dataloop, noting that this is a leaf.  no need to copy. */
	/* NOTE: element size is off. */
	dlp->kind                       = DLOOP_KIND_VECTOR | DLOOP_FINAL_MASK | (oldsize << DLOOP_ELMSIZE_SHIFT);
	dlp->loop_params.v_t.count      = count;
	dlp->loop_params.v_t.blocksize  = blocklength;
	dlp->loop_params.v_t.stride     = stride * oldsize; /* in bytes */
	dlp->loop_params.v_t.u.handle   = oldtype;
	dlp->el_extent                  = oldsize;
	dlp->el_size                    = oldsize;
    }
    else /* user-defined type */ {
	assert(0);
	/* get pointer to old datatype from handle */

	/* get old values from the old datatype structure */

	/* allocate space for dataloop */

	/* fill in top part of dataloop */

	/* copy in old dataloop */
    }

    /* return handle to new datatype in last parameter */
    *newtype = new_dtp->handle;
    return MPI_SUCCESS;
}





