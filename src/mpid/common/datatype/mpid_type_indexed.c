/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

#undef MPID_TYPE_ALLOC_DEBUG

static int MPIDI_Type_indexed_count_contig(int count,
					   int *blocklength_array,
					   void *displacement_array,
					   int dispinbytes,
					   MPI_Aint old_extent);

static void MPIDI_Type_indexed_array_copy(int count,
					  int contig_count,
					  int *input_blocklength_array,
					  void *input_displacement_array,
					  int *output_blocklength_array,
					  MPI_Aint *output_displacement_array,
					  int dispinbytes,
					  MPI_Aint old_extent);
/*@
  MPID_Type_indexed - create an indexed datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes, otherwise
  they in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new indexed datatype

  Return Value:
  0 on success, -1 on failure.

  This routine calls MPID_Dataloop_copy() to create the loops for this
  new datatype.  It calls MPIU_Handle_obj_alloc() to allocate space for the new
  datatype.
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

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_indexed", __LINE__,
					 MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->loopsize       = -1;
    new_dtp->loopinfo       = NULL;
    new_dtp->loopinfo_depth = -1;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);


    /* builtins are handled differently than user-defined types because they
     * have no associated dataloop or datatype structure.
     */
    if (is_builtin) {
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
    }
    else /* user-defined base type (oldtype) */ {
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
    }

    /* priming for loop */
    old_ct = blocklength_array[0];
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[0] :
	(((MPI_Aint) ((int *) displacement_array)[0]) * old_extent);

    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[0],
			      eff_disp,
			      old_lb,
			      old_ub,
			      old_extent,
			      min_lb,
			      max_ub);

    /* determine min lb, max ub, and count of old types */
    for (i=1; i < count; i++) {
	MPI_Aint tmp_lb, tmp_ub;
	
	old_ct += blocklength_array[i]; /* add more oldtypes */
	
	eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
	    (((MPI_Aint) ((int *) displacement_array)[0]) * old_extent);
	
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

    if ((contig_count == 1) && (new_dtp->size == new_dtp->extent)) {
	new_dtp->is_contig = old_is_contig;
    }
    else {
	new_dtp->is_contig = 0;
    }

    /* fill in dataloop */
    MPID_Dataloop_create_indexed(count,
				 blocklength_array,
				 displacement_array,
				 dispinbytes,
				 oldtype,
				 &(new_dtp->loopinfo),
				 &(new_dtp->loopsize),
				 &(new_dtp->loopinfo_depth),
				 0);

    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("(h)indexed type %x created.\n", new_dtp->handle);
#endif
    return MPI_SUCCESS;
}

void MPID_Dataloop_create_indexed(int count,
				  int *blocklength_array,
				  void *displacement_array,
				  int dispinbytes,
				  MPI_Datatype oldtype,
				  MPID_Dataloop **dlp_p,
				  int *dlsz_p,
				  int *dldepth_p,
				  int flags)
{
    int is_builtin;
    int i, old_loop_sz, new_loop_sz, old_loop_depth;
    int contig_count, old_type_count = 0;

    MPI_Aint old_extent;
    char *curpos;

    MPID_Datatype *old_dtp = NULL;
    struct MPID_Dataloop *new_dlp;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin) {
	old_extent     = MPID_Datatype_get_basic_size(oldtype);
	old_loop_sz    = 0;
	old_loop_depth = 0;
    }
    else {
	MPID_Datatype_get_ptr(oldtype, old_dtp);
	old_extent     = old_dtp->extent;
	old_loop_sz    = old_dtp->loopsize;
	old_loop_depth = old_dtp->loopinfo_depth;
    }

    for (i=0; i < count; i++) {
	old_type_count += blocklength_array[i];
    }

    contig_count = MPIDI_Type_indexed_count_contig(count,
						   blocklength_array,
						   displacement_array,
						   dispinbytes,
						   old_extent);
    assert(contig_count > 0);

    /* optimization:
     *
     * if contig_count == 1 and block starts at displacement 0,
     * store it as a contiguous rather than an indexed dataloop.
     */
    if ((contig_count == 1) &&
	((!dispinbytes && ((int *) displacement_array)[0] == 0) ||
	 (dispinbytes && ((MPI_Aint *) displacement_array)[0] == 0)))
    {
	new_loop_sz = sizeof(struct MPID_Dataloop) + old_loop_sz;
	new_dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loop_sz);
	assert(new_dlp != NULL);

	new_dlp->loop_params.c_t.count = old_type_count;

	if (is_builtin) {
	    new_dlp->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
#if 0
	    new_dlp->handle = new_dtp->handle;
#endif
	    new_dlp->el_size   = old_extent;
	    new_dlp->el_extent = old_extent;
	    new_dlp->el_type   = oldtype;

	    new_dlp->loop_params.c_t.dataloop = NULL;
	}
	else {
	    new_dlp->kind = DLOOP_KIND_CONTIG;
#if 0
	    new_dlp->handle = new_dtp->handle;
#endif
	    new_dlp->el_size   = old_dtp->size;
	    new_dlp->el_extent = old_dtp->extent;
	    new_dlp->el_type   = old_dtp->eltype;


	    curpos = (char *) new_dlp;
	    curpos += sizeof(struct MPID_Dataloop);
	    /* TODO: ACCOUNT FOR PADDING HERE */

	    /* copy old dataloop and set pointer to it */
	    MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	    new_dlp->loop_params.c_t.dataloop =
		(struct MPID_Dataloop *) curpos;
	}

	*dlp_p     = new_dlp;
	*dlsz_p    = new_loop_sz;
	*dldepth_p = old_loop_depth + 1;

	return;
    }

    /* note: could look for a blockindexed or a vector here... */

    /* otherwise storing as an indexed dataloop */

    new_loop_sz = sizeof(struct MPID_Dataloop) + 
	(contig_count * (sizeof(MPI_Aint) + sizeof(int))) +
	old_loop_sz;
    /* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */
    new_dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loop_sz);
    assert(new_dlp != NULL);

    if (is_builtin) {
	new_dlp->kind = DLOOP_KIND_INDEXED | DLOOP_FINAL_MASK;

#if 0
	new_dlp->handle = new_dtp->handle;
#endif
	new_dlp->el_size   = old_extent;
	new_dlp->el_extent = old_extent;
	new_dlp->el_type   = oldtype;

	new_dlp->loop_params.i_t.dataloop = NULL;
    }
    else {
	new_dlp->kind = DLOOP_KIND_INDEXED;
#if 0
	new_dlp->handle = new_dtp->handle;
#endif
	new_dlp->el_size   = old_dtp->size;
	new_dlp->el_extent = old_dtp->extent;
	new_dlp->el_type   = old_dtp->eltype;

	/* copy old dataloop and set pointer to it */
	curpos = (char *) new_dlp;
	curpos += (new_loop_sz - old_loop_sz);
	MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	new_dlp->loop_params.i_t.dataloop = (struct MPID_Dataloop *) curpos;
    }

    new_dlp->loop_params.i_t.count        = contig_count;
    new_dlp->loop_params.i_t.total_blocks = old_type_count;

    /* copy in blocklength and displacement parameters (in that order)
     *
     * regardless of dispinbytes, we store displacements in bytes in loop.
     */
    curpos = (char *) new_dlp;
    curpos += sizeof(struct MPID_Dataloop);

    new_dlp->loop_params.i_t.blocksize_array = (int *) curpos;
    curpos += contig_count * sizeof(int);

    new_dlp->loop_params.i_t.offset_array = (MPI_Aint *) curpos;

    MPIDI_Type_indexed_array_copy(count,
				  contig_count,
				  blocklength_array,
				  displacement_array,
				  new_dlp->loop_params.i_t.blocksize_array,
				  new_dlp->loop_params.i_t.offset_array,
				  dispinbytes,
				  old_extent);

    *dlp_p     = new_dlp;
    *dlsz_p    = new_loop_sz;
    *dldepth_p = old_loop_depth + 1;

    return;
}


