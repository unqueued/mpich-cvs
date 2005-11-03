/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include "mpidi_ch3_impl.h"
#include "pmi.h"

static int getNumProcessors( void );

/*  MPIDI_CH3U_Init_sshm - does scalable shared memory specific channel 
 *  initialization
 *     publish_bc - if non-NULL, will be a pointer to the original position of 
 *     the bc_val and should do KVS Put/Commit/Barrier on business card before 
 *     returning
 *     bc_key     - business card key buffer pointer.  freed if successfully 
 *                  published
 *     bc_val     - business card value buffer pointer, updated to the next 
 *                  available location or freed if published.
 *     val_max_sz - maximum value buffer size reduced by the number of 
 *                  characters written
 *                               
 */

/* This routine is used only by channels/{sshm,ssm}/src/ch3_init.c */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Init_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Init_sshm(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
                         char **publish_bc_p, char **bc_key_p, 
			 char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int pg_size;
    int p;
#ifdef USE_PERSISTENT_SHARED_MEMORY
    char * parent_bizcard = NULL;
#endif
#ifdef USE_MQSHM
    char queue_name[MPIDI_MAX_SHM_NAME_LENGTH];
    int initialize_queue = 0;
#endif
    int key_max_sz;
    int val_max_sz;
    char * key = NULL;
    char * val = NULL;
    MPIU_CHKLMEM_DECL(2);

    srand(getpid()); /* brad : needed by generate_shm_string */

    MPIDI_CH3I_Process.shm_reading_list = NULL;
    MPIDI_CH3I_Process.shm_writing_list = NULL;
    MPIDI_CH3I_Process.num_cpus = -1;

    /* brad : need to set these locally */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_key_length_max", 
			     "**pmi_kvs_get_key_length_max %d", pmi_errno);
    }

    MPIU_CHKLMEM_MALLOC(key,char *,key_max_sz,mpi_errno,"key");
    
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_value_length_max", 
			     "**pmi_kvs_get_value_length_max %d", pmi_errno);
    }

    MPIU_CHKLMEM_MALLOC(val,char *,val_max_sz,mpi_errno,"val");

#ifdef MPIDI_CH3_USES_SHM_NAME
    pg_p->ch.shm_name = NULL;
    pg_p->ch.shm_name = MPIU_Malloc(sizeof(char) * MPIDI_MAX_SHM_NAME_LENGTH);
    if (pg_p->ch.shm_name == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
    }
#endif

    /* set the global variable defaults */
    pg_p->ch.nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg_p->ch.nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    pg_p->ch.nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pg_p->ch.nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Figure out how many processors are available and set the spin count 
       accordingly */
    /* If there were topology information available we could calculate a 
       multi-cpu number */
    {
	int ncpus = getNumProcessors();
	/* if you know the number of processors, calculate the spin count 
	   relative to that number */
        if (ncpus == 1)
            pg_p->ch.nShmWaitSpinCount = 1;
	/* FIXME: Why is this commented out? */
	/*
        else if (ncpus  < num_procs_per_node)
            pg->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * ncpus) / num_procs_per_node;
	*/
	if (ncpus > 0)
	    MPIDI_CH3I_Process.num_cpus = ncpus;
    }

/* FIXME: This code probably reflects a bug caused by commenting out the above
   code */
#ifndef HAVE_WINDOWS_H    /* brad - nShmWaitSpinCount is uninitialized in sshm but probably shouldn't be */
    pg_p->ch.nShmWaitSpinCount = 1;
    g_nLockSpinCount = 1;
#endif

    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
    }
    
    /* Initialize the VC table associated with this process group (and thus COMM_WORLD) */
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3_VC_Init( &pg_p->vct[p] );
    }

    /* brad : do the shared memory specific setup items so we can later do the
     *         shared memory aspects of the business card
     */
    
#ifdef HAVE_WINDOWS_H
    {
	DWORD len = sizeof(pg_p->ch.shm_hostname);
	/*GetComputerName(pg_p->ch.shm_hostname, &len);*/
	GetComputerNameEx(ComputerNameDnsFullyQualified, pg_p->ch.shm_hostname, &len);
    }
