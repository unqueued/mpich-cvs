/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

static int verbose = 0;

static int parse_args(int argc, char **argv);

int main( int argc, char *argv[] )
{
    int i, j, errs = 0, err;
    int rank, size;
    MPI_Datatype newtype;
    char *buf = NULL;

    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
	if (verbose) fprintf(stderr, "comm size must be > 1\n");
	errs++;
	goto fn_exit;
    }

    buf = malloc(64 * 129);
    if (buf == NULL) {
	if (verbose) fprintf(stderr, "error allocating buffer\n");
	errs++;
	goto fn_exit;
    }

    for (i = 8; i < 64; i += 4) {
	err = MPI_Type_vector(i, 128, 129, MPI_CHAR, &newtype);

	MPI_Type_commit(&newtype);
	memset(buf, 0, 64*129);

	if (rank == 0) {
	    /* init buffer */
	    for (j=0; j < i; j++) {
		int k;
		for (k=0; k < 129; k++) {
		    buf[j*k] = (char) j;
		}
	    }

	    /* send */
	    err = MPI_Send(buf, 1, newtype, 1, i, MPI_COMM_WORLD);
	}
	else if (rank == 1) {
	    /* recv */
	    err = MPI_Recv(buf, 1, newtype, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	    /* check buffer */
	    for (j=0; j < i; j++) {
		int k;
		for (k=0; k < 129; k++) {
		    if (k < 128 && buf[j*k] != (char) j) {
			if (verbose) fprintf(stderr,
					     "(i=%d, pos=%d) should be %d but is %d\n",
					     i, j*k, j, (int) buf[j*k]);
			errs++;
		    }
		    else if (k == 128 && buf[j*k] != (char) 0) {
			if (verbose) fprintf(stderr,
					     "(i=%d, pos=%d) should be %d but is %d\n",
					     i, j*k, 0, (int) buf[j*k]);
			errs++;
		    }
		}
	    }
	}

	MPI_Type_free(&newtype);
    }

 fn_exit:

    free(buf);
    /* print message and exit */
    if (errs) {
	fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
	printf("No errors\n");
    }
    MPI_Finalize();
    return 0;
}

static int parse_args(int argc, char **argv)
{
    int ret;

    while ((ret = getopt(argc, argv, "v")) >= 0)
    {
	switch (ret) {
	    case 'v':
		verbose = 1;
		break;
	}
    }
    return 0;
}
