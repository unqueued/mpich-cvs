/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_get_type_extent - Returns the extent of datatype in the file

Input Parameters:
. fh - file handle (handle)
. datatype - datatype (handle)

Output Parameters:
. extent - extent of the datatype (nonnegative integer)

.N fortran
@*/
int MPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype, 
                             MPI_Aint *extent)
{
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_type_extent: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_get_type_extent: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return MPI_Type_extent(datatype, extent);
}
