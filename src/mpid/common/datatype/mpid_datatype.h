/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPID_DATATYPE_H
#define MPID_DATATYPE_H

#include "mpiimpl.h"
#include "mpid_dataloop.h"
#include "mpihandlemem.h"

/* NOTE: 
 * - struct MPID_Dataloop and MPID_Segment are defined in 
 *   src/mpid/common/datatype/mpid_dataloop.h (and gen_dataloop.h).
 * - MPIU_Object_alloc_t is defined in src/include/mpihandle.h
 */

#define MPID_Datatype_get_ptr(a,ptr)   MPID_Getb_ptr(Datatype,a,0x000000ff,ptr)
#define MPID_Datatype_get_basic_size(a) (((a)&0x0000ff00)>>8)

#define MPID_Datatype_add_ref(datatype_ptr) MPIU_Object_add_ref((datatype_ptr))

#define MPID_Datatype_get_basic_type(a,eltype_)			\
{									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            eltype_ = ((MPID_Datatype *) ptr)->eltype;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            eltype_ = ((MPID_Datatype *) ptr)->eltype;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            eltype_ = a;						\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    eltype_ = 0;						\
	    break;							\
 									\
    }									\
}

/* MPID_Datatype_release decrements the reference count on the MPID_Datatype
 * and, if the refct is then zero, frees the MPID_Datatype and associated
 * structures.
 */
#define MPID_Datatype_release(datatype_ptr)				    \
{									    \
    int inuse;								    \
									    \
    MPIU_Object_release_ref((datatype_ptr),&inuse);			    \
    if (!inuse) {							    \
        int lmpi_errno = MPI_SUCCESS;					    \
	if (MPIR_Process.attr_free && datatype_ptr->attributes) {	    \
	    lmpi_errno = MPIR_Process.attr_free( datatype_ptr->handle,	    \
						datatype_ptr->attributes ); \
	}								    \
 	/* LEAVE THIS COMMENTED OUT UNTIL WE HAVE SOME USE FOR THE FREE_FN  \
	if (datatype_ptr->free_fn) {					    \
	    mpi_errno = (datatype_ptr->free_fn)( datatype_ptr );	    \
	     if (mpi_errno) {						    \
		 MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_FREE);		    \
		 return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );	    \
	     }								    \
	} */								    \
        if (lmpi_errno == MPI_SUCCESS) {				    \
	    MPID_Datatype_free(datatype_ptr);				    \
        }								    \
    }                                                                       \
}

/* Note: Probably there is some clever way to build all of these from a macro.
 */
#define MPID_Datatype_get_size_macro(a,size_)				\
{									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            size_ = ((MPID_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            size_ = ((MPID_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            size_ = MPID_Datatype_get_basic_size(a);			\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    size_ = 0;							\
	    break;							\
 									\
    }									\
}

#define MPID_Datatype_get_loopdepth_macro(a,depth_,hetero_)		\
{									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            if (!(hetero_))						\
                depth_ = ((MPID_Datatype *)ptr)->dataloop_depth;	\
            else depth_ = ((MPID_Datatype *) ptr)->hetero_dloop_depth;	\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            if (!(hetero_))						\
                depth_ = ((MPID_Datatype *)ptr)->dataloop_depth;	\
            else depth_ = ((MPID_Datatype *) ptr)->hetero_dloop_depth;	\
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            depth_ = 0;						\
            break;							\
    }                                                                   \
}

#define MPID_Datatype_get_loopsize_macro(a,depth_,hetero_)		\
{									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            if (!(hetero_))						\
                depth_ = ((MPID_Datatype *)ptr)->dataloop_size;	\
            else depth_ = ((MPID_Datatype *) ptr)->hetero_dloop_size;	\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            if (!(hetero_))						\
                depth_ = ((MPID_Datatype *)ptr)->dataloop_size;	\
            else depth_ = ((MPID_Datatype *) ptr)->hetero_dloop_size;	\
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            depth_ = 0;						\
            break;							\
    }                                                                   \
}

#define MPID_Datatype_get_loopptr_macro(a,lptr_,hetero_)		\
{									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            if (!(hetero_))						\
                lptr_ = ((MPID_Datatype *) ptr)->dataloop;		\
            else lptr_ = ((MPID_Datatype *) ptr)->hetero_dloop;	\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            if (!(hetero_))						\
                lptr_ = ((MPID_Datatype *) ptr)->dataloop;		\
            else lptr_ = ((MPID_Datatype *) ptr)->hetero_dloop;	\
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            lptr_ = 0;							\
            break;							\
    }									\
}

