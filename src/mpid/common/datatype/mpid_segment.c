/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpiimpl.h>
#include <mpid_dataloop.h>

#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

/* MPID_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our 
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 *
 * TODO: MOVE THIS OUT AND MAKE IT USE-DEPENDENT
 */
struct MPID_Segment_piece_params {
    union {
        struct {
            char *pack_buffer;
        } pack;
        struct {
            DLOOP_VECTOR *vectorp;
            int index;
            int length;
        } pack_vector;
        struct {
	    int64_t *offp;
	    int *sizep; /* see notes in Segment_flatten header */
            int index;
            int length;
        } flatten;
	struct {
	    char *last_loc;
	    int count;
	} contig_blocks;
        struct {
            char *unpack_buffer;
        } unpack;
        struct {
            int stream_off;
        } print;
    } u;
};

#define MPIDI_COPY_FROM_VEC(src,dest,stride,type,nelms,count)			\
do {										\
    type * l_src = (type *)src, * l_dest = (type *)dest;                    	\
    int i, j;									\
    const int l_stride = stride;						\
    if (nelms == 1) {								\
        for (i=count;i!=0;i--) {						\
            *l_dest++ = *l_src;							\
            l_src = (type *) ((char *) l_src + l_stride);			\
        }									\
    }										\
    else {									\
        for (i=count; i!=0; i--) {						\
            for (j=0; j<nelms; j++) {						\
                *l_dest++ = l_src[j];						\
	    }									\
            l_src = (type *) ((char *) l_src + l_stride);			\
        }									\
    }                                                                           \
    dest = (char *) l_dest;                                                     \
    src  = (char *) l_src;                                                      \
} while (0)

#define MPIDI_COPY_TO_VEC(src,dest,stride,type,nelms,count)			\
do {										\
    type * l_src = (type *)src, * l_dest = (type *)dest;                    	\
    int i, j;									\
    const int l_stride = stride;						\
    if (nelms == 1) {								\
        for (i=count;i!=0;i--) {						\
            *l_dest = *l_src++;							\
            l_dest = (type *) ((char *) l_dest + l_stride);			\
        }									\
    }										\
    else {									\
        for (i=count; i!=0; i--) {						\
            for (j=0; j<nelms; j++) {						\
                l_dest[j] = *l_src++;						\
	    }									\
            l_dest = (type *) ((char *) l_dest + l_stride);			\
        }									\
    }                                                                           \
    dest = (char *) l_dest;                                                     \
    src  = (char *) l_src;                                                      \
} while (0)

static int MPID_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
					   DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_unpack_to_buf(DLOOP_Offset *blocks_p,
                                             DLOOP_Type el_type,
					     DLOOP_Offset rel_off,
					     void *bufp,
					     void *v_paramp);

static int MPID_Segment_index_unpack_to_buf(DLOOP_Offset *blocks_p,
					    int count,
					    int *blockarray,
					    DLOOP_Offset *offsetarray,
                                            DLOOP_Type el_type,
					    DLOOP_Offset rel_off,
					    void *bufp,
					    void *v_paramp);

static int MPID_Segment_index_pack_to_buf(DLOOP_Offset *blocks_p,
					  int count,
					  int *blockarray,
					  DLOOP_Offset *offsetarray,
                                          DLOOP_Type el_type,
					  DLOOP_Offset rel_off,
					  void *bufp,
					  void *v_paramp);

