/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"
/*#include "bnr.h"*/


/* -- Begin Profiling Symbol Block for routine MPI_Init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init = PMPI_Init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init  MPI_Init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init as PMPI_Init
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Init PMPI_Init

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init

/*@
   MPI_Init - Initialize the MPI execution environment

   Input Parameters:
+  argc - Pointer to the number of arguments 
-  argv - Pointer to the argument vector

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Init( int *argc, char ***argv )
{
    static const char FCNAME[] = "MPI_Init";
    int mpi_errno = MPI_SUCCESS;
    /*
    char *pszParent;
    MPI_Info info;
    MPI_Comm intercomm;
    */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INIT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**inittwice", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /*
    BNR_Init();
    */

    MPIR_Init_thread( MPI_THREAD_SINGLE, (int *)0 );

    /*
    BNR_DB_Get_my_name(MPIR_Process.bnr_dbname);
    BNR_Barrier();

    pszParent = getenv("BNR_PARENT");
    if (pszParent != NULL)
    {
	PMPI_Info_create(&info);
	PMPI_Comm_connect(pszParent, info, 0, MPI_COMM_WORLD, &intercomm);
    }
    else
    {
	intercomm = MPI_COMM_NULL;
    }
    */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT);
    return MPI_SUCCESS;
}
