#include "mpi.h"
#include "mpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* tests noncontiguous reads/writes using collective I/O */

#define SIZE 5000

main(int argc, char **argv)
{
    int *buf, i, mynod, nprocs, flag, len, b[3];
    MPI_Aint d[3];
    MPI_File fh;
    MPI_Status status;
    char *filename;
    MPI_Datatype typevec, newtype, t[3];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &mynod);

    if (nprocs != 2) {
        printf("Run this program on two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

/* process 0 takes the file name as a command-line argument and 
   broadcasts it to other processes */
    if (!mynod) {
	i = 1;
	while ((i < argc) && strcmp("-fname", *argv)) {
	    i++;
	    argv++;
	}
	if (i >= argc) {
	    printf("\n*#  Usage: noncontig_coll -fname filename\n\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
	argv++;
	len = strlen(*argv);
	filename = (char *) malloc(len+1);
	strcpy(filename, *argv);
	MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(filename, len+1, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    else {
	MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
	filename = (char *) malloc(len+1);
	MPI_Bcast(filename, len+1, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    buf = (int *) malloc(SIZE*sizeof(int));

    MPI_Type_vector(SIZE/2, 1, 2, MPI_INT, &typevec);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = mynod*sizeof(int);
    d[2] = SIZE*sizeof(int);
    t[0] = MPI_LB;
    t[1] = typevec;
    t[2] = MPI_UB;

    MPI_Type_struct(3, b, d, t, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&typevec);

    flag = 0;
    if (!mynod) {
	printf("\ntesting noncontiguous in memory, noncontiguous in file using collective I/O\n");
	MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                  MPI_INFO_NULL, &fh);

    MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", MPI_INFO_NULL);

    for (i=0; i<SIZE; i++) buf[i] = i + mynod*SIZE;
    MPI_File_write_all(fh, buf, 1, newtype, &status);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i<SIZE; i++) buf[i] = -1;

    MPI_File_read_at_all(fh, 0, buf, 1, newtype, &status);

    for (i=0; i<SIZE; i++) {
	if (!mynod) {
	    if ((i%2) && (buf[i] != -1)) {
		printf("Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
		flag = 1;
	    }
	    if (!(i%2) && (buf[i] != i)) {
		printf("Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i);
		flag = 1;
	    }
	}
	else {
	    if ((i%2) && (buf[i] != i + mynod*SIZE)) {
		printf("Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i + mynod*SIZE);
		flag = 1;
	    }
	    if (!(i%2) && (buf[i] != -1)) {
		printf("Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
		flag = 1;
	    }
	}
    }

    if (!flag) printf("noncontiguous in memory, noncontiguous in file works fine on process %d\n", mynod);

    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);

    flag = 0;

    if (!mynod) {
	printf("\ntesting noncontiguous in memory, contiguous in file using collective I/O\n");
	MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                  MPI_INFO_NULL, &fh);

    for (i=0; i<SIZE; i++) buf[i] = i + mynod*SIZE;
    MPI_File_write_at_all(fh, mynod*(SIZE/2)*sizeof(int), buf, 1, newtype, &status);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i<SIZE; i++) buf[i] = -1;

    MPI_File_read_at_all(fh, mynod*(SIZE/2)*sizeof(int), buf, 1, newtype, &status);

    for (i=0; i<SIZE; i++) {
	if (!mynod) {
	    if ((i%2) && (buf[i] != -1)) {
		printf("Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
		flag = 1;
	    }
	    if (!(i%2) && (buf[i] != i)) {
		printf("Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i);
		flag = 1;
	    }
	}
	else {
	    if ((i%2) && (buf[i] != i + mynod*SIZE)) {
		printf("Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i + mynod*SIZE);
		flag = 1;
	    }
	    if (!(i%2) && (buf[i] != -1)) {
		printf("Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
		flag = 1;
	    }
	}
    }

    if (!flag) printf("noncontiguous in memory, contiguous in file works fine on process %d\n", mynod);

    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);

    flag = 0;

    if (!mynod) {
	printf("\ntesting contiguous in memory, noncontiguous in file using collective I/O\n");
	MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                  MPI_INFO_NULL, &fh);

    MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", MPI_INFO_NULL);

    for (i=0; i<SIZE; i++) buf[i] = i + mynod*SIZE;
    MPI_File_write_all(fh, buf, SIZE, MPI_INT, &status);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i<SIZE; i++) buf[i] = -1;

    MPI_File_read_at_all(fh, 0, buf, SIZE, MPI_INT, &status);

    for (i=0; i<SIZE; i++) {
	if (!mynod) {
	    if (buf[i] != i) {
		printf("Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i);
		flag = 1;
	    }
	}
	else {
	    if (buf[i] != i + mynod*SIZE) {
		printf("Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i + mynod*SIZE);
		flag = 1;
	    }
	}
    }

    if (!flag) printf("contiguous in memory, noncontiguous in file works fine on process %d\n", mynod);

    MPI_File_close(&fh);


    MPI_Type_free(&newtype);
    free(buf);
    free(filename);
    MPI_Finalize();
}
