/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
 * This program tests that MPI_Comm_create applies to intercommunicators;
 * this is an extension added in MPI-2
 */

int TestIntercomm( MPI_Comm );

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, isLeft, wrank;
    int count = 0;
    MPI_Comm intercomm, newcomm;
    MPI_Group oldgroup, newgroup;
    
    MTest_Init( &argc, &argv );
    
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (size < 4) {
	printf( "This test requires at least 4 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    while (count++ < 1 && MTestGetIntercomm( &intercomm, &isLeft, 2 )) {
	int ranks[10], nranks, result;

        MPI_Comm_group( intercomm, &oldgroup );
	ranks[0] = 0;
	nranks   = 1;
	MTestPrintfMsg( 1, "Creating a new intercomm 0-0\n" );
	MPI_Group_incl( oldgroup, nranks, ranks, &newgroup );
	MPI_Comm_create( intercomm, newgroup, &newcomm );

	/* Make sure that the new communicator has the appropriate pieces */
	if (newcomm != MPI_COMM_NULL) {
	    int new_rsize, new_size, flag, commok = 1;;

	    MPI_Comm_set_name( newcomm, "Single rank in each group" );
	    MPI_Comm_test_inter( intercomm, &flag );
	    if (!flag) {
		errs++;
		printf( "[%d] Output communicator is not an intercomm\n",
			wrank );
		commok = 0;
	    }

	    MPI_Comm_remote_size( newcomm, &new_rsize );
	    MPI_Comm_size( newcomm, &new_size );
	    /* The new communicator has 1 process in each group */
	    if (new_rsize != 1) {
		errs++;
		printf( "[%d] Remote size is %d, should be one\n", 
			wrank, new_rsize );
		commok = 0;
	    }
	    if (new_size != 1) {
		errs++;
		printf( "[%d] Local size is %d, should be one\n", 
			wrank, new_size );
		commok = 0;
	    }
	    /* ... more to do */
	    if (commok) {
		errs += TestIntercomm( newcomm );
	    }
	}
	MPI_Group_free( &newgroup );
	if (newcomm != MPI_COMM_NULL) {
	    MPI_Comm_free( &newcomm );
	}

	/* Now, do a sort of dup, using the original group */
	MTestPrintfMsg( 1, "Creating a new intercomm (manual dup)\n" );
	MPI_Comm_create( intercomm, oldgroup, &newcomm );
	MPI_Comm_set_name( newcomm, "Dup of original" );
	MTestPrintfMsg( 1, "Creating a new intercomm (manual dup (done))\n" );

	MPI_Comm_compare( intercomm, newcomm, &result );
	MTestPrintfMsg( 1, "Result of comm/intercomm compare is %d\n", result );
	if (result != MPI_CONGRUENT) {
	    const char *rname=0;
	    errs++;
	    switch (result) {
	    case MPI_IDENT:     rname = "IDENT"; break;
	    case MPI_CONGRUENT: rname = "CONGRUENT"; break;
	    case MPI_SIMILAR:   rname = "SIMILAR"; break;
	    case MPI_UNEQUAL:   rname = "UNEQUAL"; break;
	    printf( "[%d] Expected MPI_CONGRUENT but saw %d (%s)", 
		    wrank, result, rname ); fflush(stdout);
	    }
	}
	else {
	    /* Try to communication between each member of intercomm */
	    errs += TestIntercomm( newcomm );
	}

	MPI_Group_free( &oldgroup );
	MPI_Comm_free( &newcomm );
	MPI_Comm_free( &intercomm );
    }

    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}

int TestIntercomm( MPI_Comm comm )
{
    int local_size, remote_size, rank, **bufs, *bufmem, rbuf[2], j;
    int errs = 0, wrank, nsize;
    char commname[MPI_MAX_OBJECT_NAME+1];
    MPI_Request *reqs;

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( comm, &local_size );
    MPI_Comm_remote_size( comm, &remote_size );
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_get_name( comm, commname, &nsize );

    MTestPrintfMsg( 1, "Testing communication on intercomm %s\n", commname );
    
    reqs = (MPI_Request *)malloc( remote_size * sizeof(MPI_Request) );
    if (!reqs) {
	printf( "[%d] Unable to allocated %d requests for testing intercomm %s\n", 
		wrank, remote_size, commname );
	errs++;
	return errs;
    }
    bufs = (int **) malloc( remote_size * sizeof(int *) );
    if (!bufs) {
	printf( "[%d] Unable to allocated %d int pointers for testing intercomm %s\n", 
		wrank, remote_size, commname );
	errs++;
	return errs;
    }
    bufmem = (int *) malloc( remote_size * 2 * sizeof(int) );
    if (!bufmem) {
	printf( "[%d] Unable to allocated %d int data for testing intercomm %s\n", 
		wrank, 2*remote_size, commname );
	errs++;
	return errs;
    }

    /* Each process sends a message containing its own rank and the
       rank of the destination with a nonblocking send.  Because we're using
       nonblocking sends, we need to use different buffers for each isend */
    for (j=0; j<remote_size; j++) {
	bufs[j]    = &bufmem[2*j];
	bufs[j][0] = rank;
	bufs[j][1] = j;
	MPI_Isend( bufs[j], 2, MPI_INT, j, 0, comm, &reqs[j] );
    }

    for (j=0; j<remote_size; j++) {
	MPI_Recv( rbuf, 2, MPI_INT, j, 0, comm, MPI_STATUS_IGNORE );
	if (rbuf[0] != j) {
	    printf( "[%d] Expected rank %d but saw %d in %s\n", 
		    wrank, j, rbuf[0], commname );
	    errs++;
	}
	if (rbuf[1] != rank) {
	    printf( "[%d] Expected target rank %d but saw %d from %d in %s\n", 
		    wrank, rank, rbuf[1], j, commname );
	    errs++;
	}
    }
    if (errs) 
	fflush(stdout);
    MPI_Waitall( remote_size, reqs, MPI_STATUSES_IGNORE );

    free( reqs );
    free( bufs );
    free( bufmem );

    return errs;
}