static int MPID_Segment_vector_pack_to_buf(DLOOP_Offset *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_vector_unpack_to_buf(DLOOP_Offset *blocks_p,
					     int count,
					     int blksz,
					     DLOOP_Offset stride,
                                             DLOOP_Type el_type,
					     DLOOP_Offset rel_off,
					     void *bufp,
					     void *v_paramp);

static int MPID_Segment_contig_pack_to_buf(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_count_block(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_flatten(DLOOP_Offset *blocks_p,
                                       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp);

static int MPID_Segment_vector_flatten(DLOOP_Offset *blocks_p,
				       int count,
				       int blksz,
				       DLOOP_Offset stride,
                                       DLOOP_Type el_type,
				       DLOOP_Offset rel_off, /* offset into buffer */
				       void *bufp, /* start of buffer */
				       void *v_paramp);

/* Segment_pack - we need to implement this if for no other reason
 * than for performance testing
 *
 * Input Parameters:
 * segp - pointer to segment
 * pack_buffer - pointer to buffer to pack into
 * first - first byte index to be packed (or actually packed (?))
 *
 * InOut Parameters:
 * last - pointer to last byte index to be packed plus 1 (makes math easier)
 *
 * This and the similar functions all set up a piece_params structure that
 * they then pass to MPID_Segment_manipulate along with the function that 
 * they want called on each piece.  So in this case MPID_Segment_manipulate
 * will call MPID_Segment_piece_pack() on each piece of the buffer to pack,
 * where a piece is a basic datatype.
 *
 * Eventually we'll probably ditch this approach to gain some speed, but
 * for now it lets me have one function (_manipulate) that implements our
 * algorithm for parsing.
 *
 */
void MPID_Segment_pack(struct DLOOP_Segment *segp,
		       DLOOP_Offset first,
		       DLOOP_Offset *lastp, 
		       void *pack_buffer)
{
    struct MPID_Segment_piece_params pack_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_PACK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_PACK);

    pack_params.u.pack.pack_buffer = pack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_pack_to_buf, 
			    MPID_Segment_vector_pack_to_buf,
			    MPID_Segment_index_pack_to_buf,
                            NULL,
			    &pack_params);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_PACK);
    return;
}

/* MPID_Segment_pack_vector
 */
void MPID_Segment_pack_vector(struct DLOOP_Segment *segp,
			      DLOOP_Offset first,
			      DLOOP_Offset *lastp,
			      DLOOP_VECTOR *vectorp,
			      int *lengthp)
{
    struct MPID_Segment_piece_params packvec_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);

    packvec_params.u.pack_vector.vectorp = vectorp;
    packvec_params.u.pack_vector.index   = 0;
    packvec_params.u.pack_vector.length  = *lengthp;

    assert(*lengthp > 0);

    MPID_Segment_manipulate(segp,
			    first,
			    lastp, 
			    MPID_Segment_contig_pack_to_iov, 
			    MPID_Segment_vector_pack_to_iov,
			    NULL,
                            NULL,
			    &packvec_params);

    /* last value already handled by MPID_Segment_manipulate */
    *lengthp = packvec_params.u.pack_vector.index;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);
    return;
}

/* MPID_Segment_flatten
 *
 * offp    - pointer to array to fill in with offsets
 * sizep   - pointer to array to fill in with sizes
 * lengthp - pointer to value holding size of arrays; # used is returned
 *
 * Internally, index is used to store the index of next array value to fill in.
 *
 * TODO: MAKE SIZES Aints IN ROMIO, CHANGE THIS TO USE INTS TOO.
 */
void MPID_Segment_flatten(struct DLOOP_Segment *segp,
			  DLOOP_Offset first,
			  DLOOP_Offset *lastp,
			  DLOOP_Offset *offp,
			  int *sizep,
			  DLOOP_Offset *lengthp)
{
    struct MPID_Segment_piece_params packvec_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_FLATTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_FLATTEN);

    packvec_params.u.flatten.offp = (int64_t *) offp;
    packvec_params.u.flatten.sizep = sizep;
    packvec_params.u.flatten.index   = 0;
    packvec_params.u.flatten.length  = *lengthp;

    assert(*lengthp > 0);

    MPID_Segment_manipulate(segp,
			    first,
			    lastp, 
			    MPID_Segment_contig_flatten, 
			    MPID_Segment_vector_flatten,
			    NULL,
                            NULL,
			    &packvec_params);

    /* last value already handled by MPID_Segment_manipulate */
    *lengthp = packvec_params.u.flatten.index;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_FLATTEN);
    return;
}


