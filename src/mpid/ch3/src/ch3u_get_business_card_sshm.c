#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "mpidi_ch3_impl.h"
#include "pmi.h"


/*  MPIDI_CH3U_Get_business_card_sshm - does sshm specific portion of getting a business card
 *     bc_val_p     - business card value buffer pointer, updated to the next available location or
 *                    freed if published.
 *     val_max_sz_p - ptr to maximum value buffer size reduced by the number of characters written
 *                               
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Get_business_card_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Get_business_card_sshm(char **bc_val_p, int *val_max_sz_p)
{
#ifdef MPIDI_CH3_USES_SSHM
    char queue_name[100];
    char key[] = "bootstrapQ_name";
    int mpi_errno;
    MPIDI_PG_t * pg = MPIDI_Process.my_pg;
    

    /* must ensure shm_hostname is set prior to this upcall */
/*     printf("before first MPIU_Str_add_string_arg\n"); */
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_SHM_HOST_KEY, pg->ch.shm_hostname);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

/*     printf("before MPIDI_CH3I_BootstrapQ_tostring\n"); */
    /* brad: must ensure that KVS was already appropriately updated (i.e. bootstrapQ was set) */
    queue_name[0] = '\0';
    mpi_errno = MPIDI_CH3I_BootstrapQ_tostring(pg->ch.bootstrapQ, queue_name, 100);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != 0)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_SHM_QUEUE_KEY, queue_name);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

#ifdef MPIDI_CH3_IMPLEMENTS_COMM_CONNECT
    /* brad : bootstrapQ_name already set in ch3u_init_sshm.c . added for dynamic process to work on SMP.  */
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_SHM_BOOTSTRAPQ_NAME_KEY,
                                        pg->ch.bootstrapQ_name );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
#endif    

#endif /* uses MPIDI_CH3_USES_SSHM */
    return MPI_SUCCESS;
}