/* MPIDI_Type_indexed_count_contig()
 *
 * Determines the actual number of contiguous blocks represented by the
 * blocklength/displacement arrays.  This might be less than count (as
 * few as 1).
 *
 * Extent passed in is for the original type.
 */
static int MPIDI_Type_indexed_count_contig(int count,
					   int *blocklength_array,
					   void *displacement_array,
					   int dispinbytes,
					   MPI_Aint old_extent)
{
    int i, contig_count = 1;
    int cur_blklen = blocklength_array[0];

    if (!dispinbytes) {
	int cur_tdisp = ((int *) displacement_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (cur_tdisp + cur_blklen == ((int *) displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else {
		cur_tdisp  = ((int *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    else {
	MPI_Aint cur_bdisp = ((MPI_Aint *) displacement_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (cur_bdisp + cur_blklen * old_extent ==
		((MPI_Aint *) displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else {
		cur_bdisp  = ((MPI_Aint *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    return contig_count;
}


/* MPIDI_Type_indexed_array_copy()
 *
 * Copies arrays into place, combining adjacent contiguous regions.
 *
 * Extent passed in is for the original type.
 */
static void MPIDI_Type_indexed_array_copy(int count,
					  int contig_count,
					  int *in_blklen_array,
					  void *in_disp_array,
					  int *out_blklen_array,
					  MPI_Aint *out_disp_array,
					  int dispinbytes,
					  MPI_Aint old_extent)
{
    int i, cur_idx = 0;

    out_blklen_array[0] = in_blklen_array[0];

    if (!dispinbytes) {
	 out_disp_array[0] = (MPI_Aint) ((int *) in_disp_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (out_disp_array[cur_idx] + ((MPI_Aint) out_blklen_array[cur_idx]) * old_extent ==
		((MPI_Aint) ((int *) in_disp_array)[i]) * old_extent)
	    {
		/* adjacent to current block; add to block */
		out_blklen_array[cur_idx] += in_blklen_array[i];
	    }
	    else {
		cur_idx++;
		assert(cur_idx < contig_count);
		out_disp_array[cur_idx] = ((MPI_Aint) ((int *) in_disp_array)[i]) * old_extent;
		out_blklen_array[cur_idx]  = in_blklen_array[i];
	    }
	}
    }
    else {
	out_disp_array[0] = ((MPI_Aint *) in_disp_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (out_disp_array[cur_idx] + ((MPI_Aint) out_blklen_array[cur_idx]) * old_extent ==
		((MPI_Aint *) in_disp_array)[i])
	    {
		/* adjacent to current block; add to block */
		out_blklen_array[cur_idx] += in_blklen_array[i];
	    }
	    else {
		cur_idx++;
		assert(cur_idx < contig_count);
		out_disp_array[cur_idx]   = ((MPI_Aint *) in_disp_array)[i];
		out_blklen_array[cur_idx] = in_blklen_array[i];
	    }
	}
    }
    return;
}


