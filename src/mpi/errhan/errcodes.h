/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ERRCODES_INCLUDED
#define ERRCODES_INCLUDED

/* Prototypes for internal routines for the errhandling module */

/* Routine to return the error message for an error code.  Null on failure */
void MPIR_Err_get_string( int, char * );
int MPIR_Err_set_msg( int, const char * );
int MPIR_Err_add_class( const char * );
int MPIR_Err_add_code( int, const char * );
void MPIR_Err_delete_code( int );
void MPIR_Err_delete_class( int );

/* 
   This file contains the definitions of the error code fields
   
   An error code is organized as

   is-fatal? specific-msg-sequence# specific-msg-index generic-code is-dynamic? class

   where
   class: The MPI error class (including dynamically defined classes)
   is-dynamic?: Set if this is a dynamically created error code (using
   the routines to add error classes and codes at runtime)
   generic-code: Index into the array of generic messages
   specific-msg-index: index to the *buffer* containing recent 
   instance-specific error messages.
   specific-msg-sequence#: A sequence number used to check that the specific
   message text is still valid.
   is-fatal?: the error is fatal and should not be returned to the user
 */

#define ERROR_CLASS_MASK          0x0000003F
#define ERROR_DYN_MASK            0x00000040
#define ERROR_DYN_SHIFT           6
#define ERROR_GENERIC_MASK        0x0007FF80
#define ERROR_GENERIC_SHIFT       7
#define ERROR_SPECIFIC_INDEX_MASK 0x00F80000
#define ERROR_SPECIFIC_INDEX_SHIFT 19
#define ERROR_SPECIFIC_SEQ_MASK   0x3F000000
/* Size is size of field as an integer, not the number of bits */
#define ERROR_SPECIFIC_SEQ_SIZE   64
#define ERROR_SPECIFIC_SEQ_SHIFT  24
#define ERROR_FATAL_MASK          0x80000000
#define ERROR_GET_CLASS( code ) ((code) & ERROR_CLASS_MASK)

/* These must correspond to the masks defined above */
#define ERROR_MAX_NCLASS 64
#define ERROR_MAX_NCODE  8192
#endif