#define MPID_Datatype_get_hetero_loopptr_macro(a,lptr_)                \
{                                                                    \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);                 \
            lptr_ = ((MPID_Datatype *) ptr)->hetero_dloop;             \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPID_Datatype *)                                    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem)); \
            lptr_ = ((MPID_Datatype *) ptr)->hetero_dloop;             \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            lptr_ = 0;                                                 \
            break;                                                      \
    }                                                                   \
}
        
#define MPID_Datatype_get_extent_macro(a,extent_)			    \
{									    \
    void *ptr;								    \
    switch (HANDLE_GET_KIND(a)) {					    \
        case HANDLE_KIND_DIRECT:					    \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			    \
            extent_ = ((MPID_Datatype *) ptr)->extent;			    \
            break;							    \
        case HANDLE_KIND_INDIRECT:					    \
            ptr = ((MPID_Datatype *)					    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	    \
            extent_ = ((MPID_Datatype *) ptr)->extent;			    \
            break;							    \
        case HANDLE_KIND_INVALID:					    \
        case HANDLE_KIND_BUILTIN:					    \
        default:							    \
            extent_ = MPID_Datatype_get_basic_size(a);  /* same as size */ \
            break;							    \
    }									    \
}

#define MPID_Datatype_valid_ptr(ptr,err) MPID_Valid_ptr_class(Datatype,ptr,MPI_ERR_TYPE,err)

/* to be used only after MPID_Datatype_valid_ptr(); the check on
 * err == MPI_SUCCESS ensures that we won't try to dereference the
 * pointer if something has already been detected as wrong.
 */
#define MPID_Datatype_committed_ptr(ptr,err)			\
{								\
    if ((err == MPI_SUCCESS) && !((ptr)->is_committed))		\
        err = MPIR_Err_create_code(MPI_SUCCESS,			\
				   MPIR_ERR_RECOVERABLE,	\
				   FCNAME,			\
				   __LINE__,			\
				   MPI_ERR_TYPE,		\
				   "**dtypecommit",		\
				   0);				\
}

/*S
  MPID_Datatype_contents - Holds envelope and contents data for a given
                           datatype

  Notes:
  Space is allocated beyond the structure itself in order to hold the
  arrays of types, ints, and aints, in that order.

  S*/
typedef struct MPID_Datatype_contents {
    int combiner;
    int nr_ints;
    int nr_aints;
    int nr_types;
    /* space allocated beyond structure used to store the types[],
     * ints[], and aints[], in that order.
     */
} MPID_Datatype_contents;

/* Datatype Structure */
/*S
  MPID_Datatype - Description of the MPID Datatype structure

  Notes:
  The 'ref_count' is needed for nonblocking operations such as
.vb
   MPI_Type_struct( ... , &newtype );
   MPI_Irecv( buf, 1000, newtype, ..., &request );
   MPI_Type_free( &newtype );
   ...
   MPI_Wait( &request, &status );
.ve

  Module:
  Datatype-DS

  Notes:

  Alternatives:
  The following alternatives for the layout of this structure were considered.
  Most were not chosen because any benefit in performance or memory 
  efficiency was outweighed by the added complexity of the implementation.

  A number of fields contain only boolean inforation ('is_contig', 
  'has_sticky_ub', 'has_sticky_lb', 'is_permanent', 'is_committed').  These 
  could be combined and stored in a single bit vector.  

  'MPI_Type_dup' could be implemented with a shallow copy, where most of the
  data fields, would not be copied into the new object created by
  'MPI_Type_dup'; instead, the new object could point to the data fields in
  the old object.  However, this requires more code to make sure that fields
  are found in the correct objects and that deleting the old object doesn't
  invalidate the dup'ed datatype.

  Originally we attempted to keep contents/envelope data in a non-optimized
  dataloop.  The subarray and darray types were particularly problematic,
  and eventually we decided it would be simpler to just keep contents/
  envelope data in arrays separately.

  Earlier versions of the ADI used a single API to change the 'ref_count', 
  with each MPI object type having a separate routine.  Since reference
  count changes are always up or down one, and since all MPI objects 
  are defined to have the 'ref_count' field in the same place, the current
  ADI3 API uses two routines, 'MPIU_Object_add_ref' and 
  'MPIU_Object_release_ref', to increment and decrement the reference count.

  S*/
typedef struct MPID_Datatype { 
    /* handle and ref_count are filled in by MPIU_Handle_obj_alloc() */
    int          handle; /* value of MPI_Datatype for structure */
    volatile int ref_count;

    /* basic parameters for datatype, accessible via MPI calls */
    int      size;
    MPI_Aint extent, ub, lb, true_ub, true_lb;

    /* chars affecting subsequent datatype processing and creation */
    int alignsize, has_sticky_ub, has_sticky_lb;
    int is_permanent; /* non-zero if datatype is a predefined type */
    int is_committed;

    /* element information; used for accumulate and get elements
     *
     * if type is composed of more than one element type, then
     * eltype == MPI_DATATYPE_NULL and element_size == -1
     */
    int      eltype, n_elements;
    MPI_Aint element_size;

    /* information on contiguity of type, for processing shortcuts.
     *
     * is_contig is non-zero only if N instances of the type would be
     * contiguous.
     */
    int is_contig;
    int n_contig_blocks; /* # of contig blocks in one instance */

    /* pointer to contents and envelope data for the datatype */
    MPID_Datatype_contents *contents;

    /* dataloop members, including a pointer to the loop, the size in bytes,
     * and a depth used to verify that we can process it (limited stack depth
     */
    struct MPID_Dataloop *dataloop; /* might be optimized for homogenous */
    int                   dataloop_size;
    int                   dataloop_depth;

    struct MPID_Dataloop *hetero_dloop; /* heterogeneous dataloop */
    int                   hetero_dloop_size;
    int                   hetero_dloop_depth;

    /* MPI-2 attributes and name */
    struct MPID_Attribute *attributes;
    char                  name[MPI_MAX_OBJECT_NAME];

    /* not yet used; will be used to track what processes have cached
     * copies of this type.
     */
    int32_t cache_id;
    /* MPID_Lpidmask mask; */

    /* int (*free_fn)( struct MPID_Datatype * ); */ /* Function to free this datatype */

    /* Other, device-specific information */
#ifdef MPID_DEV_DATATYPE_DECL
    MPID_DEV_DATATYPE_DECL
#endif
} MPID_Datatype;

extern MPIU_Object_alloc_t MPID_Datatype_mem;

/* Preallocated datatype objects */
#define MPID_DATATYPE_N_BUILTIN 50
extern MPID_Datatype MPID_Datatype_builtin[MPID_DATATYPE_N_BUILTIN + 1];
extern MPID_Datatype MPID_Datatype_direct[];

#define MPID_DTYPE_BEGINNING  0
#define MPID_DTYPE_END       -1

/* LB/UB calculation helper macros */

/* MPID_DATATYPE_CONTIG_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the
 * old type's LB, UB, and extent, and a count of these types in the
 * block.
 *
 * Note: if the displacement is non-zero, the MPID_DATATYPE_BLOCK_LB_UB()
 * should be used instead (see below).
 */
#define MPID_DATATYPE_CONTIG_LB_UB(cnt_,		\
				   old_lb_,		\
				   old_ub_,		\
				   old_extent_,	\
				   lb_,		\
				   ub_)		\
{							\
    if (cnt_ == 0) {					\
	lb_ = old_lb_;				\
	ub_ = old_ub_;				\
    }							\
    else if (old_ub_ >= old_lb_) {			\
        lb_ = old_lb_;				\
        ub_ = old_ub_ + (old_extent_) * (cnt_ - 1);	\
    }							\
    else /* negative extent */ {			\
	lb_ = old_lb_ + (old_extent_) * (cnt_ - 1);	\
	ub_ = old_ub_;				\
    }                                                   \
}