/* MPID_Segment_count_contig_blocks()
 *
 * Count number of contiguous regions in segment between first and last.
 */
void MPID_Segment_count_contig_blocks(struct DLOOP_Segment *segp,
				      DLOOP_Offset first,
				      DLOOP_Offset *lastp,
				      DLOOP_Offset *countp)
{
    struct MPID_Segment_piece_params packvec_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_COUNT_CONTIG_BLOCKS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_COUNT_CONTIG_BLOCKS);

    packvec_params.u.contig_blocks.last_loc = NULL;
    packvec_params.u.contig_blocks.count    = 0;

    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_count_block,
			    NULL,
			    NULL,
                            NULL,
			    &packvec_params);

    *countp = packvec_params.u.contig_blocks.count;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_COUNT_CONTIG_BLOCKS);
}

/* Segment_unpack
 */
void MPID_Segment_unpack(struct DLOOP_Segment *segp,
			 DLOOP_Offset first,
			 DLOOP_Offset *lastp,
			 const DLOOP_Buffer unpack_buffer)
{
    struct MPID_Segment_piece_params unpack_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK);

    unpack_params.u.unpack.unpack_buffer = (DLOOP_Buffer) unpack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp, 
			    MPID_Segment_contig_unpack_to_buf,
			    MPID_Segment_vector_unpack_to_buf,
			    MPID_Segment_index_unpack_to_buf,
                            NULL,
			    &unpack_params);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK);
    return;
}

/* MPID_Segment_vector_pack_to_iov
 *
 * Input Parameters:
 * blocks_p - [inout] pointer to a count of blocks (total, for all noncontiguous pieces)
 * count    - # of noncontiguous regions
 * blksz    - size of each noncontiguous region
 * stride   - distance in bytes from start of one region to start of next
 * el_type - elemental type (e.g. MPI_INT)
 * ...
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int MPID_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   DLOOP_Type el_type,
					   DLOOP_Offset rel_off, /* offset into buffer */
					   void *bufp, /* start of buffer */
					   void *v_paramp)
{
    int i, basic_size;
    DLOOP_Offset size, blocks_left;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);

    basic_size = MPID_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[vector to vec: do=%d, dp=%x, ind=%d, sz=%d, blksz=%d, str=%d, blks=%d]\n",
		    (unsigned) rel_off,
		    (unsigned) bufp,
		    paramp->u.pack_vector.index,
		    basic_size,
		    blksz,
		    stride,
		    (int) *blocks_p);
#endif

    for (i=0; i < count && blocks_left > 0; i++) {
	if (blocks_left > blksz) {
	    size = blksz * basic_size;
	    blocks_left -= blksz;
	}
	else {
	    /* last pass */
	    size = blocks_left * basic_size;
	    blocks_left = 0;
	}

	if (paramp->u.pack_vector.index > 0 && ((char *) bufp + rel_off) ==
	    (((char *) paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_BUF) +
	     paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN))
	{
	    /* add this size to the last vector rather than using up another one */
	    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN += size;
	}
	else {
	    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_BUF = (char *) bufp + rel_off;
	    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_LEN = size;
	    paramp->u.pack_vector.index++;
	    /* check to see if we have used our entire vector buffer, and if so return 1 to stop processing */
	    if (paramp->u.pack_vector.index == paramp->u.pack_vector.length) {
		/* TODO: FIX SO THAT WE CONTINUE TO DO AGGREGATION ON LAST VECTOR */
		*blocks_p -= blocks_left;
		MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);
		return 1;
	    }
	}

	rel_off += stride;

    }
    assert(blocks_left == 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);
    return 0;
}

