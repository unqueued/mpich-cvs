#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	int i;
	double dStart, dFinish, dDuration;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &i);

	dStart = MPI_Wtime();
	sleep(1);
	dFinish = MPI_Wtime();
	dDuration = dFinish - dStart;

	printf("start:%g\nfinish:%g\nduration:%g\n", dStart, dFinish, dDuration);

	MPI_Finalize();
	return 0;
}
