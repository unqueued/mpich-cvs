/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Get with Fence";

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count; 
    MPI_Comm      comm;
    MPI_Win       win;
    MPI_Aint      extent;
    MTestDatatype sendtype, recvtype;

    MTest_Init( &argc, &argv );

    /* The following illustrates the use of the routines to 
       run through a selection of communicators and datatypes.
       Use subsets of these for tests that do not involve combinations 
       of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	/* Determine the sender and receiver */
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );
	source = 0;
	dest   = size - 1;
	
	for (count = 1; count < 65000; count = count * 2) {
	    while (MTestGetDatatypes( &sendtype, &recvtype, count )) {
		/* Make sure that everyone has a recv buffer */
		recvtype.InitBuf( &recvtype );
		sendtype.InitBuf( &sendtype );

		MPI_Type_extent( sendtype.datatype, &extent );
		MPI_Win_create( sendtype.buf, sendtype.count * extent, 
				extent, MPI_INFO_NULL, comm, &win );
		MPI_Win_fence( 0, win );
		if (rank == source) {
		    /* The source does not need to do anything besides the
		       fence */
		    MPI_Win_fence( 0, win );
		}
		else if (rank == dest) {
		    /* This should have the same effect, in terms of
		       transfering data, as a send/recv pair */
		    err = MPI_Get( recvtype.buf, recvtype.count, 
				   recvtype.datatype, source, 0, 
				   sendtype.count, sendtype.datatype, win );
		    if (err) {
			errs++;
			MTestPrintError( err );
		    }
		    MPI_Win_fence( 0, win );
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) {
			errs += errs;
		    }
		}
		else {
		    MPI_Win_fence( 0, win );
		}
		MTestFreeDatatype( &recvtype );
		MTestFreeDatatype( &sendtype );
		MPI_Win_free( &win );
	    }
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
