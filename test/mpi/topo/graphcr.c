/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Create a communicator with a graph that contains no processes";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *index = 0, *edges = 0;
    MPI_Comm comm;

    MTest_Init( &argc, &argv );

    MPI_Graph_create( MPI_COMM_WORLD, 0, index, edges, 0, &comm );
    if (comm != MPI_COMM_NULL) {
	errs++;
	fprintf( stderr, "Expected MPI_COMM_NULL from empty graph create\n" );
    }
	
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