/* MPID_Segment_vector_flatten
 *
 * Notes: 
 * - this is only called when the starting position is at the beginning
 *   of a whole block in a vector type.
 * - this was a virtual copy of MPID_Segment_pack_to_iov; now it has improvements
 *   that MPID_Segment_pack_to_iov needs.
 * - we return the number of blocks that we did process in region pointed to by
 *   blocks_p.
 */
static int MPID_Segment_vector_flatten(DLOOP_Offset *blocks_p,
				       int count,
				       int blksz,
				       DLOOP_Offset stride,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off, /* offset into buffer */
				       void *bufp, /* start of buffer */
				       void *v_paramp)
{
    int i, basic_size;
    DLOOP_Offset size, blocks_left;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);

    basic_size = MPID_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

    for (i=0; i < count && blocks_left > 0; i++) {
	int index = paramp->u.flatten.index;

	if (blocks_left > blksz) {
	    size = blksz * (DLOOP_Offset) basic_size;
	    blocks_left -= blksz;
	}
	else {
	    /* last pass */
	    size = blocks_left * basic_size;
	    blocks_left = 0;
	}

	if (index > 0 && ((DLOOP_Offset) bufp + rel_off) ==
	    ((paramp->u.flatten.offp[index - 1]) + (DLOOP_Offset) paramp->u.flatten.sizep[index - 1]))
	{
	    /* add this size to the last region rather than using up another one */
	    paramp->u.flatten.sizep[index - 1] += size;
	}
	else if (index < paramp->u.flatten.length) {
	    /* take up another region */
	    paramp->u.flatten.offp[index]  = (DLOOP_Offset) bufp + rel_off;
	    paramp->u.flatten.sizep[index] = size;
	    paramp->u.flatten.index++;
	}
	else {
	    /* we tried to add to the end of the last region and failed; add blocks back in */
	    *blocks_p = *blocks_p - blocks_left + (size / basic_size);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);
	    return 1;
	}
	rel_off += stride;

    }
    assert(blocks_left == 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);
    return 0;
}

/* MPID_Segment_contig_pack_to_iov
 */
static int MPID_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int el_size;
    DLOOP_Offset size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);

    el_size = MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[contig to vec: do=%d, dp=%x, ind=%d, sz=%d, blksz=%d]\n",
		    (unsigned) rel_off,
		    (unsigned) bufp,
		    paramp->u.pack_vector.index,
		    el_size,
		    (int) *blocks_p);
#endif
    
    if (paramp->u.pack_vector.index > 0 && ((char *) bufp + rel_off) ==
	(((char *) paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_BUF) +
	 paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN))
    {
	/* add this size to the last vector rather than using up another one */
	paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN += size;
    }
    else {
	paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_BUF = (char *) bufp + rel_off;
	paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_LEN = size;
	paramp->u.pack_vector.index++;
	/* check to see if we have used our entire vector buffer, and if so return 1 to stop processing */
	if (paramp->u.pack_vector.index == paramp->u.pack_vector.length)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);
	    return 1;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);
    return 0;
}

/* MPID_Segment_contig_flatten
 */
static int MPID_Segment_contig_flatten(DLOOP_Offset *blocks_p,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp)
{
    int index, el_size;
    DLOOP_Offset size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);

    el_size = MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;
    index = paramp->u.flatten.index;

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[contig flatten: index = %d, loc = (%x + %x) = %x, size = %d]\n",
		    index,
		    (unsigned) bufp,
		    (unsigned) rel_off,
		    (unsigned) bufp + rel_off,
		    (int) size);
#endif
    
    if (index > 0 && ((DLOOP_Offset) bufp + rel_off) ==
	((paramp->u.flatten.offp[index - 1]) + (DLOOP_Offset) paramp->u.flatten.sizep[index - 1]))
    {
	/* add this size to the last vector rather than using up another one */
	paramp->u.flatten.sizep[index - 1] += size;
    }
    else {
	paramp->u.flatten.offp[index] = (int64_t) bufp + (int64_t) rel_off;
	paramp->u.flatten.sizep[index] = size;

	paramp->u.flatten.index++;
	/* check to see if we have used our entire vector buffer, and if so return 1 to stop processing */
	if (paramp->u.flatten.index == paramp->u.flatten.length)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);
	    return 1;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);
    return 0;
}

