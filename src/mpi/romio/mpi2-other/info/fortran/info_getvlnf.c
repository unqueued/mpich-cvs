/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"
#include "adio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_info_get_valuelen_ PMPI_INFO_GET_VALUELEN
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_get_valuelen_ pmpi_info_get_valuelen__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_get_valuelen_ pmpi_info_get_valuelen
#else
#define mpi_info_get_valuelen_ pmpi_info_get_valuelen_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_info_get_valuelen_ MPI_INFO_GET_VALUELEN
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_get_valuelen_ mpi_info_get_valuelen__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_get_valuelen_ mpi_info_get_valuelen
#endif
#endif

void mpi_info_get_valuelen_(MPI_Fint *info, char *key, int *valuelen,
                 int *flag, int *__ierr, int keylen )
{
    MPI_Info info_c;
    char *newkey;
    int new_keylen, lead_blanks, i;

    if (key <= (char *) 0) {
        printf("MPI_Info_get_valuelen: key is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* strip leading and trailing blanks in key */
    lead_blanks = 0;
    for (i=0; i<keylen; i++) 
        if (key[i] == ' ') lead_blanks++;
        else break;

    for (i=keylen-1; i>=0; i--) if (key[i] != ' ') break;
    if (i < 0) {
        printf("MPI_Info_get_valuelen: key is a blank string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    new_keylen = i + 1 - lead_blanks;
    key += lead_blanks;

    newkey = (char *) ADIOI_Malloc((new_keylen+1)*sizeof(char));
    strncpy(newkey, key, new_keylen);
    newkey[new_keylen] = '\0';

    info_c = MPI_Info_f2c(*info);
    *__ierr = MPI_Info_get_valuelen(info_c, newkey, valuelen, flag);
    ADIOI_Free(newkey);
}
