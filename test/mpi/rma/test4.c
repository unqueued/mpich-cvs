#include "mpi.h" 
#include "stdio.h"

/* tests passive target RMA on 2 processes. tests the lock-single_op-unlock 
   optimization. */

#define SIZE1 20
#define SIZE2 40

int main(int argc, char *argv[]) 
{ 
    int rank, nprocs, A[SIZE2], B[SIZE2], i;
    MPI_Win win;
 
    MPI_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    if (rank == 0) {
        for (i=0; i<SIZE2; i++) A[i] = B[i] = i;
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win); 

        for (i=0; i<SIZE1; i++) {
            MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
            MPI_Put(A+i, 1, MPI_INT, 1, i, 1, MPI_INT, win);
            MPI_Win_unlock(1, win);
        }

        for (i=0; i<SIZE1; i++) {
            MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
            MPI_Get(B+i, 1, MPI_INT, 1, SIZE1+i, 1, MPI_INT, win);
            MPI_Win_unlock(1, win);
        }

        MPI_Win_free(&win);

        for (i=0; i<SIZE1; i++) 
            if (B[i] != (-4)*(i+SIZE1)) 
            printf("Get Error: B[%d] is %d, should be %d\n", i, B[i], (-4)*(i+SIZE1));
    }
 
    else {  /* rank=1 */
        for (i=0; i<SIZE2; i++) B[i] = (-4)*i;
        MPI_Win_create(B, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, 
                       MPI_COMM_WORLD, &win);

        for (i=0; i<100; i++) {
            usleep(1000);
            MPIDI_CH3I_Progress(0);
        }

        MPI_Win_free(&win); 
        
        for (i=0; i<SIZE1; i++) {
            if (B[i] != i)
                printf("Put Error: B[%d] is %d, should be %d\n", i, B[i], i);
        }
    }

    if (rank==0) printf("Done\n");
    MPI_Finalize(); 
    return 0; 
} 