#else
    gethostname(pg_p->ch.shm_hostname, sizeof(pg_p->ch.shm_hostname));
#endif

#ifdef MPIDI_CH3_USES_SHM_NAME
    MPIDI_Process.my_pg = pg_p;  /* was later prior but internally Get_parent_port needs this */    
#ifdef USE_PERSISTENT_SHARED_MEMORY
    if (has_parent) /* set in PMI_Init */
    {
        mpi_errno = MPIDI_CH3_Get_parent_port(&parent_bizcard);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|get_parent_port");
        }

	/* Parse the shared memory queue name from the bizcard */
	{
	    char *orig_str, *tmp_str = MPIU_Malloc(sizeof(char) * MPIDI_MAX_SHM_NAME_LENGTH);
	    if (tmp_str == NULL) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    mpi_errno = MPIU_Str_get_string_arg(parent_bizcard, 
		 MPIDI_CH3I_SHM_QUEUE_KEY, tmp_str, MPIDI_MAX_SHM_NAME_LENGTH);
	    if (mpi_errno != MPIU_STR_SUCCESS) {
		MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
				     "**fail", "**fail %d", mpi_errno);
	    }
	    orig_str = tmp_str;
	    while (*tmp_str != ':' && *tmp_str != '\0')
		tmp_str++;
	    if (*tmp_str == ':')
	    {
		tmp_str++;
		mpi_errno = MPIU_Strncpy(pg_p->ch.shm_name, tmp_str, 
					 MPIDI_MAX_SHM_NAME_LENGTH);
		MPIU_Free(orig_str);
		if (mpi_errno != 0) {
		    MPIU_ERR_POP(mpi_errno);
		}
	    }
	    else
	    {
		MPIU_Free(orig_str);
		MPIU_ERR_POP(mpi_errno);
	    }
	}
    } /* has_parent */
#else
    /* NOTE: Do not use shared memory to communicate to parent */
    pg_p->ch.shm_name[0] = 0;
#endif
#endif            

#ifdef USE_MQSHM

    MPIU_Strncpy(key, MPIDI_CH3I_SHM_QUEUE_NAME_KEY, key_max_sz );
    if (pg_rank == 0)
    {
#ifdef USE_PERSISTENT_SHARED_MEMORY
	/* With persistent shared memory, only the first process of the first group needs to create the bootstrap queue. */
	/* Everyone else including spawned processes will attach to this queue. */
	if (has_parent)
	{
	    /* If you have a parent then you must not initialize the queue since the parent already did. */
	    initialize_queue = 0;
	    MPIU_Strncpy(queue_name, pg_p->ch.shm_name, MPIDI_MAX_SHM_NAME_LENGTH);
	    MPIU_Strncpy(val, queue_name, val_max_sz);
	}
	else
#else
	/* Without persistent shared memory the root process of each process group creates a unique
	 * bootstrap queue to be used only by processes within the same process group */
#endif
	{
	    mpi_errno = MPIDI_CH3I_BootstrapQ_create_unique_name(queue_name, MPIDI_MAX_SHM_NAME_LENGTH);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
	    }
	    /* If you don't have a parent then you must initialize the queue */
	    initialize_queue = 1;
	    MPIU_Strncpy(val, queue_name, val_max_sz);
	    MPIU_Strncpy(pg_p->ch.shm_name, val, val_max_sz);
	}

	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pg_p->ch.bootstrapQ, queue_name, initialize_queue);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
	}
	/*printf("root process created bootQ: '%s'\n", queue_name);fflush(stdout);*/

	mpi_errno = PMI_KVS_Put(pg_p->ch.kvs_name, key, val);          
	if (mpi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_put", 
				 "**pmi_kvs_put %d", mpi_errno);
	}
	mpi_errno = PMI_KVS_Commit(pg_p->ch.kvs_name);
	if (mpi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_commit", 
				 "**pmi_kvs_commit %d", mpi_errno);
	}
	mpi_errno = PMI_Barrier();
	if (mpi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_barrier", 
				 "**pmi_barrier %d", mpi_errno);
	}
    }
    else
    {
	mpi_errno = PMI_Barrier();
	if (mpi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_barrier", 
				 "**pmi_barrier %d", mpi_errno);
	}
	mpi_errno = PMI_KVS_Get(pg_p->ch.kvs_name, key, val, val_max_sz);
	if (mpi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_get", 
				 "**pmi_kvs_get %d", mpi_errno);
	}
	MPIU_Strncpy(queue_name, val, MPIDI_MAX_SHM_NAME_LENGTH);