/* MPID_Segment_contig_count_block
 */
static int MPID_Segment_contig_count_block(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int el_size;
    DLOOP_Offset size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_COUNT_BLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_COUNT_BLOCK);

    el_size = MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;

#ifdef MPID_SP_VERBOSE
#if 0
    MPIU_dbg_printf("contig count block: count = %d, buf+off = %d, lastloc = %d\n",
		    (int) paramp->u.contig_blocks.count,
		    (int) ((char *) bufp + rel_off),
		    (int) paramp->u.contig_blocks.last_loc);
#endif
#endif

    if (paramp->u.contig_blocks.count > 0 && ((char *) bufp + rel_off) == paramp->u.contig_blocks.last_loc)
    {
	/* this region is adjacent to the last */
	paramp->u.contig_blocks.last_loc += size;
    }
    else {
	/* new region */
	paramp->u.contig_blocks.last_loc = (char *) bufp + rel_off + size;
	paramp->u.contig_blocks.count++;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_COUNT_BLOCK);
    return 0;
}

/* MPID_Segment_unpack_vector
 *
 * Q: Should this be any different from pack vector?
 */
void MPID_Segment_unpack_vector(struct DLOOP_Segment *segp,
				DLOOP_Offset first,
				DLOOP_Offset *lastp,
				DLOOP_VECTOR *vectorp,
				int *lengthp)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
    MPID_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
    return;
}

/* MPID_Segment_vector_unpack_to_buf
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int MPID_Segment_vector_unpack_to_buf(DLOOP_Offset *blocks_p,
					     int count,
					     int blksz,
					     DLOOP_Offset stride,
					     DLOOP_Type el_type,
					     DLOOP_Offset rel_off, /* offset into buffer */
					     void *bufp, /* start of buffer */
					     void *v_paramp)
{
    int i, basic_size;
    DLOOP_Offset blocks_left, whole_count;
    char *cbufp = (char *) bufp + rel_off;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_UNPACK_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_UNPACK_TO_BUF);

    basic_size = MPID_Datatype_get_basic_size(el_type);

    whole_count = *blocks_p / blksz;
    blocks_left = *blocks_p % blksz;

#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[vector unpack: do=%d, dp=%x, bp=%x, sz=%d, blksz=%d, str=%d, blks=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.unpack.unpack_buffer,
	       (int) basic_size,
	       (int) blksz,
	       (int) stride,
	       (int) *blocks_p);
#endif

    if (basic_size == 8) {
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, stride, int64_t, blksz, whole_count);
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int64_t, blocks_left, 1);
    }
    else if (basic_size == 4) {
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, stride, int32_t, blksz, whole_count);
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int32_t, blocks_left, 1);
    }
    else if (basic_size == 2) {
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, stride, int16_t, blksz, whole_count);
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int16_t, blocks_left, 1);
    }
    else {
	for (i=0; i < whole_count; i++) {
#ifdef MPID_SU_VERBOSE
	    dbg_printf("\t[vector unpack(1): do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
		       (int) (cbufp - (char *) bufp), 
		       (unsigned) bufp,
		       (unsigned) paramp->u.unpack.unpack_buffer,
		       basic_size,
		       (int) blksz * basic_size);
#endif
	    memcpy(cbufp, paramp->u.unpack.unpack_buffer, blksz * basic_size);
	    paramp->u.unpack.unpack_buffer += blksz * basic_size;
	    cbufp += stride;
	}
	if (blocks_left) {
#ifdef MPID_SU_VERBOSE
	    dbg_printf("\t[vector unpack(2): do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
		       (int) (cbufp - (char *) bufp), 
		       (unsigned) bufp,
		       (unsigned) paramp->u.unpack.unpack_buffer,
		       basic_size,
		       (int) blocks_left * basic_size);
#endif
	    memcpy(cbufp, paramp->u.unpack.unpack_buffer, blocks_left * basic_size);
	    paramp->u.unpack.unpack_buffer += blocks_left * basic_size;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_UNPACK_TO_BUF);
    return 0;
}

