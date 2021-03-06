/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_create = PMPI_Cart_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_create  MPI_Cart_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_create as PMPI_Cart_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_create
#define MPI_Cart_create PMPI_Cart_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_create

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
int MPIR_Cart_create( const MPID_Comm *comm_ptr, int ndims, const int dims[], 
		      const int periods[], int reorder, MPI_Comm *comm_cart )
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPIR_Cart_create";
#endif
    int       i, newsize, rank, nranks, mpi_errno = MPI_SUCCESS;
    MPID_Comm *newcomm_ptr = NULL;
    MPIR_Topology *cart_ptr = NULL;
    MPIU_CHKPMEM_DECL(4);
    
    /* Set this as null incase we exit with an error */
    *comm_cart = MPI_COMM_NULL;

    /* Check for invalid arguments */
    newsize = 1;
    for (i=0; i<ndims; i++) 
	newsize *= dims[i];

    /* Use ERR_ARG instead of ERR_TOPOLOGY because there is no topology yet */
    MPIU_ERR_CHKANDJUMP2((newsize > comm_ptr->remote_size), mpi_errno, 
			 MPI_ERR_ARG, "**cartdim",
			 "**cartdim %d %d", comm_ptr->remote_size, newsize);


    /* Create a new communicator as a duplicate of the input communicator
       (but do not duplicate the attributes) */
    if (reorder) {
	MPI_Comm ncomm;
	MPIU_THREADPRIV_DECL;

	/* Allow the cart map routine to remap the assignment of ranks to 
	   processes */
	MPIU_THREADPRIV_GET;
	MPIR_Nest_incr();
	mpi_errno = NMPI_Cart_map( comm_ptr->handle, ndims, (int *)dims, 
				   (int *)periods, &rank );
	/* Create the new communicator with split, since we need to reorder
	   the ranks (including the related internals, such as the connection
	   tables */
	if (mpi_errno == 0) {
	    mpi_errno = NMPI_Comm_split( comm_ptr->handle, 
				rank == MPI_UNDEFINED ? MPI_UNDEFINED : 1,
				rank, &ncomm );
	    if (!mpi_errno) {
		MPID_Comm_get_ptr( ncomm, newcomm_ptr );
	    }
	}
	MPIR_Nest_decr();
    }
    else {
	mpi_errno = MPIR_Comm_copy( (MPID_Comm *)comm_ptr, newsize, 
				    &newcomm_ptr );
	rank   = comm_ptr->rank;
    }
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* If this process is not in the resulting communicator, return a 
       null communicator and exit */
    if (rank >= newsize || rank == MPI_UNDEFINED) {
	*comm_cart = MPI_COMM_NULL;
	return MPI_SUCCESS;
    }

    /* Create the topololgy structure */
    MPIU_CHKPMEM_MALLOC(cart_ptr,MPIR_Topology*,sizeof(MPIR_Topology),
			mpi_errno, "cart_ptr" );

    cart_ptr->kind               = MPI_CART;
    cart_ptr->topo.cart.nnodes   = newsize;
    cart_ptr->topo.cart.ndims    = ndims;
    MPIU_CHKPMEM_MALLOC(cart_ptr->topo.cart.dims,int*,ndims*sizeof(int),
			mpi_errno, "cart.dims");
    MPIU_CHKPMEM_MALLOC(cart_ptr->topo.cart.periodic,int*,ndims*sizeof(int),
			mpi_errno, "cart.periodic");
    MPIU_CHKPMEM_MALLOC(cart_ptr->topo.cart.position,int*,ndims*sizeof(int),
			mpi_errno, "cart.position");
    nranks = newsize;
    for (i=0; i<ndims; i++)
    {
	cart_ptr->topo.cart.dims[i]     = dims[i];
	cart_ptr->topo.cart.periodic[i] = periods[i];
	nranks = nranks / dims[i];
	/* FIXME: nranks could be zero (?) */
	cart_ptr->topo.cart.position[i] = rank / nranks;
	rank = rank % nranks;
    }

    /* Place this topology onto the communicator */
    mpi_errno = MPIR_Topology_put( newcomm_ptr, cart_ptr );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    *comm_cart = newcomm_ptr->handle;

    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, "MPIR_Cart_create", __LINE__, 
	    MPI_ERR_OTHER, 
	    "**mpi_cart_create",
	    "**mpi_cart_create %C %d %p %p %d %p", comm_ptr, ndims, dims, 
	    periods, reorder, comm_cart);
    }
#   endif
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}
#endif

/*@

MPI_Cart_create - Makes a new communicator to which topology information
                  has been attached

Input Parameters:
+ comm_old - input communicator (handle) 
. ndims - number of dimensions of cartesian grid (integer) 
. dims - integer array of size ndims specifying the number of processes in 
  each dimension 
. periods - logical array of size ndims specifying whether the grid is 
  periodic (true) or not (false) in each dimension 
- reorder - ranking may be reordered (true) or not (false) (logical) 

Output Parameter:
. comm_cart - communicator with new cartesian topology (handle) 

Algorithm:
We ignore 'reorder' info currently.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		    int reorder, MPI_Comm *comm_cart)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Cart_create";
#endif
    int       mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("topo");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_CREATE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm_old, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm_old, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL( dims, "dims", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( periods, "periods", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( comm_cart, "comm_cart", mpi_errno );
	    if (ndims <= 0) {
		/* Must have a positive number of dimensions */
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
			  MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_DIMS,
						  "**dims",  "**dims %d", 0 );
	    }
	    MPIR_ERRTEST_ARGNEG( ndims, "ndims", mpi_errno );
	    if (comm_ptr) {
		MPIR_ERRTEST_COMM_INTRA( comm_ptr, mpi_errno );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartCreate != NULL) {
	mpi_errno = comm_ptr->topo_fns->cartCreate( comm_ptr, ndims, 
						    (const int*) dims,
						    (const int*) periods, 
						    reorder,
						    comm_cart );
    }
    else {
	mpi_errno = MPIR_Cart_create( comm_ptr, ndims, 
				      (const int*) dims,
				      (const int*) periods, reorder, 
				      comm_cart );
    }
#ifdef HAVE_ERROR_CHECKING
    if (mpi_errno) goto fn_fail;
#endif
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_CREATE);
    MPIU_THREAD_SINGLE_CS_EXIT("topo");
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_cart_create",
	    "**mpi_cart_create %C %d %p %p %d %p", comm_old, ndims, dims, 
	    periods, reorder, comm_cart);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