#ifdef MPIDI_CH3_USES_SHM_NAME
	MPIU_Strncpy(pg_p->ch.shm_name, val, MPIDI_MAX_SHM_NAME_LENGTH);
#endif
	/*printf("process %d got bootQ name: '%s'\n", pg_rank, queue_name);fflush(stdout);*/
	/* The root process initialized the queue */
	initialize_queue = 1;
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pg_p->ch.bootstrapQ, queue_name, initialize_queue);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
	}
    }
    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", 
			     "**pmi_barrier %d", mpi_errno);
    }

#ifdef USE_PERSISTENT_SHARED_MEMORY
    /* The bootstrap queue cannot be unlinked because it can be used outside of this process group. */
    /* Spawned groups will use it and other MPI jobs may use it by calling MPI_Comm_connect/accept */
    /* By not unlinking here, if the program aborts, the 
     * shared memory segments can be left dangling.
     */
#else
    /* Unlinking here prevents leaking shared memory segments but also prevents any other processes
     * from attaching to the segment later thus preventing the implementation of 
     * MPI_Comm_connect/accept/spawn/spawn_multiple
     */
    mpi_errno = MPIDI_CH3I_BootstrapQ_unlink(pg_p->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_unlink");
    }
#endif

#else

    mpi_errno = MPIDI_CH3I_BootstrapQ_create(&pg_p->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
    }

#endif

    /* brad : the pg needs to be set for sshm channels.  for all channels this is done in mpid_init.c */
    MPIDI_Process.my_pg = pg_p;

    /* brad : get the sshm part of the business card  */
    mpi_errno = MPIDI_CH3U_Get_business_card_sshm(bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_buscard");
    }

    /* see if we're meant to publish */
    if (publish_bc_p != NULL) {
	/*
	printf("business card:\n<%s>\npg_id:\n<%s>\n\n", *publish_bc_p, pg_p->id);
	fflush(stdout);
	*/
	pmi_errno = PMI_KVS_Put(pg_p->ch.kvs_name, *bc_key_p, *publish_bc_p);
	if (pmi_errno != PMI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**pmi_kvs_put",
				 "**pmi_kvs_put %d", pmi_errno);
	}
	pmi_errno = PMI_KVS_Commit(pg_p->ch.kvs_name);
	if (pmi_errno != PMI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**pmi_kvs_commit",
				 "**pmi_kvs_commit %d", pmi_errno);
	}

	pmi_errno = PMI_Barrier();
	if (pmi_errno != PMI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**pmi_barrier",
				 "**pmi_barrier %d", pmi_errno);
	}
    }

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
    /* --BEGIN ERROR HANDLING-- */
    if (pg_p != NULL)
    {
	/* MPIDI_CH3I_PG_Destroy(), which is called by MPIDI_PG_Destroy(), frees pg->ch.kvs_name */
	MPIDI_PG_Destroy( pg_p );
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* This routine initializes shm-specific elements of the VC */
int MPIDI_VC_InitShm( MPIDI_VC_t *vc ) 
{
    vc->ch.recv_active        = NULL;
    vc->ch.send_active        = NULL;
    vc->ch.req                = NULL;
    vc->ch.read_shmq          = NULL;
    vc->ch.write_shmq         = NULL;
    vc->ch.shm                = NULL;
    vc->ch.shm_state          = 0;
    vc->ch.shm_next_reader    = NULL;
    vc->ch.shm_next_writer    = NULL;
    vc->ch.shm_read_connected = 0;
    return 0;
}

/* Return the number of processors, or one if the number cannot be 
   determined */
static int getNumProcessors( void )
{
#ifdef HAVE_WINDOWS_H
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
#elif defined(HAVE_SYSCONF)
    int num_cpus;
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    return num_cpus;
#else
    return 1;
#endif
}
