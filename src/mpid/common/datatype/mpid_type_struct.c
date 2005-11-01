/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

#undef MPID_STRUCT_FLATTEN_DEBUG
#undef MPID_STRUCT_DEBUG

static int MPID_Type_struct_alignsize(int count,
				      MPI_Datatype *oldtype_array);

static int MPID_Type_struct_alignsize(int count,
				      MPI_Datatype *oldtype_array)
{
    int i, max_alignsize = 0, tmp_alignsize;

    for (i=0; i < count; i++)
    {
	/* shouldn't be called with an LB or UB, but we'll handle it nicely */
	if (oldtype_array[i] == MPI_LB || oldtype_array[i] == MPI_UB) continue;
	else if (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN)
	{
	    tmp_alignsize = MPID_Datatype_get_basic_size(oldtype_array[i]);
	}
	else
	{
	    MPID_Datatype *dtp;	    

	    MPID_Datatype_get_ptr(oldtype_array[i], dtp);
	    tmp_alignsize = dtp->alignsize;
	}
	if (max_alignsize < tmp_alignsize) max_alignsize = tmp_alignsize;
    }

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (max_alignsize > HAVE_MAX_STRUCT_ALIGNMENT)
	max_alignsize = HAVE_MAX_STRUCT_ALIGNMENT;
#endif
    /* if we didn't calculate a maximum struct alignment (above), then the
     * alignment was either "largest", in which case we just use what we found,
     * or "unknown", in which case what we found is as good a guess as any.
     */

    return max_alignsize;
}


