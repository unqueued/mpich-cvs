/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include <stdio.h>

/* This is the utility file for info that contains routines used to 
   manage the arrays used to store handle objects.  

   To use these routines, allocate the following in a utility file 
   used by each object (e.g., info, datatype, comm, group, ... ).  
   (The comment format // is used in this example but the usual 
   C comment convention should be used of course):

   // Preallocated objects
   MPID_<obj> MPID_<obj>_direct[MPID_<OBJ>_PREALLOC];
   static int initialized = 0;
   
   // Next available object 
   static int MPID_<obj> *avail = 0;
   
   // Extension (indirect) objects
   static MPID_<obj> *(*MPID_<obj>_indirect)[] = 0;
   static int MPID_<obj>_indirect_size = 0;

   The following routines are provided:
   void *MPIU_Handle_direct_init( void *direct, int direct_size, int obj_size,
                                  int handle_type )
        Initialize the preallocated array (MPID_<obj>_direct) with
        direct_size elements each of obj_size.  Returns the first available
        element (which should usually be assigned to "avail").
	handle_type is the kind of object (e.g., MPID_INFO)

   void *MPIU_Handle_indirect_init( void (**indirect)[], int *indirect_size, 
                                    int indirect_max_size,
                                    int indirect_block_size, int obj_size,
                                    int handle_type )
	Initialize the indirect array (MPID_<obj>_indirect) of size
        indirect_size, each block of which contains indirect_block_size
	members of size obj_size.  Returns the first available element, or
	NULL if no memory is available.  
        Also incrementes indirect_size and assigns to indirect if it is null.

   int MPIU_Handle_free( void *(*indirect)[], int indirect_size )
        Frees any memory allocated for the indirect handles.  Returns 0 on
	success and nonzero on failure

   None of these routines is thread-safe.  Any routine that uses them 
   must ensure that only one thread at a time may call them.  
   
*/

/*
 * You can use this to allocated that necessary local structures
 */
#define MPID_HANDLE_MEM_ALLOC(Name,NAME) \
MPID_##Name MPID_##Name_direct[MPID_##NAME##_PREALLOC]; \
static int initialize = 0;\
static int MPID_##Name *avail=0;\
static MPID_##Name *(*MPID_##Name##_indirect)[] = 0;\
static int MPID_##Name##_indirect_size = 0;

#ifdef FOO
/* 
 * Get an pointer to dynamically allocated storage for Info objects.
 * This has an MPID prefix to simplify the "get object" routines
 */
MPID_Info *MPID_Info_Get_ptr_indirect( int handle )
{
    int block_num, index_num;

    /* Check for a valid handle type */
    if (HANDLE_GET_KIND(handle) != MPID_INFO) {
	return 0;
    }

    /* Find the block */
    block_num = HANDLE_BLOCK(handle);
    if (block_num >= MPID_Info_indirect_size) {
	return 0;
    }
    
    /* Find the entry */
    index_num = HANDLE_BLOCK_INDEX(handle);
    return &(*MPID_Info_indirect)[block_num][index_num];
}
#endif

/* This routine is called by finalize when MPI exits */
int MPIU_Handle_free( void *((*indirect)[]), int indirect_size )
{
    int i;
    
    /* Remove any allocated storage */
    for (i=0; i<indirect_size; i++) {
	MPIU_Free( (*indirect)[i] );
    }
    if (indirect) {
	MPIU_Free( indirect );
    }
    /* This does *not* remove any objects that the user created 
       and then did not destroy */
    return 0;
}

void *MPIU_Handle_direct_init( void *direct, int direct_size, int obj_size, 
			       int handle_type )
{
    int                i;
    MPIU_Handle_common *hptr=0;
    char               *ptr = (char *)direct;
    
    for (i=0; i<direct_size; i++) {
	hptr = (MPIU_Handle_common *)ptr;
	ptr  = ptr + obj_size;
	hptr->next = ptr;
	hptr->id   = (HANDLE_KIND_DIRECT << 30) | (handle_type << 27) | i;
	}
    hptr->next = 0;
    return direct;
}

/* indirect is really a pointer to a pointer to an array of pointers */
void *MPIU_Handle_indirect_init( void *(**indirect)[], int *indirect_size, 
				 int indirect_max_size,
				 int indirect_block_size, int obj_size, 
				 int handle_type )
{
    void               *block_ptr;
    MPIU_Handle_common *hptr=0;
    char               *ptr;
    int                i;

    /* Must create new storage for dynamically allocated objects */
    /* Create the table */
    if (!*indirect) {
	/* printf( "Creating indirect table\n" ); */
	*indirect = (void *)MPIU_Calloc( indirect_max_size, 
					      sizeof(void *) );
	if (!*indirect) {
	    return 0;
	}
	*indirect_size = 0;
    }

    /* See if we can allocate another block */
    if (*indirect_size >= indirect_max_size-1) {
	return 0;
    }
    
    /* Create the next block */
    /* printf( "Adding indirect block %d\n", MPID_Info_indirect_size ); */
    block_ptr = (void *)MPIU_Calloc( indirect_block_size, obj_size );
    if (!block_ptr) { 
	return 0;
    }
    ptr = (char *)block_ptr;
    for (i=0; i<indirect_block_size; i++) {
	hptr       = (MPIU_Handle_common *)ptr;
	ptr        = ptr + obj_size;
	hptr->next = ptr;
	hptr->id   = (HANDLE_KIND_INDIRECT << 30) | (handle_type << 27) | 
	    (*indirect_size << 16) | i;
    }
    hptr->next = 0;
    /* We're here because avail is null, so there is no need to set 
       the last block ptr to avail */
    /* printf( "loc of update is %x\n", &(**indirect)[*indirect_size] );  */
    (**indirect)[*indirect_size] = block_ptr;
    *indirect_size = *indirect_size + 1;
    return block_ptr;
}

/*
  Create and return a pointer to an info object.  Returns null if there is 
  an error such as out-of-memory.  Does not allocate space for the
  key or value.

 */

/* For debugging */
void MPIU_Print_handle( int handle )
{
    int type, kind, block, index;

    type = HANDLE_GET_MPI_KIND(handle);
    kind = HANDLE_GET_KIND(handle);
    switch (type) {
    case HANDLE_KIND_INVALID:
	printf( "invalid" );
	break;
    case HANDLE_KIND_BUILTIN:
	printf( "builtin" );
	break;
    case HANDLE_KIND_DIRECT:
	index = HANDLE_INDEX(handle);
	printf( "direct: %d", index );
	break;
    case HANDLE_KIND_INDIRECT:
	block = HANDLE_BLOCK(handle);
	index = HANDLE_BLOCK_INDEX(handle);
	printf( "indirect: block %d index %d", block, index );
	break;
    }
}
