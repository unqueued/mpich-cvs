/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_create = PMPI_Graph_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_create  MPI_Graph_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_create as PMPI_Graph_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Graph_create PMPI_Graph_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_create

/*@
MPI_Graph_create - Makes a new communicator to which topology information
                 has been attached

Input Parameters:
+ comm_old - input communicator without topology (handle) 
. nnodes - number of nodes in graph (integer) 
. index - array of integers describing node degrees (see below) 
. edges - array of integers describing graph edges (see below) 
- reorder - ranking may be reordered (true) or not (false) (logical) 

Output Parameter:
. comm_graph - communicator with graph topology added (handle) 

Notes:
Each process must provide a description of the entire graph, not just the
neigbors of the calling process.  

Algorithm:
We ignore the 'reorder' info currently.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG

@*/
int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index, int *edges, 
		     int reorder, MPI_Comm *comm_graph)
{
    static const char FCNAME[] = "MPI_Graph_create";
    int mpi_errno = MPI_SUCCESS;
    int i, nedges;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPIR_Topology *graph_ptr;
    MPIU_CHKPMEM_DECL(3);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_CREATE);
    
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
	    if (comm_ptr) {
		MPIR_ERRTEST_COMM_INTRA(comm_ptr,mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNEG(nnodes,"nnodes",mpi_errno);
	    if (nnodes > 0) {
		MPIR_ERRTEST_ARGNULL(index,"index",mpi_errno);
		MPIR_ERRTEST_ARGNULL(edges,"edges",mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(comm_graph,"comm_graph",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int j;

	    /* Check that the communicator is large enough */
	    if (nnodes > comm_ptr->remote_size) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
				  "**topotoolarge", "**topotoolarge %d %d",
					  nnodes, comm_ptr->remote_size );
	    }
	    
	    /* Perform the remaining tests only if nnodes is valid.  
	       This avoids SEGVs from accessing invalid parts of the
	       edges or index arrays */
            if (mpi_errno) goto fn_fail;
	    
	    /* Check that index is monotone nondecreasing */
	    /* Use ERR_ARG instead of ERR_TOPOLOGY because there is not
	       topology yet */
	    for (i=0; i<nnodes; i++) {
		if (index[i] < 0) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
			      "**indexneg", "**indexneg %d %d", i, index[i] );
		}
		if (i+1<nnodes && index[i] > index[i+1]) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
			   "**indexnonmonotone", "**indexnonmonotone %d %d %d",
					      i, index[i], index[i+1] );
		}
	    }

	    /* Check that edge number is in range */
	    if (nnodes > 0) { 
		for (i=0; i<index[nnodes-1]; i++) {
		    if (edges[i] > nnodes || edges[i] < 0) {
			mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
				  "**edgeoutrange", "**edgeoutrange %d %d %d", 
						  i, edges[i], nnodes );
		    }
		}
	    }
	    /* We could also check that no edge is from a node to itself.
	       This gives us an excuse to run over the entire arrays. 
	       This test could be combined with the above to make the code
	       shorter.
	    */
	    if (!mpi_errno) {
		j = 0;
		for (i=0; i<nnodes && !mpi_errno; i++) {
		    for (;j<index[i]; j++) {
			if (edges[j] == i) {
			    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
				     "**nulledge", "**nulledge %d %d", i, j );
			    break;
			}
		    }
		}
	    }
	    
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Test for empty communicator */
    if (nnodes == 0) {
	*comm_graph = MPI_COMM_NULL;
	goto fn_exit;
    }

    /* Create a new communicator */
    mpi_errno = MPIR_Comm_copy( comm_ptr, nnodes, &newcomm_ptr );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* If this process is not in the resulting communicator, return a 
       null communicator and exit */
    if (!newcomm_ptr) {
	*comm_graph = MPI_COMM_NULL;
	goto fn_exit;
    }

    nedges = index[nnodes-1];
    MPIU_CHKPMEM_MALLOC(graph_ptr,MPIR_Topology*,sizeof(MPIR_Topology),
			mpi_errno,"graph_ptr");
    
    graph_ptr->kind = MPI_GRAPH;
    graph_ptr->topo.graph.nnodes = nnodes;
    graph_ptr->topo.graph.nedges = nedges;
    MPIU_CHKPMEM_MALLOC(graph_ptr->topo.graph.index,int*,
			nnodes*sizeof(int),mpi_errno,"graph.index");
    MPIU_CHKPMEM_MALLOC(graph_ptr->topo.graph.edges,int*,
			nedges*sizeof(int),mpi_errno,"graph.edges");
    for (i=0; i<nnodes; i++) 
	graph_ptr->topo.graph.index[i] = index[i];
    for (i=0; i<nedges; i++) 
	graph_ptr->topo.graph.edges[i] = edges[i];

    /* Finally, place the topology onto the new communicator and return the
       handle */
    mpi_errno = MPIR_Topology_put( newcomm_ptr, graph_ptr );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    *comm_graph = newcomm_ptr->handle;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_CREATE );
    MPID_CS_EXIT();
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_graph_create",
	    "**mpi_graph_create %C %d %p %p %d %p", comm_old, nnodes, index, edges, reorder, comm_graph);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
