/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Simple intercomm broadcast test";

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int *buf = 0;
    int leftGroup, i, count, rank;
    MPI_Comm comm;
    MPI_Datatype datatype;

    MTest_Init( &argc, &argv );

    datatype = MPI_INT;
    while (MTestGetIntercomm( &comm, &leftGroup, 4 )) {
	for (count = 1; count < 65000; count = 2 * count) {
	    buf = (int *)malloc( count * sizeof(int) );
	    /* Get an intercommunicator */
	    if (leftGroup) {
		MPI_Comm_rank( comm, &rank );
		if (rank == 0) {
		    for (i=0; i<count; i++) buf[i] = i;
		}
		else {
		    for (i=0; i<count; i++) buf[i] = -1;
		}
		err = MPI_Bcast( buf, count, datatype, 
				 (rank == 0) ? MPI_ROOT : MPI_PROC_NULL,
				 comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
		/* Test that no other process in this group received the 
		   broadcast */
		if (rank != 0) {
		    for (i=0; i<count; i++) {
			if (buf[i] != -1) {
			    errs++;
			}
		    }
		}
	    }
	    else {
		/* In the right group */
		for (i=0; i<count; i++) buf[i] = -1;
		err = MPI_Bcast( buf, count, datatype, 0, comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
		/* Check that we have received the correct data */
		for (i=0; i<count; i++) {
		    if (buf[i] != i) {
			errs++;
		    }
		}
	    }
	free( buf );
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