/* MPID_Segment_contig_unpack_to_buf
 */
static int MPID_Segment_contig_unpack_to_buf(DLOOP_Offset *blocks_p,
                                             DLOOP_Type el_type,
					     DLOOP_Offset rel_off,
					     void *bufp,
					     void *v_paramp)
{
    int el_size;
    DLOOP_Offset size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_TO_BUF);

    el_size = MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;

#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[contig unpack: do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.unpack.unpack_buffer,
	       el_size,
	       (int) *blocks_p);
#endif
    
    memcpy((char *) bufp + rel_off, paramp->u.unpack.unpack_buffer, size);
    paramp->u.unpack.unpack_buffer += size;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_TO_BUF);
    return 0;
}


/* MPID_Segment_vector_pack_to_buf
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int MPID_Segment_vector_pack_to_buf(DLOOP_Offset *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   DLOOP_Type el_type,
					   DLOOP_Offset rel_off, /* offset into buffer */
					   void *bufp, /* start of buffer */
					   void *v_paramp)
{
    int i, basic_size;
    DLOOP_Offset blocks_left, whole_count;
    char *cbufp = (char *) bufp + rel_off;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_BUF);

    basic_size = MPID_Datatype_get_basic_size(el_type);

    whole_count = *blocks_p / blksz;
    blocks_left = *blocks_p % blksz;

#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[vector pack: do=%d, dp=%x, bp=%x, sz=%d, blksz=%d, str=%d, blks=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.pack.pack_buffer,
	       (int) basic_size,
	       (int) blksz,
	       (int) stride,
	       (int) *blocks_p);
#endif

    if (basic_size == 8) {
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, stride, int64_t, blksz, whole_count);
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int64_t, blocks_left, 1);
    }
    else if (basic_size == 4) {
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, stride, int32_t, blksz, whole_count);
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int32_t, blocks_left, 1);
    }
    else if (basic_size == 2) {
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, stride, int16_t, blksz, whole_count);
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int16_t, blocks_left, 1);
    }
    else {
	for (i=0; i < whole_count; i++) {
#ifdef MPID_SP_VERBOSE
	    dbg_printf("\t[vector pack(1): do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
		       (int) (cbufp - (char *) bufp), 
		       (unsigned) bufp,
		       (unsigned) paramp->u.pack.pack_buffer,
		       basic_size,
		       (int) blksz * basic_size);
#endif
	    memcpy(paramp->u.pack.pack_buffer, cbufp, blksz * basic_size);
	    paramp->u.pack.pack_buffer += blksz * basic_size;
	    cbufp += stride;
	}
	if (blocks_left) {
#ifdef MPID_SP_VERBOSE
	    dbg_printf("\t[vector pack(2): do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
		       (int) (cbufp - (char *) bufp), 
		       (unsigned) bufp,
		       (unsigned) paramp->u.pack.pack_buffer,
		       basic_size,
		       (int) blocks_left * basic_size);
#endif
	    memcpy(paramp->u.pack.pack_buffer, cbufp, blocks_left * basic_size);
	    paramp->u.pack.pack_buffer += blocks_left * basic_size;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_BUF);
    return 0;
}


/* MPID_Segment_contig_pack_to_buf
 */
