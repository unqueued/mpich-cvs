/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static void generate_shm_string(char *str)
{
#ifdef USE_WINDOWS_SHM
    UUID guid;
    UuidCreate(&guid);
    MPIU_Snprintf(str, MPIDI_MAX_SHM_NAME_LENGTH, 
	"%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    MPIU_DBG_PRINTF(("GUID = %s\n", str));
#elif defined (USE_POSIX_SHM)
    MPIU_Snprintf(str, MPIDI_MAX_SHM_NAME_LENGTH, "/mpich_shm_%d", getpid());
#elif defined (USE_SYSV_SHM)
    MPIU_Snprintf(str, MPIDI_MAX_SHM_NAME_LENGTH, "%d", getpid());
#else
#error No shared memory subsystem defined
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg, int pg_rank,
                    char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;
    MPIDI_VC_t * vc;
    int pg_size;
    int p;
    char * pg_id;
    int pg_id_sz;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int kvs_name_sz;
    char shmemkey[MPIDI_MAX_SHM_NAME_LENGTH];
    int i, j, k;
    int shm_block;
    char local_host[100];
#ifdef HAVE_WINDOWS_H
    DWORD host_len;
#endif

    MPIU_UNREFERENCED_ARG(publish_bc_p);
    MPIU_UNREFERENCED_ARG(bc_key_p);
    MPIU_UNREFERENCED_ARG(bc_val_p);
    MPIU_UNREFERENCED_ARG(val_max_sz_p);

    /*
     * Extract process group related information from PMI and initialize
     * structures that track the process group connections, MPI_COMM_WORLD, and
     * MPI_COMM_SELF
     */
    /* Init is done before this routine is called */
    /* MPID_Init in mpid_init.c handles the process group initialization. */
#if 0
    /* ch3/src/mpid_init.c guarantees that PMI_Init has been called and
       the rank, size, and appnum have been set */
    mpi_errno = PMI_Init(has_parent);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_rank(&pg_rank);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank", "**pmi_get_rank %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }
    
    MPIU_dbg_init(pg_rank);

    /*
     * Get the process group id
     */
    pmi_errno = PMI_Get_id_length_max(&pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pg_id = MPIU_Malloc(pg_id_sz + 1);
    if (pg_id == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_Get_id(pg_id, pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id",
					 "**pmi_get_id %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }


    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(MPIDI_CH3I_PG_Compare_ids, MPIDI_CH3I_PG_Destroy);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_init", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    /*
     * Create a new structure to track the process group
     */
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_create", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    pg->ch.kvs_name = NULL;
    
    /*
     * Get the name of the key-value space (KVS)
     */
    pmi_errno = PMI_KVS_Get_name_length_max(&kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_name_length_max", "**pmi_kvs_get_name_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pg->ch.kvs_name = MPIU_Malloc(kvs_name_sz + 1);
    if (pg->ch.kvs_name == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_KVS_Get_my_name(pg->ch.kvs_name, kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
#endif


    /* set the global variable defaults */
    pg->ch.nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg->ch.nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#ifdef HAVE_WINDOWS_H
    pg->ch.pSharedProcessHandles = NULL;
#else
    pg->ch.pSharedProcessIDs = NULL;
    pg->ch.pSharedProcessFileDescriptors = NULL;
#endif
#endif
    pg->ch.addr = NULL;
#ifdef USE_POSIX_SHM
    pg->ch.key[0] = '\0';
    pg->ch.id = -1;
#elif defined (USE_SYSV_SHM)
    pg->ch.key = -1;
    pg->ch.id = -1;
#elif defined (USE_WINDOWS_SHM)
    pg->ch.key[0] = '\0';
    pg->ch.id = NULL;
#else
#error No shared memory subsystem defined
#endif
    pg->ch.nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pg->ch.nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Initialize the VC table associated with this process
       group (and thus COMM_WORLD) */
    pg_size = pg->size;

    for (p = 0; p < pg_size; p++)
    {
	pg->vct[p].ch.sendq_head = NULL;
	pg->vct[p].ch.sendq_tail = NULL;
	pg->vct[p].ch.req = (MPID_Request*)MPIU_Malloc(sizeof(MPID_Request));
	/* FIXME: the vc's must be set to active for the close protocol to work in the shm channel */
	pg->vct[p].state = MPIDI_VC_STATE_ACTIVE;
	pg->vct[p].ch.shm_reading_pkt = TRUE;
	pg->vct[p].ch.shm_state = 0;
	pg->vct[p].ch.recv_active = NULL;
	pg->vct[p].ch.send_active = NULL;
#ifdef USE_SHM_UNEX
	pg->vct[p].ch.unex_finished_next = NULL;
	pg->vct[p].ch.unex_list = NULL;
#endif
	pg->vct[p].ch.shm = NULL;
	pg->vct[p].ch.read_shmq = NULL;
	pg->vct[p].ch.write_shmq = NULL;
    }
    
    /* save my vc_ptr for easy access */
    MPIDI_PG_Get_vcr(pg, pg_rank, &MPIDI_CH3I_Process.vc);

    /* Initialize Progress Engine */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }

    /* Allocate space for pmi keys and values */
    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    key_max_sz++;
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    val_max_sz++;
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* initialize the shared memory */
    shm_block = sizeof(MPIDI_CH3I_SHM_Queue_t) * pg_size; 

    if (pg_size > 1)
    {
	if (pg_rank == 0)
	{
	    /* Put the shared memory key */
	    generate_shm_string(shmemkey);
	    if (MPIU_Strncpy(key, "SHMEMKEY", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    if (MPIU_Strncpy(val, shmemkey, val_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    mpi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
		return mpi_errno;
	    }

	    /* Put the hostname to make sure everyone is on the same host */
	    if (MPIU_Strncpy(key, "SHMHOST", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
#ifdef HAVE_WINDOWS_H
	    host_len = val_max_sz;
	    /*GetComputerName(val, &host_len);*/
	    GetComputerNameEx(ComputerNameDnsFullyQualified, val, &host_len);
#else
	    gethostname(val, val_max_sz); /* Don't call this under Windows because it requires the socket library */
#endif
	    mpi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
		return mpi_errno;
	    }

	    mpi_errno = PMI_KVS_Commit(pg->ch.kvs_name);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
		return mpi_errno;
	    }
	    mpi_errno = PMI_Barrier();
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
		return mpi_errno;
	    }
	}
	else
	{
	    /* Get the shared memory key */
	    if (MPIU_Strncpy(key, "SHMEMKEY", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    mpi_errno = PMI_Barrier();
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
		return mpi_errno;
	    }
	    mpi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
		return mpi_errno;
	    }
	    if (MPIU_Strncpy(shmemkey, val, val_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    /* Get the root host and make sure local process is on the same node */
	    if (MPIU_Strncpy(key, "SHMHOST", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    mpi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
		return mpi_errno;
	    }
#ifdef HAVE_WINDOWS_H
	    host_len = 100;
	    /*GetComputerName(local_host, &host_len);*/
	    GetComputerNameEx(ComputerNameDnsFullyQualified, local_host, &host_len);
#else
	    gethostname(local_host, 100); /* Don't call this under Windows because it requires the socket library */
#endif
	    if (strcmp(val, local_host))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmhost", "**shmhost %s %s", local_host, val);
		return mpi_errno;
	    }
	}

	MPIU_DBG_PRINTF(("KEY = %s\n", shmemkey));
#if defined(USE_POSIX_SHM) || defined(USE_WINDOWS_SHM)
	if (MPIU_Strncpy(pg->ch.key, shmemkey, MPIDI_MAX_SHM_NAME_LENGTH))
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
	    return mpi_errno;
	}
#elif defined (USE_SYSV_SHM)
	pg->ch.key = atoi(shmemkey);
#else
#error No shared memory subsystem defined
#endif

	mpi_errno = MPIDI_CH3I_SHM_Get_mem( pg, pg_size * shm_block, pg_rank, pg_size, TRUE );
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmgetmem", 0);
	    return mpi_errno;
	}
    }
    else
    {
	mpi_errno = MPIDI_CH3I_SHM_Get_mem( pg, shm_block, 0, 1, FALSE );
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmgetmem", 0);
	    return mpi_errno;
	}
    }

    /* initialize each shared memory queue */
    for (i=0; i<pg_size; i++)
    {
	MPIDI_PG_Get_vcr(pg, i, &vc);
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
	if (pg->ch.pSharedProcessHandles)
	    vc->ch.hSharedProcessHandle = pg->ch.pSharedProcessHandles[i];
#else
	if (pg->ch.pSharedProcessIDs)
	{
	    vc->ch.nSharedProcessID = pg->ch.pSharedProcessIDs[i];
	    vc->ch.nSharedProcessFileDescriptor = pg->ch.pSharedProcessFileDescriptors[i];
	}
#endif
#endif
	if (i == pg_rank)
	{
	    vc->ch.shm = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->ch.addr + (shm_block * i));
	    for (j=0; j<pg_size; j++)
	    {
		vc->ch.shm[j].head_index = 0;
		vc->ch.shm[j].tail_index = 0;
		for (k=0; k<MPIDI_CH3I_NUM_PACKETS; k++)
		{
		    vc->ch.shm[j].packet[k].offset = 0;
		    vc->ch.shm[j].packet[k].avail = MPIDI_CH3I_PKT_AVAILABLE;
		}
	    }
	}
	else
	{
	    /*vc->ch.shm += pg_rank;*/
	    vc->ch.shm = NULL;
	    vc->ch.write_shmq = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->ch.addr + (shm_block * i)) + pg_rank;
	    vc->ch.read_shmq = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->ch.addr + (shm_block * pg_rank)) + i;
	    /* post a read of the first packet header */
	    vc->ch.shm_reading_pkt = TRUE;
	}
    }

#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pg->ch.nShmWaitSpinCount = 1;
        else if (info.dwNumberOfProcessors < (DWORD) pg_size)
            pg->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * info.dwNumberOfProcessors ) / pg_size;
    }
#else
    /* figure out how many processors are available and set the spin count accordingly */
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    {
	int num_cpus;
	num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (num_cpus == 1)
	    pg->ch.nShmWaitSpinCount = 1;
	else if (num_cpus > 0 && num_cpus < pg_size)
            pg->ch.nShmWaitSpinCount = 1;
	    /* pg->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / pg_size; */
    }
#else
    /* if the number of cpus cannot be determined, set the spin count to 1 */
    pg->ch.nShmWaitSpinCount = 1;
#endif
#endif

    mpi_errno = PMI_KVS_Commit(pg->ch.kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }
#ifdef USE_POSIX_SHM
    if (shm_unlink(pg->ch.key))
    {
	/* Everyone is unlinking the same object so failure is ok? */
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_unlink", "**shm_unlink %s %d", pg->ch.key, errno);
	return mpi_errno;
	*/
    }
#elif defined (USE_SYSV_SHM)
    if (shmctl(pg->ch.id, IPC_RMID, NULL))
    {
	/* Everyone is removing the same object so failure is ok? */
	if (errno != EINVAL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmctl", "**shmctl %d %d", pg->ch.id, errno);
	    return mpi_errno;
	}
    }
#endif

fn_exit:
    if (val != NULL)
    { 
	MPIU_Free(val);
    }
    if (key != NULL)
    { 
	MPIU_Free(key);
    }
    
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc ) {
    vc->ch.sendq_head         = NULL;
    vc->ch.sendq_tail         = NULL;

    /* Which of these do we need? */
    vc->ch.recv_active        = NULL;
    vc->ch.send_active        = NULL;
    vc->ch.req                = NULL;
    vc->ch.read_shmq          = NULL;
    vc->ch.write_shmq         = NULL;
    vc->ch.shm                = NULL;
    vc->ch.shm_state          = 0;
    return 0;
}
