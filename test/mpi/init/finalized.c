/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    int error;
    int flag;
    char err_string[1024];
    int length = 1024;
    int rank;

    flag = 0;
    error = MPI_Finalized(&flag);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalized failed: %s\n", err_string);
	fflush(stdout);
	return 1;
    }
    if (flag)
    {
	printf("MPI_Finalized returned true before MPI_Init.\n");
	return 1;
    }

    error = MPI_Init(&argc, &argv);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Init failed: %s\n", err_string);
	fflush(stdout);
	return 1;
    }

    error = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Comm_rank failed: %s\n", err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
	return 1;
    }

    flag = 0;
    error = MPI_Finalized(&flag);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalized failed: %s\n", err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
	return error;
    }
    if (flag)
    {
	printf("MPI_Finalized returned true before MPI_Finalize.\n");
	return 1;
    }

    error = MPI_Finalize();
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalize failed: %s\n", err_string);
	fflush(stdout);
	return 1;
    }

    flag = 0;
    error = MPI_Finalized(&flag);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalized failed: %s\n", err_string);
	fflush(stdout);
	return 1;
    }
    if (!flag)
    {
	printf("MPI_Finalized returned false after MPI_Finalize.\n");
	return 1;
    }
    if (rank == 0)
    {
	printf(" No errors\n");
    }
    return 0;  
}
