/* based on the pack.c test in the mpich suite.
 */

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mpi.h"

static int verbose = 0;

#define BUF_SIZE 16384

int main(int argc, char *argv[]);
int parse_args(int argc, char **argv);

int main(int argc, char *argv[])
{
    int errs = 0;
    char buffer[BUF_SIZE];
    int n, size;
    double a,b;
    int pos;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    pos	= 0;
    n	= 10;
    a	= 1.1;
    b	= 2.2;

    MPI_Pack(&n, 1, MPI_INT, buffer, BUF_SIZE, &pos, MPI_COMM_WORLD);
    MPI_Pack(&a, 1, MPI_DOUBLE, buffer, BUF_SIZE, &pos, MPI_COMM_WORLD);
    MPI_Pack(&b, 1, MPI_DOUBLE, buffer, BUF_SIZE, &pos, MPI_COMM_WORLD);

#if 0
    /* not a valid test for all MPI implementations */
    if ( *(int *) buffer != 10) {
    	errs++;
	if (verbose) fprintf(stderr, "Wrong value for n in packbuf.\n");
    }
#endif

    size = pos;
    pos  = 0;
    n    = 0;
    a    = 0;
    b    = 0;

    MPI_Unpack(buffer, size, &pos, &n, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Unpack(buffer, size, &pos, &a, 1, MPI_DOUBLE, MPI_COMM_WORLD);
    MPI_Unpack(buffer, size, &pos, &b, 1, MPI_DOUBLE, MPI_COMM_WORLD);
    /* Check results */
    if (n != 10) { 
	errs++;
	if (verbose) fprintf(stderr, "Wrong value for n; got %d expected %d\n", n, 10 );
    }
    if (a != 1.1) { 
	errs++;
	if (verbose) fprintf(stderr, "Wrong value for a; got %f expected %f\n", a, 1.1 );
    }
    if (b != 2.2) { 
	errs++;
	if (verbose) fprintf(stderr, "Wrong value for b; got %f expected %f\n", b, 2.2 );
    }

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

int parse_args(int argc, char **argv)
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
