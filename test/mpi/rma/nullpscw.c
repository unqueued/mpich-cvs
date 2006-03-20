#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

int main(int argc, char* argv[])
{
  MPI_Win win;
  MPI_Group group;
  int errs = 0;

  MTest_Init(&argc,&argv); 

  MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
  MPI_Win_get_group(win, &group);
  
  MPI_Win_post(group, 0, win);
  MPI_Win_start(group, 0, win);
  
  MPI_Win_complete(win);
  
  MPI_Win_wait(win);

  MPI_Win_free(&win); 
  MTest_Finalize(errs);
  MPI_Finalize();
  return 0;
}
