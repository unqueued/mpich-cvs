/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Abort - abort

   Parameters:
+   MPID_Comm *comm_ptr - communicator
-  int err_code - error code

   Notes:
@*/
int MPID_Abort( MPID_Comm *comm_ptr, int err_code )
{
    MM_ENTER_FUNC(MPID_ABORT);

    err_printf("MPID_Abort: error %d\n", err_code);
    exit(err_code);

    MM_EXIT_FUNC(MPID_ABORT);
    return MPI_SUCCESS;
}