/* MPID_DATATYPE_VECTOR_LB_UB()
 *
 * Determines the new LB and UB for a vector of blocks of old types
 * given the old type's LB, UB, and extent, and a count, stride, and
 * blocklen describing the vectorization.
 */
#define MPID_DATATYPE_VECTOR_LB_UB(cnt_,			\
				   stride_,			\
				   blklen_,			\
				   old_lb_,			\
				   old_ub_,			\
				   old_extent_,		\
				   lb_,			\
				   ub_)			\
{								\
    if (cnt_ == 0 || blklen_ == 0) {				\
	lb_ = old_lb_;					\
	ub_ = old_ub_;					\
    }								\
    else if (stride_ >= 0 && (old_extent_) >= 0) {		\
	lb_ = old_lb_;					\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1) +	\
	    (stride_) * ((cnt_) - 1);				\
    }								\
    else if (stride_ < 0 && (old_extent_) >= 0) {		\
	lb_ = old_lb_ + (stride_) * ((cnt_) - 1);		\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1);	\
    }								\
    else if (stride_ >= 0 && (old_extent_) < 0) {		\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1);	\
	ub_ = old_ub_ + (stride_) * ((cnt_) - 1);		\
    }								\
    else {							\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1) +	\
	    (stride_) * ((cnt_) - 1);				\
	ub_ = old_ub_;					\
    }								\
}