static int MPID_Segment_contig_pack_to_buf(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int el_size;
    DLOOP_Offset size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_BUF);

    el_size =MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * bp = pack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[contig pack: do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.pack.pack_buffer,
	       el_size,
	       (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */

    memcpy(paramp->u.pack.pack_buffer, (char *) bufp + rel_off, size);
    paramp->u.pack.pack_buffer += size;

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_BUF);
    return 0;
}

/* MPID_Segment_index_pack_to_buf
 */
static int MPID_Segment_index_pack_to_buf(DLOOP_Offset *blocks_p,
					  int count,
					  int *blockarray,
					  DLOOP_Offset *offsetarray,
                                          DLOOP_Type el_type,
					  DLOOP_Offset rel_off,
					  void *bufp,
					  void *v_paramp)
{
    int curblock = 0, el_size;
    DLOOP_Offset cur_block_sz, blocks_left = *blocks_p;
    char *cbufp;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    
    el_size = MPID_Datatype_get_basic_size(el_type);

#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[index pack: do=%d, dp=%x, bp=%x, sz=%d (%s), cnt=%d, blks=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.pack.pack_buffer,
	       (int) el_size,
	       MPIDU_Datatype_builtin_to_string(el_type),
	       (int) count,
	       (int) *blocks_p);
#endif

    while (blocks_left) {
	assert(curblock < count);
	cur_block_sz = blockarray[curblock];
	cbufp = (char *) bufp + rel_off + offsetarray[curblock];

	if (cur_block_sz > blocks_left) cur_block_sz = blocks_left;

#ifdef MPID_SP_VERBOSE
	    dbg_printf("\t[index pack(1): do=%d, dp=%x, bp=%x, sz=%d, blksz=%d]\n",
		       (int) (cbufp - (char *) bufp), 
		       (unsigned) bufp,
		       (unsigned) paramp->u.pack.pack_buffer,
		       el_size,
		       (int) cur_block_sz * el_size);
#endif
	if (el_size == 8) {
	    /* note: macro updates pack buffer location */
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int64_t, cur_block_sz, 1);
	}
	else if (el_size == 4) {
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int32_t, cur_block_sz, 1);
	}
	else if (el_size == 2) {
	    MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int16_t, cur_block_sz, 1);
	}
	else {
	    DLOOP_Offset size = cur_block_sz * el_size;

	    memcpy(paramp->u.pack.pack_buffer, cbufp, size);
	    paramp->u.pack.pack_buffer += size;
	}
	blocks_left -= cur_block_sz;
	curblock++;
    }
    return 0;
}

/* MPID_Segment_index_unpack_to_buf
 */
static int MPID_Segment_index_unpack_to_buf(DLOOP_Offset *blocks_p,
					    int count,
					    int *blockarray,
					    DLOOP_Offset *offsetarray,
					    DLOOP_Type el_type,
					    DLOOP_Offset rel_off,
					    void *bufp,
					    void *v_paramp)
{
    int curblock = 0, el_size;
    DLOOP_Offset cur_block_sz, blocks_left = *blocks_p;
    char *cbufp = (char *) bufp + rel_off;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    
    el_size = MPID_Datatype_get_basic_size(el_type);

    while (blocks_left) {
	cur_block_sz = blockarray[curblock];
	cbufp = (char *) bufp + rel_off + offsetarray[curblock];

	if (cur_block_sz > blocks_left) cur_block_sz = blocks_left;

	if (el_size == 8) {
	    /* note: macro updates pack buffer location */
	    MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int64_t, cur_block_sz, 1);
	}
	else if (el_size == 4) {
	    MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int32_t, cur_block_sz, 1);
	}
	else if (el_size == 2) {
	    MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int16_t, cur_block_sz, 1);
	}
	else {
	    DLOOP_Offset size = cur_block_sz * el_size;
	    memcpy(cbufp, paramp->u.unpack.unpack_buffer, size);
	    paramp->u.unpack.unpack_buffer += size;
	}
	blocks_left -= cur_block_sz;
	curblock++;
    }
    return 0;
}






