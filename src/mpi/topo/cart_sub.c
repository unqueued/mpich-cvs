/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_sub */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_sub = PMPI_Cart_sub
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_sub  MPI_Cart_sub
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_sub as PMPI_Cart_sub
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_sub
#define MPI_Cart_sub PMPI_Cart_sub

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_sub

/*@

MPI_Cart_sub - Partitions a communicator into subgroups which 
               form lower-dimensional cartesian subgrids

Input Parameters:
+ comm - communicator with cartesian structure (handle) 
- remain_dims - the  'i'th entry of remain_dims specifies whether the 'i'th 
dimension is kept in the subgrid (true) or is dropped (false) (logical 
vector) 

Output Parameter:
. newcomm - communicator containing the subgrid that includes the calling 
process (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Cart_sub(MPI_Comm comm, int *remain_dims, MPI_Comm *comm_new)
{
    static const char FCNAME[] = "MPI_Cart_sub";
    int mpi_errno = MPI_SUCCESS;
    int ndims, key, color, ndims_in_subcomm, nnodes_in_subcomm, i, j, rank;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPIR_Topology *topo_ptr, *toponew_ptr;
    MPIU_THREADPRIV_DECL;
    MPIU_CHKPMEM_DECL(4);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_SUB);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("topo");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_SUB);

    MPIU_THREADPRIV_GET;

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Check that the communicator already has a Cartesian topology */
    topo_ptr = MPIR_Topology_get( comm_ptr );

    MPIU_ERR_CHKANDJUMP(!topo_ptr,mpi_errno,MPI_ERR_TOPOLOGY,"**notopology");
    MPIU_ERR_CHKANDJUMP(topo_ptr->kind != MPI_CART,mpi_errno,MPI_ERR_TOPOLOGY,
			"**notcarttopo");

    /* Determine the number of remaining dimensions */
    ndims = topo_ptr->topo.cart.ndims;
    ndims_in_subcomm = 0;
    nnodes_in_subcomm = 1;
    for (i=0; i<ndims; i++) {
	if (remain_dims[i]) {
	    ndims_in_subcomm ++;
	    nnodes_in_subcomm *= topo_ptr->topo.cart.dims[i];
	}
    }

    /* Split this communicator.  Do this even if there are no remaining
       dimensions so that the topology information is attached */
    key   = 0;
    color = 0;
    for (i=0; i<ndims; i++) {
	if (remain_dims[i]) {
	    key = (key * topo_ptr->topo.cart.dims[i]) + 
		topo_ptr->topo.cart.position[i];
	}
	else {
	    color = (color * topo_ptr->topo.cart.dims[i]) + 
		topo_ptr->topo.cart.position[i];
	}
    }
    MPIR_Nest_incr();
    mpi_errno = NMPI_Comm_split( comm, color, key, comm_new );
    MPIR_Nest_decr();
    if (mpi_errno) goto fn_fail;
    
    /* Save the topology of this new communicator */
    MPIU_CHKPMEM_MALLOC(toponew_ptr,MPIR_Topology*,sizeof(MPIR_Topology),
			mpi_errno,"toponew_ptr");
	
    toponew_ptr->kind		  = MPI_CART;
    toponew_ptr->topo.cart.ndims  = ndims_in_subcomm;
    toponew_ptr->topo.cart.nnodes = nnodes_in_subcomm;
    if (ndims_in_subcomm) {
	MPIU_CHKPMEM_MALLOC(toponew_ptr->topo.cart.dims,int*,
		    ndims_in_subcomm*sizeof(int),mpi_errno,"cart.dims");
	MPIU_CHKPMEM_MALLOC(toponew_ptr->topo.cart.periodic,int*,
		    ndims_in_subcomm*sizeof(int),mpi_errno,"cart.periodic");
	MPIU_CHKPMEM_MALLOC(toponew_ptr->topo.cart.position,int*,
		    ndims_in_subcomm*sizeof(int),mpi_errno,"cart.position");
    }
    else {
	toponew_ptr->topo.cart.dims     = 0;
	toponew_ptr->topo.cart.periodic = 0;
	toponew_ptr->topo.cart.position = 0;
    }

    j = 0;
    for (i=0; i<ndims; i++) {
	if (remain_dims[i]) {
	    toponew_ptr->topo.cart.dims[j] = topo_ptr->topo.cart.dims[i];
	    toponew_ptr->topo.cart.periodic[j] = topo_ptr->topo.cart.periodic[i];
	    j++;
	}
    }

    MPID_Comm_get_ptr( *comm_new, newcomm_ptr );
    /* Compute the position of this process in the new communicator */
    rank = newcomm_ptr->rank;
    for (i=0; i<ndims_in_subcomm; i++) {
	nnodes_in_subcomm /= toponew_ptr->topo.cart.dims[i];
	toponew_ptr->topo.cart.position[i] = rank / nnodes_in_subcomm;
	rank = rank % nnodes_in_subcomm;
    }

    mpi_errno = MPIR_Topology_put( newcomm_ptr, toponew_ptr );
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_SUB);
    MPIU_THREAD_SINGLE_CS_EXIT("topo");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_cart_sub",
	    "**mpi_cart_sub %C %p %p", comm, remain_dims, comm_new);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
