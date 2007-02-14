/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Support for dynamically loaded libraries */

#include "mpiimpl.h"

#ifdef USE_DYNAMIC_LIBRARIES

#if defined(HAVE_FUNC_DLOPEN) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#undef FCNAME
#define FCNAME "MPIU_DLL_Open"
int MPIU_DLL_Open( const char libname[], void **handle )
{
    int mpi_errno = MPI_SUCCESS;

    *handle = dlopen( libname, RTLD_LAZY | RTLD_GLOBAL );
    if (!*handle) {
	MPIU_ERR_SET2(mpi_errno,MPI_ERR_OTHER,"**unableToLoadDLL",
		      "**unableToLoadDLL %s %s", libname, dlerror() );
    }
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIU_DLL_FindSym"
int MPIU_DLL_FindSym( void *handle, const char symbol[], void **value )
{
    int mpi_errno = MPI_SUCCESS;
    
    *value = dlsym( handle, symbol );
    if (!*value) {
	MPIU_ERR_SET2(mpi_errno,MPI_ERR_OTHER,"**unableToLoadDLLsym",
		      "**unableToLoadDLLsym %s %s", symbol, dlerror() );
    }
    return mpi_errno;
}
#endif /* HAVE_FUNC_DLOPEN */

#else
#undef FCNAME
#define FCNAME "MPIU_DLL_Open"
int MPIU_DLL_Open( const char libname[], void **handle )
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**notimpl" );
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIU_DLL_FindSym"
int MPIU_DLL_FindSym( void *handle, const char symbol[], void **value )
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**notimpl" );
    return mpi_errno;
}
#endif /* USE_DYNAMIC_LIBRARIES */