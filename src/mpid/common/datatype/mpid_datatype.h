/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPID_DATATYPE_H
#define MPID_DATATYPE_H

/* NOTE: 
 * - struct MPID_Datatype is defined in src/include/mpiimpl.h.
 * - struct MPID_Dataloop and MPID_Segment are defined in 
 *   src/mpid/common/datatype/mpid_dataloop.h (and gen_dataloop.h).
 */

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#define MPID_VECTOR         WSABUF
#define MPID_VECTOR_LEN     len
#define MPID_VECTOR_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_VECTOR         struct iovec
#define MPID_VECTOR_LEN     iov_len
#define MPID_VECTOR_BUF     iov_base
#endif
#define MPID_VECTOR_LIMIT   16

#define MPID_DTYPE_BEGINNING  0
#define MPID_DTYPE_END       -1

/* Datatype functions */
int MPID_Type_vector(int count,
		     int blocklength,
		     int stride,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype);

/* Dataloop functions */

typedef struct MPID_Dataloop * MPID_Dataloop_foo; /* HACK */

void MPID_Dataloop_copy(void *dest,
			void *src,
			MPI_Datatype handle,
			int size);

void MPID_Dataloop_print(struct MPID_Dataloop *dataloop,
			 int depth);

struct MPID_Dataloop * MPID_Dataloop_alloc(void);

void MPID_Dataloop_free(struct MPID_Dataloop *dataloop);

/* Segment functions */

typedef struct MPID_Segment * MPID_Segment_foo; /* HACK */

struct MPID_Segment * MPID_Segment_alloc(void);

void MPID_Segment_free(struct MPID_Segment *segp);

int MPID_Segment_init(const void *buf,
		      int count,
		      MPI_Datatype handle,
		      struct MPID_Segment *segp);

void MPID_Segment_pack(struct MPID_Segment *segp,
		       int first,
		       int *lastp,
		       void *pack_buffer);

void MPID_Segment_unpack(struct MPID_Segment *segp,
			 int first,
			 int *lastp,
			 const void * unpack_buffer);

void MPID_Segment_pack_vector(struct MPID_Segment *segp,
			      int first,
			      int *lastp,
			      MPID_VECTOR *vector,
			      int *lengthp);

void MPID_Segment_unpack_vector(struct MPID_Segment *segp,
				int first,
				int *lastp,
				MPID_VECTOR *vector,
				int *lengthp);
#endif
