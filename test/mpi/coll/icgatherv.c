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

static char MTEST_Descrip[] = "Simple intercomm gatherv test";

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int *buf = 0;
    int *recvcounts;
    int *recvdispls;
    int leftGroup, i, count, rank, rsize;
    MPI_Comm comm;
    MPI_Datatype datatype;

    MTest_Init( &argc, &argv );

    datatype = MPI_INT;
    while (MTestGetIntercomm( &comm, &leftGroup, 4 )) {
	if (comm == MPI_COMM_NULL) continue;
	for (count = 1; count < 65000; count = 2 * count) {
	    /* Get an intercommunicator */
	    MPI_Comm_remote_size( comm, &rsize );
	    recvcounts = (int *)malloc( rsize * sizeof(int) );
	    recvdispls = (int *)malloc( rsize * sizeof(int) );
	    /* This simple test duplicates the Gather test, 
	       using the same lengths for all messages */
	    for (i=0; i<rsize; i++) {
		recvcounts[i] = count;
		recvdispls[i] = count * i;
	    }
	    if (leftGroup) {
		MPI_Comm_rank( comm, &rank );
		buf = (int *)malloc( count * rsize * sizeof(int) );
		for (i=0; i<count*rsize; i++) buf[i] = -1;

		err = MPI_Gatherv( NULL, 0, datatype,
				  buf, recvcounts, recvdispls, datatype, 
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
		else {
		    /* Check for the correct data */
		    for (i=0; i<count*rsize; i++) {
			if (buf[i] != i) {
			    errs++;
			}
		    }
		}
	    }
	    else {
		int size;
		/* In the right group */
		MPI_Comm_rank( comm, &rank );
		MPI_Comm_size( comm, &size );
		buf = (int *)malloc( count * sizeof(int) );
		for (i=0; i<count; i++) buf[i] = rank * count + i;
		err = MPI_Gatherv( buf, count, datatype, 
				   NULL, 0, 0, datatype, 0, comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
	    }
	    free( buf );
	    free( recvcounts );
	    free( recvdispls );
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
