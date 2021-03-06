/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_map */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_map = PMPI_Graph_map
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_map  MPI_Graph_map
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_map as PMPI_Graph_map
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Graph_map
#define MPI_Graph_map PMPI_Graph_map
int MPIR_Graph_map( const MPID_Comm *comm_ptr, int nnodes, const int index[], 
		    const int edges[], int *newrank )
{
    MPIU_UNREFERENCED_ARG(index);
    MPIU_UNREFERENCED_ARG(edges);

    /* This is the trivial version that does not remap any processes. */
    if (comm_ptr->rank < nnodes) {
	*newrank = comm_ptr->rank;
    }
    else {
	*newrank = MPI_UNDEFINED;
    }
    return MPI_SUCCESS;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_map

/*@
MPI_Graph_map - Maps process to graph topology information

Input Parameters:
+ comm - input communicator (handle) 
. nnodes - number of graph nodes (integer) 
. index - integer array specifying the graph structure, see 'MPI_GRAPH_CREATE' 
- edges - integer array specifying the graph structure 

Output Parameter:
. newrank - reordered rank of the calling process; 'MPI_UNDEFINED' if the 
calling process does not belong to graph (integer) 

.N SignalSafe
 
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Graph_map(MPI_Comm comm_old, int nnodes, int *index, int *edges,
                  int *newrank)
{
    static const char FCNAME[] = "MPI_Graph_map";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_MAP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_MAP);
    
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
	    MPIR_ERRTEST_ARGNULL(newrank,"newrank",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(index,"index",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(edges,"edges",mpi_errno);
	    MPIR_ERRTEST_ARGNONPOS(nnodes,"nnodes",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIU_ERR_CHKANDJUMP(comm_ptr->local_size < nnodes,mpi_errno,MPI_ERR_ARG,
			"**graphnnodes");
    
    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->graphMap != NULL) {
	mpi_errno = comm_ptr->topo_fns->graphMap( comm_ptr, nnodes, 
						  (const int*) index,
						  (const int*) edges, 
						  newrank );
    }
    else {
	mpi_errno = MPIR_Graph_map( comm_ptr, nnodes,
				   (const int*) index,
				   (const int*) edges, newrank );
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_MAP);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_graph_map",
	    "**mpi_graph_map %C %d %p %p %p", comm_old, nnodes, index, edges, 
	    newrank);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
