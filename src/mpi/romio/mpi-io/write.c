/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/* status object not filled currently */

/*@
    MPI_File_write - Write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write(MPI_File fh, void *buf, int count, 
                   MPI_Datatype datatype, MPI_Status *status)
{
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    int datatype_size;
    ADIO_Offset off;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEWRITE, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_write: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (buf <= (void *) 0) {
        printf("MPI_File_write: buf is not a valid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_write: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_write: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(datatype, &datatype_size);
    if (count*datatype_size == 0) return MPI_SUCCESS;

    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_write: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    /* contiguous or strided? */

    if (buftype_is_contig && filetype_is_contig) {
    /* convert bufcount to bytes */
	bufsize = datatype_size * count;

        /* if atomic mode requested, lock (exclusive) the region, because there
           could be a concurrent noncontiguous request. Locking doesn't 
           work on PIOFS, and on NFS it is done in the ADIO_WriteContig.*/
        off = fh->fp_ind;
        if ((fh->atomicity) && (fh->file_system != ADIO_PIOFS) && 
              (fh->file_system != ADIO_NFS))
            ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);

	ADIO_WriteContig(fh, buf, bufsize, ADIO_INDIVIDUAL,
		     0, status, &error_code); 

        if ((fh->atomicity) && (fh->file_system != ADIO_PIOFS) && 
              (fh->file_system != ADIO_NFS))
            ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);
    }
    else
	ADIO_WriteStrided(fh, buf, count, datatype, ADIO_INDIVIDUAL,
			 0, status, &error_code); 
    /* For strided and atomic mode, locking is done in ADIO_WriteStrided */

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}