/* MPID_DATATYPE_BLOCK_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the LB,
 * UB, and extent of the old type as well as a new displacement and count
 * of types.
 *
 * Note: we need the extent here in addition to the lb and ub because the
 * extent might have some padding in it that we need to take into account.
 */
#define MPID_DATATYPE_BLOCK_LB_UB(cnt_,				\
				  disp_,				\
				  old_lb_,				\
				  old_ub_,				\
				  old_extent_,				\
				  lb_,					\
				  ub_)					\
{									\
    if (cnt_ == 0) {							\
	lb_ = old_lb_ + (disp_);					\
	ub_ = old_ub_ + (disp_);					\
    }									\
    else if (old_ub_ >= old_lb_) {					\
        lb_ = old_lb_ + (disp_);					\
        ub_ = old_ub_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
    }									\
    else /* negative extent */ {					\
	lb_ = old_lb_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
	ub_ = old_ub_ + (disp_);					\
    }									\
}

/* Datatype functions */
int MPID_Type_commit(MPI_Datatype *type);

int MPID_Type_dup(MPI_Datatype oldtype,
		  MPI_Datatype *newtype);

int MPID_Type_struct(int count,
		     int *blocklength_array,
		     MPI_Aint *displacement_array,
		     MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype);

int MPID_Type_indexed(int count,
		      int *blocklength_array,
		      void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype);

int MPID_Type_blockindexed(int count,
			   int blocklength,
			   void *displacement_array,
			   int dispinbytes,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype);

int MPID_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype);

int MPID_Type_contiguous(int count,
			 MPI_Datatype oldtype,
			 MPI_Datatype *newtype);

int MPID_Type_create_resized(MPI_Datatype oldtype,
			     MPI_Aint lb,
			     MPI_Aint extent,
			     MPI_Datatype *newtype);

int MPID_Type_get_envelope(MPI_Datatype datatype,
			   int *num_integers,
			   int *num_addresses,
			   int *num_datatypes,
			   int *combiner);

int MPID_Type_get_contents(MPI_Datatype datatype, 
			   int max_integers, 
			   int max_addresses, 
			   int max_datatypes, 
			   int array_of_integers[], 
			   MPI_Aint array_of_addresses[], 
			   MPI_Datatype array_of_datatypes[]);

int MPID_Type_create_pairtype(MPI_Datatype datatype,
                              MPID_Datatype *new_dtp);

/* internal debugging functions */
void MPIDI_Datatype_printf(MPI_Datatype type,
			   int depth,
			   MPI_Aint displacement,
			   int blocklength,
			   int header);