/*@
  MPID_Type_struct - create a struct datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type in bytes
- oldtype_array - types (using handle) of datatypes on which vector is based

  Output Parameters:
. newtype - handle of new struct datatype

  Return Value:
  MPI_SUCCESS on success, MPI errno on failure.
@*/
int MPID_Type_struct(int count,
		     int *blocklength_array,
		     MPI_Aint *displacement_array,
		     MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype)
{
    int err, mpi_errno = MPI_SUCCESS;
    int i, old_are_contig = 1;
    int found_sticky_lb = 0, found_sticky_ub = 0, found_true_lb = 0,
	found_true_ub = 0, found_el_type = 0;
    int el_sz = 0, size = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    MPI_Aint true_lb_disp = 0, true_ub_disp = 0, sticky_lb_disp = 0,
	sticky_ub_disp = 0;

    MPID_Datatype *new_dtp;

#ifdef MPID_STRUCT_DEBUG
    MPIDI_Datatype_printf(oldtype_array[0], 1, displacement_array[0],
			  blocklength_array[0], 1);
    for (i=1; i < count; i++)
    {
	MPIDI_Datatype_printf(oldtype_array[i], 1, displacement_array[i],
			      blocklength_array[i], 0);
    }
#endif

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_struct",
					 __LINE__, MPI_ERR_OTHER,
					 "**nomem", 0);
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

    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_depth = -1;

    /* check for junk struct with all zero blocks */
    for (i=0; i < count; i++) if (blocklength_array[i] != 0) break;

    if (count == 0 || i == count)
    {
	/* we are interpreting the standard here based on the fact that
	 * with a zero count there is nothing in the typemap.
	 *
	 * we do the same thing for a type with all zero blocks.
	 *
	 * we handle this case explicitly to get it out of the way.
	 */
	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;

	new_dtp->alignsize    = 0;
	new_dtp->element_size = 0;
	new_dtp->eltype       = 0;

	new_dtp->size    = 0;
	new_dtp->lb      = 0;
	new_dtp->ub      = 0;
	new_dtp->true_lb = 0;
	new_dtp->true_ub = 0;
	new_dtp->extent  = 0;

	new_dtp->n_elements = 0;
	new_dtp->is_contig  = 1;

	err = MPID_Dataloop_create_struct(0,
					  NULL,
					  NULL,
					  NULL,
					  &(new_dtp->dataloop),
					  &(new_dtp->dataloop_size),
					  &(new_dtp->dataloop_depth),
					  0);

	if (!err) {
	    /* heterogeneous dataloop representation */
	    err = MPID_Dataloop_create_struct(0,
					      NULL,
					      NULL,
					      NULL,
					      &(new_dtp->hetero_dloop),
					      &(new_dtp->hetero_dloop_size),
					      &(new_dtp->hetero_dloop_depth),
					      0);
	}
	/* --BEGIN ERROR HANDLING-- */
	if (err) {
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					     MPIR_ERR_RECOVERABLE,
					     "MPID_Dataloop_create_struct",
					     __LINE__,
					     MPI_ERR_OTHER,
					     "**nomem",
					     0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
  
	*newtype = new_dtp->handle;
	return mpi_errno;
    }

    for (i=0; i < count; i++)
    {
	int is_builtin =
	    (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN);
	MPI_Aint tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
	int tmp_el_sz;
	MPI_Datatype tmp_el_type;
	MPID_Datatype *old_dtp = NULL;

	/* Interpreting typemap to not include 0 blklen things, including
	 * MPI_LB and MPI_UB. -- Rob Ross, 10/31/2005
	 */
	if (blocklength_array[i] == 0) continue;

	if (is_builtin)
	{
	    /* Q: DO LB or UBs count in element counts? */
	    tmp_el_sz   = MPID_Datatype_get_basic_size(oldtype_array[i]);
	    tmp_el_type = oldtype_array[i];

	    MPID_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
				      displacement_array[i],
				      0,
				      tmp_el_sz,
				      tmp_el_sz,
				      tmp_lb,
				      tmp_ub);
	    tmp_true_lb = tmp_lb;
	    tmp_true_ub = tmp_ub;

	    size += tmp_el_sz * blocklength_array[i];
	}
	else
	{
	    MPID_Datatype_get_ptr(oldtype_array[i], old_dtp);

	    tmp_el_sz   = old_dtp->element_size;
	    tmp_el_type = old_dtp->eltype;

	    MPID_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
				      displacement_array[i],
				      old_dtp->lb,
				      old_dtp->ub,
				      old_dtp->extent,
				      tmp_lb,
				      tmp_ub);
	    tmp_true_lb = tmp_lb + (old_dtp->true_lb - old_dtp->lb);
	    tmp_true_ub = tmp_ub + (old_dtp->true_ub - old_dtp->ub);

	    size += old_dtp->size * blocklength_array[i];
	}

	/* element size and type */
	if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB)
	{
	    if (found_el_type == 0)
	    {
		el_sz         = tmp_el_sz;
		el_type       = tmp_el_type;
		found_el_type = 1;
	    }
	    else if (el_sz != tmp_el_sz)
	    {
		el_sz = -1;
		el_type = MPI_DATATYPE_NULL;
	    }
	    else if (el_type != tmp_el_type)
	    {
		/* Q: should we set el_sz = -1 even though the same? */
		el_type = MPI_DATATYPE_NULL;
	    }
	}

	/* keep lowest sticky lb */
	if ((oldtype_array[i] == MPI_LB) ||
	    (!is_builtin && old_dtp->has_sticky_lb))
	{
	    if (!found_sticky_lb)
	    {
		found_sticky_lb = 1;
		sticky_lb_disp  = tmp_lb;
	    }
	    else if (sticky_lb_disp > tmp_lb)
	    {
		sticky_lb_disp = tmp_lb;
	    }
	}

	/* keep highest sticky ub */
	if ((oldtype_array[i] == MPI_UB) || 
	    (!is_builtin && old_dtp->has_sticky_ub))
	{
	    if (!found_sticky_ub)
	    {
		found_sticky_ub = 1;
		sticky_ub_disp  = tmp_ub;
	    }
	    else if (sticky_ub_disp < tmp_ub)
	    {
		sticky_ub_disp = tmp_ub;
	    }
	}

	/* keep lowest true lb and highest true ub */
	if (oldtype_array[i] != MPI_UB && oldtype_array[i] != MPI_LB)
	{
	    if (!found_true_lb)
	    {
		found_true_lb = 1;
		true_lb_disp  = tmp_true_lb;
	    }
	    else if (true_lb_disp > tmp_true_lb)
	    {
		true_lb_disp = tmp_true_lb;
	    }
	    if (!found_true_ub)
	    {
		found_true_ub = 1;
		true_ub_disp  = tmp_true_ub;
	    }
	    else if (true_ub_disp < tmp_true_ub)
	    {
		true_ub_disp = tmp_true_ub;
	    }
	}

	if (!is_builtin && !old_dtp->is_contig)
	{
	    old_are_contig = 0;
	}
    }

    new_dtp->n_elements = -1; /* TODO */
    new_dtp->element_size = el_sz;
    new_dtp->eltype = el_type;

    new_dtp->has_sticky_lb = found_sticky_lb;
    new_dtp->true_lb       = true_lb_disp;
    new_dtp->lb = (found_sticky_lb) ? sticky_lb_disp : true_lb_disp;

    new_dtp->has_sticky_ub = found_sticky_ub;
    new_dtp->true_ub       = true_ub_disp;
    new_dtp->ub = (found_sticky_ub) ? sticky_ub_disp : true_ub_disp;

    new_dtp->alignsize = MPID_Type_struct_alignsize(count, oldtype_array);

    new_dtp->extent = new_dtp->ub - new_dtp->lb;
    if ((!found_sticky_lb) && (!found_sticky_ub))
    {
	/* account for padding */
	MPI_Aint epsilon = new_dtp->extent % new_dtp->alignsize;

	if (epsilon)
	{
	    new_dtp->ub    += (new_dtp->alignsize - epsilon);
	    new_dtp->extent = new_dtp->ub - new_dtp->lb;
	}
    }

    new_dtp->size = size;

    /* new type is contig for N types if its size and extent are the
     * same, and the old type was also contiguous
     */
    if ((new_dtp->size == new_dtp->extent) && old_are_contig)
    {
	new_dtp->is_contig = 1;
    }
    else
    {
	new_dtp->is_contig = 0;
    }

    /* fill in dataloop(s) */
    err = MPID_Dataloop_create_struct(count,
				      blocklength_array,
				      displacement_array,
				      oldtype_array,
				      &(new_dtp->dataloop),
				      &(new_dtp->dataloop_size),
				      &(new_dtp->dataloop_depth),
				      MPID_DATALOOP_HOMOGENEOUS);
    if (!err) {
	/* heterogeneous dataloop representation */
	err = MPID_Dataloop_create_struct(count,
					  blocklength_array,
					  displacement_array,
					  oldtype_array,
					  &(new_dtp->hetero_dloop),
					  &(new_dtp->hetero_dloop_size),
					  &(new_dtp->hetero_dloop_depth),
					  0);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (err) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Dataloop_create_struct",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    *newtype = new_dtp->handle;
    return mpi_errno;
}


