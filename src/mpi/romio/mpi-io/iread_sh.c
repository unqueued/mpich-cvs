/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_iread_shared = PMPI_File_iread_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread_shared MPI_File_iread_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread_shared as PMPI_File_iread_shared
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_iread_shared - Nonblocking read using shared file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"

int MPI_File_iread_shared(MPI_File mpi_fh, void *buf, int count, 
			  MPI_Datatype datatype, MPIO_Request *request)
{
	int error_code;
	ADIO_File fh;
	MPI_Status *status;

	fh = MPIO_File_resolve(mpi_fh);

	status = (MPI_Status *) ADIOI_Malloc(sizeof(MPI_Status));

	/* for now, no threads or anything fancy. 
	 * just call the blocking version */
	error_code = MPI_File_read_shared(mpi_fh, buf, count, datatype,
					  status); 
	/* ROMIO-1 doesn't do anything with status.MPI_ERROR */
	status->MPI_ERROR = error_code;

	/* kick off the request */
	MPI_Grequest_start(MPIU_Greq_query_fn, MPIU_Greq_free_fn, 
			MPIU_Greq_cancel_fn, status, request);
	/* but we did all the work already */
	MPI_Grequest_complete(*request);

	/* passed the buck to the blocking version...*/
	return MPI_SUCCESS;
}
#else
int MPI_File_iread_shared(MPI_File mpi_fh, void *buf, int count, 
                          MPI_Datatype datatype, MPIO_Request *request)
{
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    ADIO_File fh;
    static char myname[] = "MPI_FILE_IREAD_SHARED";
    int datatype_size, incr;
    ADIO_Status status;
    ADIO_Offset off, shared_fp;

    fh = MPIO_File_resolve(mpi_fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(fh, myname, error_code);
    MPIO_CHECK_COUNT(fh, count, myname, error_code);
    MPIO_CHECK_DATATYPE(fh, count, myname, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(fh, count, datatype_size, myname, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(fh, myname, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(fh, myname, &error_code);

    incr = (count*datatype_size)/fh->etype_size;
    ADIO_Get_shared_fp(fh, incr, &shared_fp, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
    {
	/* note: ADIO_Get_shared_fp should have set up error code already? */
	MPIO_Err_return_file(fh, error_code);
    }
    /* --END ERROR HANDLING-- */

    if (buftype_is_contig && filetype_is_contig)
    {
    /* convert count and shared_fp to bytes */
	bufsize = datatype_size * count;
	off = fh->disp + fh->etype_size * shared_fp;
        if (!(fh->atomicity))
	{
	    ADIO_IreadContig(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
			off, request, &error_code);
	}
        else
	{
            /* to maintain strict atomicity semantics with other concurrent
              operations, lock (exclusive) and call blocking routine */

            *request = ADIOI_Malloc_request();
            (*request)->optype = ADIOI_READ;
            (*request)->fd = fh;
            (*request)->datatype = datatype;
            (*request)->queued = 0;
	    (*request)->handle = 0;

            if (fh->file_system != ADIO_NFS)
	    {
                ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);
	    }

            ADIO_ReadContig(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
			    off, &status, &error_code);  

            if (fh->file_system != ADIO_NFS)
	    {
                ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);
	    }

            fh->async_count++;
            /* status info. must be linked to the request structure, so that
               it can be accessed later from a wait */
        }
    }
    else
    {
	ADIO_IreadStrided(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
			   shared_fp, request, &error_code);
    }

    return error_code;
}
#endif
