#include "mpi.h"
#include "mpitestconf.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define HAVE_MPI_TYPE_GET_TRUE_EXTENT

static int verbose = 0;

/* tests */
int int_with_lb_ub_test(void);
int contig_of_int_with_lb_ub_test(void);
int contig_negextent_of_int_with_lb_ub_test(void);
int vector_of_int_with_lb_ub_test(void);
int vector_blklen_of_int_with_lb_ub_test(void);
int vector_blklen_stride_of_int_with_lb_ub_test(void);
int vector_blklen_stride_negextent_of_int_with_lb_ub_test(void);
int vector_blklen_negstride_negextent_of_int_with_lb_ub_test(void);
int int_with_negextent_test(void);
int vector_blklen_negstride_of_int_with_lb_ub_test(void);

/* helper functions */
int parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MPI_Init(&argc, &argv); /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* perform some tests */
    err = int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in simple lb/ub test\n", err);
    errs += err;

    err = contig_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in contig test\n", err);
    errs += err;

    err = contig_negextent_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in negextent contig test\n", err);
    errs += err;

    err = vector_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in simple vector test\n", err);
    errs += err;

    err = vector_blklen_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in vector blklen test\n", err);
    errs += err;

    err = vector_blklen_stride_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in strided vector test\n", err);
    errs += err;

    err = vector_blklen_negstride_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in negstrided vector test\n", err);
    errs += err;

    err = int_with_negextent_test();
    if (err && verbose) fprintf(stderr, "found %d errors in negextent lb/ub test\n", err);
    errs += err;

    err = vector_blklen_stride_negextent_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in strided negextent vector test\n", err);
    errs += err;

    err = vector_blklen_negstride_negextent_of_int_with_lb_ub_test();
    if (err && verbose) fprintf(stderr, "found %d errors in negstrided negextent vector test\n", err);
    errs += err;

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

int int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype eviltype;

    err = MPI_Type_struct(3, blocks, disps, types, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_struct failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 4) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 4);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 9) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 9);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 6) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 6);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != 0) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 4) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 4);
    }
#endif
    
    MPI_Type_free(&eviltype);

    return errs;
}

int contig_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_contiguous(3, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_contiguous failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 12) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 12);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 27) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 27);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 24) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 24);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != 0) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 22) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 22);
    }
#endif

    return errs;
}

int contig_negextent_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_contiguous(3, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_contiguous failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 12) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 12);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 9) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 9);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -12) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -12);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, -3);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != -18) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -18);
    }

    if (aval != 22) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 22);
    }
#endif

    return errs;
}

int vector_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_vector(3, 1, 1, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_vector failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 12) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 12);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 27) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 27);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 24) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 24);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != 0) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 22) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 22);
    }
#endif

    return errs;
}

/*
 * blklen = 4
 */
int vector_blklen_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_vector(3, 4, 1, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_vector failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 48) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 54) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 54);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 51) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 51);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != 0) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 49) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 49);
    }
#endif

    return errs;
}

int vector_blklen_stride_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_vector(3, 4, 5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_vector failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 48) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 126) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 126);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -3);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 123) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 123);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != 0) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 121) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }
#endif

    return errs;
}

int vector_blklen_negstride_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { -3, 0, 6 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_vector(3, 4, -5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_vector failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 48) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 126) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 126);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -93) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -93);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 33) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 33);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != -90) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -90);
    }

    if (aval != 121) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }
#endif

    return errs;
}

int int_with_negextent_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype eviltype;

    err = MPI_Type_struct(3, blocks, disps, types, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_struct failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 4) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 4);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != -9) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, -9);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != 6) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, 6);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, -3);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != 0) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, 0);
    }

    if (aval != 4) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 4);
    }
#endif
    
    MPI_Type_free(&eviltype);

    return errs;
}

int vector_blklen_stride_negextent_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint true_lb, aval;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_vector(3, 4, 5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_vector failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 48) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 108) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 108);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -111) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -111);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != -3) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, -3);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != -117) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -117);
    }

    if (aval != 121) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }
#endif

    return errs;
}

int vector_blklen_negstride_negextent_of_int_with_lb_ub_test(void)
{
    int err, errs = 0, val;
    MPI_Aint aval, true_lb;
    int blocks[3] = { 1, 4, 1 };
    MPI_Aint disps[3] = { 6, 0, -3 };
    MPI_Datatype types[3] = { MPI_LB, MPI_BYTE, MPI_UB };

    MPI_Datatype inttype, eviltype;

    /* build same type as in int_with_lb_ub_test() */
    err = MPI_Type_struct(3, blocks, disps, types, &inttype);

    err = MPI_Type_vector(3, 4, -5, inttype, &eviltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_vector failed.\n");
    }

    err = MPI_Type_size(eviltype, &val);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_size failed.\n");
    }

    if (val != 48) {
	errs++;
	if (verbose) fprintf(stderr, "  size of type = %d; should be %d\n", val, 48);
    }

    err = MPI_Type_extent(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_extent failed.\n");
    }

    if (aval != 108) {
	errs++;
	if (verbose) fprintf(stderr, "  extent of type = %d; should be %d\n", (int) aval, 108);
    }
    
    err = MPI_Type_lb(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_lb failed.\n");
    }

    if (aval != -21) {
	errs++;
	if (verbose) fprintf(stderr, "  lb of type = %d; should be %d\n", (int) aval, -21);
    }

    err = MPI_Type_ub(eviltype, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_ub failed.\n");
    }

    if (aval != 87) {
	errs++;
	if (verbose) fprintf(stderr, "  ub of type = %d; should be %d\n", (int) aval, 87);
    }

#ifdef HAVE_MPI_TYPE_GET_TRUE_EXTENT
    err = MPI_Type_get_true_extent(eviltype, &true_lb, &aval);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "  MPI_Type_get_true_extent failed.\n");
    }

    if (true_lb != -27) {
	errs++;
	if (verbose) fprintf(stderr, "  true_lb of type = %d; should be %d\n", (int) true_lb, -27);
    }

    if (aval != 121) {
	errs++;
	if (verbose) fprintf(stderr, "  true extent of type = %d; should be %d\n", (int) aval, 121);
    }
#endif

    return errs;
}
