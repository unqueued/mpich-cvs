/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

#undef MPID_TYPE_ALLOC_DEBUG

int MPIDI_Type_indexed_count_contig(int count,
				    int *blocklength_array,
				    void *displacement_array,
				    int dispinbytes,
				    MPI_Aint old_extent);

/*@
  MPID_Type_indexed - create an indexed datatype
 
  Input Parameters:
+ count - number of blocks in type
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes, otherwise
  they in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which new type is based

  Output Parameters:
. newtype - handle of new indexed datatype

  Return Value:
  0 on success, -1 on failure.
@*/

int MPID_Type_indexed(int count,
		      int *blocklength_array,
		      void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin, old_is_contig;
    int i, contig_count;
    int el_sz, el_ct, old_ct, old_sz;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;
    MPI_Datatype el_type;

    MPID_Datatype *new_dtp;

    if (count == 0) return MPID_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Type_indexed",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin)
    {
	/* builtins are handled differently than user-defined types because
	 * they have no associated dataloop or datatype structure.
	 */
	el_sz      = MPID_Datatype_get_basic_size(oldtype);
	old_sz     = el_sz;
	el_ct      = 1;
	el_type    = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;

	new_dtp->n_contig_blocks = count;
    }
    else
    {
	/* user-defined base type (oldtype) */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	old_sz  = old_dtp->size;
	el_ct   = old_dtp->n_elements;
	el_type = old_dtp->eltype;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_extent    = old_dtp->extent;
	old_is_contig = old_dtp->is_contig;

	new_dtp->has_sticky_lb = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub = old_dtp->has_sticky_ub;
	new_dtp->element_size  = el_sz;
	new_dtp->eltype        = el_type;

	new_dtp->n_contig_blocks = old_dtp->n_contig_blocks * count;
    }

    /* find the first nonzero blocklength element */
    i = 0;
    while (i < count && blocklength_array[i] == 0) i++;

    if (i == count) {
	MPIU_Handle_obj_free(&MPID_Datatype_mem, new_dtp);
	return MPID_Type_zerolen(newtype);
    }

    /* priming for loop */
    old_ct = blocklength_array[i];
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
	(((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
    
    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
			      eff_disp,
			      old_lb,
			      old_ub,
			      old_extent,
			      min_lb,
			      max_ub);
    
    /* determine min lb, max ub, and count of old types in remaining
     * nonzero size blocks
     */
    for (i++; i < count; i++)
    {
	MPI_Aint tmp_lb, tmp_ub;
	
	if (blocklength_array[i] > 0) {
	    old_ct += blocklength_array[i]; /* add more oldtypes */
	
	    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
		(((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
	
	    /* calculate ub and lb for this block */
	    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
				      eff_disp,
				      old_lb,
				      old_ub,
				      old_extent,
				      tmp_lb,
				      tmp_ub);
	
	    if (tmp_lb < min_lb) min_lb = tmp_lb;
	    if (tmp_ub > max_ub) max_ub = tmp_ub;
	}
    }
    
    new_dtp->size = old_ct * old_sz;
    
    new_dtp->lb      = min_lb;
    new_dtp->ub      = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent  = max_ub - min_lb;
    
    new_dtp->n_elements = old_ct * el_ct;
    
    /* new type is only contig for N types if it's all one big
     * block, its size and extent are the same, and the old type
     * was also contiguous.
     */
    contig_count = MPIDI_Type_indexed_count_contig(count,
						   blocklength_array,
						   displacement_array,
						   dispinbytes,
						   old_extent);
    
    if ((contig_count == 1) && (new_dtp->size == new_dtp->extent))
    {
	new_dtp->is_contig = old_is_contig;
    }
    else
    {
	new_dtp->is_contig = 0;
    }

    *newtype = new_dtp->handle;
    return mpi_errno;
}

/* MPIDI_Type_indexed_count_contig()
 *
 * Determines the actual number of contiguous blocks represented by the
 * blocklength/displacement arrays.  This might be less than count (as
 * few as 1).
 *
 * Extent passed in is for the original type.
 */
int MPIDI_Type_indexed_count_contig(int count,
				    int *blocklength_array,
				    void *displacement_array,
				    int dispinbytes,
				    MPI_Aint old_extent)
{
    int i, contig_count = 1;
    int cur_blklen = blocklength_array[0];

    if (!dispinbytes)
    {
	int cur_tdisp = ((int *) displacement_array)[0];
	
	for (i = 1; i < count; i++)
	{
	    if (blocklength_array[i] == 0)
	    {
		continue;
	    }
	    else if (cur_tdisp + cur_blklen == ((int *) displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else
	    {
		cur_tdisp  = ((int *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    else
    {
	MPI_Aint cur_bdisp = ((MPI_Aint *) displacement_array)[0];
	
	for (i = 1; i < count; i++)
	{
	    if (blocklength_array[i] == 0)
	    {
		continue;
	    }
	    else if (cur_bdisp + cur_blklen * old_extent ==
		     ((MPI_Aint *) displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else
	    {
		cur_bdisp  = ((MPI_Aint *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    return contig_count;
}