/* Dataloop functions */
void MPID_Dataloop_copy(void *dest,
			void *src,
			int size);

void MPID_Dataloop_print(struct MPID_Dataloop *dataloop,
			 int depth);

void MPID_Dataloop_alloc(int kind,
			 int count,
			 DLOOP_Dataloop **new_loop_p,
			 int *new_loop_sz_p);

void MPID_Dataloop_alloc_and_copy(int kind,
				  int count,
				  struct DLOOP_Dataloop *old_loop,
				  int old_loop_sz,
				  struct DLOOP_Dataloop **new_loop_p,
				  int *new_loop_sz_p);
void MPID_Dataloop_struct_alloc(int count,
				int old_loop_sz,
				int basic_ct,
				DLOOP_Dataloop **old_loop_p,
				DLOOP_Dataloop **new_loop_p,
				int *new_loop_sz_p);
void MPID_Dataloop_dup(DLOOP_Dataloop *old_loop,
		       int old_loop_sz,
		       DLOOP_Dataloop **new_loop_p);
void MPID_Dataloop_free(struct MPID_Dataloop **dataloop);

/* Segment functions */
void MPID_Segment_pack(struct DLOOP_Segment *segp,
		       DLOOP_Offset first,
		       DLOOP_Offset *lastp,
		       void *pack_buffer);

void MPID_Segment_unpack(struct DLOOP_Segment *segp,
			 DLOOP_Offset first,
			 DLOOP_Offset *lastp,
			 const void * unpack_buffer);

void MPID_Segment_pack_vector(struct DLOOP_Segment *segp,
			      DLOOP_Offset first,
			      DLOOP_Offset *lastp,
			      DLOOP_VECTOR *vector,
			      int *lengthp);

void MPID_Segment_unpack_vector(struct DLOOP_Segment *segp,
				DLOOP_Offset first,
				DLOOP_Offset *lastp,
				DLOOP_VECTOR *vector,
				int *lengthp);

void MPID_Segment_count_contig_blocks(struct DLOOP_Segment *segp,
				      DLOOP_Offset first,
				      DLOOP_Offset *lastp,
				      int *countp);

void MPID_Segment_flatten(struct DLOOP_Segment *segp,
			  DLOOP_Offset first,
			  DLOOP_Offset *lastp,
			  DLOOP_Offset *offp,
			  int *sizep,
			  DLOOP_Offset *lengthp);

/* misc */
int MPID_Datatype_set_contents(struct MPID_Datatype *ptr,
			       int combiner,
			       int nr_ints,
			       int nr_aints,
			       int nr_types,
			       int *ints,
			       MPI_Aint *aints,
			       MPI_Datatype *types);

void MPID_Datatype_free_contents(struct MPID_Datatype *ptr);

void MPID_Datatype_free(struct MPID_Datatype *ptr);

void MPID_Dataloop_update(struct DLOOP_Dataloop *dataloop,
			  MPI_Aint ptrdiff);

int MPIR_Type_get_contig_blocks(MPI_Datatype type,
				int *nr_blocks_p);

int MPIR_Type_flatten(MPI_Datatype type,
		      MPI_Aint *off_array,
		      int *size_array,
		      MPI_Aint *array_len_p);

void MPID_Segment_pack_external32(struct DLOOP_Segment *segp,
				  DLOOP_Offset first,
				  DLOOP_Offset *lastp, 
				  void *pack_buffer);

void MPID_Segment_unpack_external32(struct DLOOP_Segment *segp,
				    DLOOP_Offset first,
				    DLOOP_Offset *lastp,
				    DLOOP_Buffer unpack_buffer);

MPI_Aint MPID_Datatype_size_external32(MPI_Datatype type);
MPI_Aint MPIDI_Datatype_get_basic_size_external32(MPI_Datatype el_type);

/* debugging helper functions */
char *MPIDU_Datatype_builtin_to_string(MPI_Datatype type);
char *MPIDU_Datatype_combiner_to_string(int combiner);
void MPIDU_Datatype_debug(MPI_Datatype type, int array_ct);
/* end of file */
#endif
