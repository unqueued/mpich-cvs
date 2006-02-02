/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file contains the prototypes for routines that are used with 
   "external" modules such as ROMIO.  These allow the different packages to 
   hide their internal datatypes from one another */

#ifndef MPIEXT_H_INCLUDED
#define MPIEXT_H_INCLUDED

/* This routine, given an MPI_Errhandler (from a file), returns
   a pointer to the user-supplied error function.  The last argument
   is set to an integer indicating that the function is MPI_ERRORS_RETURN 
   (value == 1), MPI_ERRORS_ARE_FATAL (value == 0), or a valid user-function
   (value == 2) 

   This routine is implemented in mpich2/src/mpi/errhan/file_set_errhandler.c
*/
void MPIR_Get_file_error_routine( MPI_Errhandler, 
				  void (**)(MPI_File *, int *, ...), 
				  int * );

/* 
   These routines provide access to the MPI_Errhandler field within the 
   ROMIO MPI_File structure
 */
int MPIR_ROMIO_Get_file_errhand( MPI_File, MPI_Errhandler * );
int MPIR_ROMIO_Set_file_errhand( MPI_File, MPI_Errhandler );

/* FIXME: This routine is also defined in adio.h */
int MPIO_Err_return_file( MPI_File, int );

#endif
