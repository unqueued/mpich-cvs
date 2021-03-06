/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include "mpitestconf.h"

int verbose = 0;

int handlerCalled = 0;

/* Prototype to suppress compiler warnings */
void user_handler( MPI_File *fh, int *errcode, ... );

void user_handler( MPI_File *fh, int *errcode, ... )
{
    if (*errcode != MPI_SUCCESS) {
	handlerCalled++;
	if (verbose) {
	    printf( "In user_handler with code %d\n", *errcode );
	}
    }
}

int main( int argc, char *argv[] )
{
    int            rank, errs = 0, rc;
    MPI_Errhandler ioerr_handler;
    MPI_Status     status;
    MPI_File       fh;
    char           inbuf[80];

    MTest_Init( &argc, &argv );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    /* Create a file to which to attach the handler */
    rc = MPI_File_open( MPI_COMM_WORLD, "test.txt", 
		MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_DELETE_ON_CLOSE, 
			MPI_INFO_NULL, &fh );
    if (rc) {
	errs ++;
	printf( "Unable to open test.txt for writing\n" );
    }

    rc = MPI_File_create_errhandler( user_handler, &ioerr_handler );
    rc = MPI_File_set_errhandler( fh, ioerr_handler );

    /* This should generate an error because the file mode is WRONLY */
    rc = MPI_File_read_at( fh, 0, inbuf, 80, MPI_BYTE, &status );
    if (handlerCalled != 1) {
	errs++;
	printf( "User-defined error handler was not called\n" );
    }

    MPI_File_close( &fh );
    
    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
