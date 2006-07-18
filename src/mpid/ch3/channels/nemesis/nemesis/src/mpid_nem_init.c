/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"

#ifdef MEM_REGION_IN_HEAP
MPID_nem_mem_region_t *MPID_nem_mem_region_ptr = 0;
#else /* MEM_REGION_IN_HEAP */
MPID_nem_mem_region_t MPID_nem_mem_region = {0};
#endif /* MEM_REGION_IN_HEAP */

char MPID_nem_hostname[MAX_HOSTNAME_LEN] = "UNKNOWN";

#define MIN( a , b ) ((a) >  (b)) ? (b) : (a)
#define MAX( a , b ) ((a) >= (b)) ? (a) : (b)

static int intcompar (const void *a, const void *b) { return *(int *)a - *(int *)b; }

char *MPID_nem_asymm_base_addr = 0;

static int get_local_procs (int rank, int num_procs, int *num_local, int **local_procs, int *local_rank);

int
MPID_nem_init (int rank, MPIDI_PG_t *pg_p)
{
    return  _MPID_nem_init (rank, pg_p, 0);
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
_MPID_nem_init (int pg_rank, MPIDI_PG_t *pg_p, int ckpt_restart)
{
    int    mpi_errno       = MPI_SUCCESS;
    int    pmi_errno;
    int    num_procs       = pg_p->size;
    pid_t  my_pid;
    int    ret;
    int    num_local;
    int   *local_procs;
    int    local_rank;
    int    global_size;
    int    index, index2, size;
    int    i;
    char  *publish_bc_orig = NULL;
    char  *bc_val          = NULL;
    int    val_max_remaining;
    MPIU_CHKPMEM_DECL(5);
    
    /* Initialize the business card */
    mpi_errno = MPIDI_CH3I_BCInit( &bc_val, &val_max_remaining );
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    publish_bc_orig = bc_val;
    
    gethostname (MPID_nem_hostname, MAX_HOSTNAME_LEN);
    MPID_nem_hostname[MAX_HOSTNAME_LEN-1] = '\0';

    mpi_errno = get_local_procs (pg_rank, num_procs, &num_local, &local_procs, &local_rank);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    

#ifdef MEM_REGION_IN_HEAP
    MPIU_CHKPMEM_MALLOC (MPID_nem_mem_region_ptr, MPID_nem_mem_region_t *, sizeof(MPID_nem_mem_region_t), mpi_errno, "mem_region");
#endif /* MEM_REGION_IN_HEAP */
    
    MPID_nem_mem_region.num_seg        = 6;
    MPIU_CHKPMEM_MALLOC (MPID_nem_mem_region.seg, MPID_nem_seg_info_ptr_t, MPID_nem_mem_region.num_seg * sizeof(MPID_nem_seg_info_t), mpi_errno, "mem_region segments");
    MPIU_CHKPMEM_MALLOC (MPID_nem_mem_region.pid, pid_t *, num_local * sizeof(pid_t), mpi_errno, "mem_region pid list");
    MPID_nem_mem_region.rank           = pg_rank;
    MPID_nem_mem_region.num_local      = num_local;
    MPID_nem_mem_region.num_procs      = num_procs;
    MPID_nem_mem_region.local_procs    = local_procs;
    MPID_nem_mem_region.local_rank     = local_rank;
    MPIU_CHKPMEM_MALLOC (MPID_nem_mem_region.local_ranks, int *, num_procs * sizeof(int), mpi_errno, "mem_region local ranks");
    MPID_nem_mem_region.ext_procs      = num_procs - num_local ; 
    MPIU_CHKPMEM_MALLOC (MPID_nem_mem_region.ext_ranks, int *, MPID_nem_mem_region.ext_procs * sizeof(int), mpi_errno, "mem_region ext ranks");
    MPID_nem_mem_region.next           = NULL;
    
    for (index = 0 ; index < num_procs; index++)
    {
	MPID_nem_mem_region.local_ranks[index] = MPID_NEM_NON_LOCAL;
    }
    for (index = 0; index < num_local; index++)
    {
	index2 = local_procs[index];
	MPID_nem_mem_region.local_ranks[index2] = index;
    }

    index2 = 0;
    for(index = 0 ; index < num_procs ; index++)
    {
	if( ! MPID_NEM_IS_LOCAL (index))
	{
	    MPID_nem_mem_region.ext_ranks[index2++] = index;
	}
    }

    /* Global size for the segment */
    /* Data cells + Header Qs + control blocks + Net data cells + POBoxes */
    global_size = ((num_local * (((MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t))  +
				 (2 * sizeof(MPID_nem_queue_t))            +
				 (sizeof(int))                       +
				 ((MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t))))+   
		   (MAX((num_local * ((num_local-1) * sizeof(MPID_nem_fastbox_t))) , MPID_NEM_ASYMM_NULL_VAL)) +
		   sizeof (MPID_nem_barrier_t));

#ifdef FORCE_ASYM
    {
        /* this is used for debugging
           each process allocates a different sized piece of shared
           memory so that when the shared memory segment used for
           communication is allocated it will probably be mapped at a
           different location for each process
        */
        char *handle;
	int size = (local_rank * 65536) + 65536;
	char *base_addr;

        mpi_errno = MPID_nem_allocate_shared_memory (&base_addr, size, &handle);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            MPID_nem_remove_shared_memory (handle);
            MPIU_Free (handle);
            MPIU_ERR_POP (mpi_errno);
        }
        /* --END ERROR HANDLING-- */
        
        mpi_errno = MPID_nem_remove_shared_memory (handle);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            MPIU_Free (handle);
            MPIU_ERR_POP (mpi_errno);
        }
        /* --END ERROR HANDLING-- */

        MPIU_Free (handle);
    }
    /*fprintf(stderr,"[%i] -- address shift ok \n",pg_rank); */
#endif  /*FORCE_ASYM */

    /*     if (num_local > 1) */
    /* 	MPID_nem_mem_region.map_lock = make_sem (local_rank, num_local, 0); */
    
    mpi_errno = MPID_nem_seg_create (&(MPID_nem_mem_region.memory), global_size, num_local, local_rank, pg_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_check_alloc (num_local);    
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Fastpath boxes */
    size =  MAX((num_local*((num_local-1)*sizeof(MPID_nem_fastbox_t))), MPID_NEM_ASYMM_NULL_VAL);
    mpi_errno = MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[0]), size);      
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Data cells */
    size =  num_local * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t);
    mpi_errno = MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[1]), size);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Network data cells */
    size =  num_local  * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t);
    mpi_errno = MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[2]), size);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Header Qs */
    size = num_local * (2 * sizeof(MPID_nem_queue_t));
    mpi_errno = MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[3]), size);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Control blocks */
    size = num_local * (sizeof(int));
    mpi_errno = MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[4]), size);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Barrier data */
    size = sizeof(MPID_nem_barrier_t);
    mpi_errno = MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[5]), size);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* set up barrier region */
    mpi_errno = MPID_nem_barrier_init ((MPID_nem_barrier_t *)(MPID_nem_mem_region.seg[5].addr));	
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

    my_pid = getpid();
    MPID_NEM_MEMCPY (&(((pid_t *)(MPID_nem_mem_region.seg[0].addr))[local_rank]), &my_pid, sizeof(pid_t));
   
    /* syncro part */  
    mpi_errno = MPID_nem_barrier (num_local, local_rank);   
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    for (index = 0 ; index < num_local ; index ++)
    {
	MPID_nem_mem_region.pid[index] = (((pid_t *)MPID_nem_mem_region.seg[0].addr)[index]);
    }
    mpi_errno = MPID_nem_barrier (num_local, local_rank);   
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* SHMEM QS */
    MPID_nem_mem_region.Elements =
	(MPID_nem_cell_ptr_t) (MPID_nem_mem_region.seg[1].addr + (local_rank * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t)));    
    MPID_nem_mem_region.FreeQ = (MPID_nem_queue_ptr_t *)MPIU_Malloc (num_procs * sizeof(MPID_nem_queue_ptr_t));
    MPID_nem_mem_region.RecvQ = (MPID_nem_queue_ptr_t *)MPIU_Malloc (num_procs * sizeof(MPID_nem_queue_ptr_t));
    MPID_nem_mem_region.net_elements =
	(MPID_nem_cell_ptr_t) (MPID_nem_mem_region.seg[2].addr + (local_rank * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t)));

    MPID_nem_mem_region.FreeQ[pg_rank] =
	(MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + local_rank * sizeof(MPID_nem_queue_t)));
    
    MPID_nem_mem_region.RecvQ[pg_rank] = 
	(MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + (num_local + local_rank) * sizeof(MPID_nem_queue_t)));

    /* Free Q init and building*/
    MPID_nem_queue_init (MPID_nem_mem_region.FreeQ[pg_rank] );
    for (index = 0; index < MPID_NEM_NUM_CELLS; ++index)
    {         
	MPID_nem_cell_init (&(MPID_nem_mem_region.Elements[index]));       
	MPID_nem_queue_enqueue (MPID_nem_mem_region.FreeQ[pg_rank], &(MPID_nem_mem_region.Elements[index]));
    }

    /* Recv Q init only*/
    MPID_nem_queue_init (MPID_nem_mem_region.RecvQ[pg_rank]);

    /* Initialize generic net module pointers */
    mpi_errno = MPID_nem_net_init();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* network init */
    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
    {
	mpi_errno = MPID_nem_net_module_init (MPID_nem_mem_region.RecvQ[pg_rank],
                                              MPID_nem_mem_region.FreeQ[pg_rank],
                                              MPID_nem_mem_region.Elements, 
                                              MPID_NEM_NUM_CELLS,
                                              MPID_nem_mem_region.net_elements, 
                                              MPID_NEM_NUM_CELLS, 
                                              &MPID_nem_mem_region.net_recv_queue, 
                                              &MPID_nem_mem_region.net_free_queue,
                                              ckpt_restart, pg_p, pg_rank,
                                              &bc_val, &val_max_remaining);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }
    else
    {
	if (pg_rank == 0)
	{
	    MPID_nem_mem_region.net_recv_queue = NULL;
	    MPID_nem_mem_region.net_free_queue = NULL;
	}
    }

    /* set default route for only external processes through network */
    for (index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
    {
	index2 = MPID_nem_mem_region.ext_ranks[index];
	MPID_nem_mem_region.FreeQ[index2] = MPID_nem_mem_region.net_free_queue;
	MPID_nem_mem_region.RecvQ[index2] = MPID_nem_mem_region.net_recv_queue;
    }

    
    /* set route for local procs through shmem */   
    for (index = 0; index < num_local; index++)
    {
	index2 = local_procs[index];
	MPID_nem_mem_region.FreeQ[index2] = 
	    (MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + index * sizeof(MPID_nem_queue_t)));
	MPID_nem_mem_region.RecvQ[index2] =
	    (MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + (num_local + index) * sizeof(MPID_nem_queue_t)));
	MPIU_Assert (MPID_NEM_ALIGNED (MPID_nem_mem_region.FreeQ[index2], MPID_NEM_CACHE_LINE_LEN));
	MPIU_Assert (MPID_NEM_ALIGNED (MPID_nem_mem_region.RecvQ[index2], MPID_NEM_CACHE_LINE_LEN));

    }

    /* make pointers to our queues global so we don't have to dereference the array */
    MPID_nem_mem_region.my_freeQ = MPID_nem_mem_region.FreeQ[MPID_nem_mem_region.rank];
    MPID_nem_mem_region.my_recvQ = MPID_nem_mem_region.RecvQ[MPID_nem_mem_region.rank];
    
    
    mpi_errno = MPID_nem_barrier (num_local, local_rank);   
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* POboxes stuff */
    MPID_nem_mem_region.mailboxes.in  = (MPID_nem_fastbox_t **)MPIU_Malloc((num_local)*sizeof(MPID_nem_fastbox_t *));
    MPID_nem_mem_region.mailboxes.out = (MPID_nem_fastbox_t **)MPIU_Malloc((num_local)*sizeof(MPID_nem_fastbox_t *));
    
    MPIU_Assert (num_local > 0);

#define MAILBOX_INDEX(sender, receiver) ( ((sender) > (receiver)) ? ((num_local-1) * (sender) + (receiver)) :		\
                                          (((sender) < (receiver)) ? ((num_local-1) * (sender) + ((receiver)-1)) : 0) )

    for (i = 0; i < num_local; ++i)
    {
	if (i == local_rank)
	{
	    MPID_nem_mem_region.mailboxes.in [i] = NULL ;
	    MPID_nem_mem_region.mailboxes.out[i] = NULL ;
	}
	else
	{
	    MPID_nem_mem_region.mailboxes.in [i] = ((MPID_nem_fastbox_t *)MPID_nem_mem_region.seg[0].addr) + MAILBOX_INDEX (i, local_rank);
	    MPID_nem_mem_region.mailboxes.out[i] = ((MPID_nem_fastbox_t *)MPID_nem_mem_region.seg[0].addr) + MAILBOX_INDEX (local_rank, i);
	    MPID_nem_mem_region.mailboxes.in [i]->common.flag.value  = 0;
	    MPID_nem_mem_region.mailboxes.out[i]->common.flag.value  = 0;	   
	}
    }
#undef MAILBOX_INDEX    

    mpi_errno = MPIDI_PG_SetConnInfo (pg_rank, (const char *)publish_bc_orig);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_barrier (num_local, local_rank);   
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    mpi_errno = MPID_nem_mpich2_init (ckpt_restart);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    mpi_errno = MPID_nem_barrier (num_local, local_rank);   
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

#ifdef ENABLED_CHECKPOINTING
    MPID_nem_ckpt_init (ckpt_restart);
#endif
    

#ifdef PAPI_MONITOR
    my_papi_start( pg_rank );
#endif /*PAPI_MONITOR   */ 

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



/* get_local_procs() determines which processes are local and should use shared memory

   OUT
     num_local -- number of local processes
     local_procs -- array of global ranks of local processes
     local_rank -- our local rank

   This uses PMI to get all of the processes that have the same
   hostname, and puts them into local_procs sorted by global rank.
*/
#undef FUNCNAME
#define FUNCNAME get_local_procs
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
get_local_procs (int global_rank, int num_global, int *num_local, int **local_procs, int *local_rank)
{
#if defined (ENABLED_NO_LOCAL)
#warning shared-memory communication disabled
    /* used for debugging only */
    /* return an array as if there are no other processes on this processor */
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);

    *num_local = 1;
    
    MPIU_CHKPMEM_MALLOC (*local_procs, int *, *num_local * sizeof (int), mpi_errno, "local proc array");
    **local_procs = global_rank;

    *local_rank = 0;
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */

#elif 0 /* PMI_Get_clique_(size)|(ranks) don't work with mpd */
#warning PMI_Get_clique doesnt work with mpd
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int *lrank_p;
    MPIU_CHKPMEM_DECL(1);

    /* get an array of all processes on this node */
    pmi_errno = PMI_Get_clique_size (num_local);
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_get_clique_size", "**pmi_get_clique_size %d", pmi_errno);
    
    MPIU_CHKPMEM_MALLOC (*local_procs, int *, *num_local * sizeof (int), mpi_errno, "local proc array");

    pmi_errno = PMI_Get_clique_ranks (*local_procs, *num_local);
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_get_clique_ranks", "**pmi_get_clique_ranks %d", pmi_errno);

    /* make sure it's sorted  so that ranks are consistent between processes */
    qsort (*local_procs, *num_local, sizeof (**local_procs), intcompar);

    /* find our local rank */
    lrank_p = bsearch (&global_rank, *local_procs, *num_local, sizeof (**local_procs), intcompar);
    MPIU_ERR_CHKANDJUMP (lrank_p == NULL, mpi_errno, MPI_ERR_OTHER, "**not_in_local_ranks");
    *local_rank = lrank_p - *local_procs;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    
#else

    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int *procs;
    int i;
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    MPIU_CHKPMEM_DECL(1);

    mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Put my hostname id */
    memset (key, 0, MPID_NEM_MAX_KEY_VAL_LEN);
    MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "hostname[%d]", global_rank);

    pmi_errno = PMI_KVS_Put (kvs_name, key, MPID_nem_hostname);
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

    pmi_errno = PMI_KVS_Commit (kvs_name);
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

    /* Gather hostnames */
    MPIU_CHKPMEM_MALLOC (procs, int *, num_global * sizeof (int), mpi_errno, "local process index array");

    *num_local = 0;

    for (i = 0; i < num_global; ++i)
    {
	memset (val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	memset (key, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "hostname[%d]", i);

	pmi_errno = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
	
	if (!strncmp (MPID_nem_hostname, val, MPID_NEM_MAX_KEY_VAL_LEN)
#if defined (ENABLED_ODD_EVEN_CLIQUES)
            /* Used for debugging on a single machine: Odd procs on a
               node are seen as local to each other, and even procs on
               a node are seen as local to each other. */
            && ((global_rank & 0x1) == (i & 0x1))
#endif
            )
	{
	    if (i == global_rank)
		*local_rank = *num_local;
	    procs[*num_local] = i;
	    ++*num_local;
	}	
    }

    MPIU_Assert (*num_local > 0); /* there's always at least one process */
    
    /* reduce size of local process array */
    *local_procs = MPIU_Realloc (procs, *num_local * sizeof (int));
    /* --BEGIN ERROR HANDLING-- */
    if (*local_procs == NULL)
    {
        MPIU_CHKMEM_SETERR (mpi_errno, *num_local * sizeof (int), "local process index array");
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif
}

/* MPID_nem_vc_init initialize nemesis' part of the vc */
#undef FUNCNAME
#define FUNCNAME MPID_nem_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL (MPID_STATE_MPID_NEM_VC_INIT);

    MPIDI_FUNC_ENTER (MPID_STATE_MPID_NEM_VC_INIT);
    vc->ch.send_seqno = 0;
    
    /* We do different things for vcs in the COMM_WORLD pg vs other pgs
       COMM_WORLD vcs may use shared memory, and already have queues allocated
    */
    if (vc->lpid < MPID_nem_mem_region.num_procs) 
    {
	/* This vc is in COMM_WORLD */
	vc->ch.is_local = MPID_NEM_IS_LOCAL (vc->lpid);
	vc->ch.free_queue = MPID_nem_mem_region.FreeQ[vc->lpid]; /* networks and local procs have free queues */    
    }
    else
    {
	/* this vc is the result of a connect */
	vc->ch.is_local = 0;
	vc->ch.free_queue = MPID_nem_mem_region.net_free_queue;
    }
    
    if (vc->ch.is_local)
    {
	vc->ch.fbox_out = &MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[vc->lpid]]->mpich2;
	vc->ch.fbox_in = &MPID_nem_mem_region.mailboxes.in[MPID_nem_mem_region.local_ranks[vc->lpid]]->mpich2;
	vc->ch.recv_queue = MPID_nem_mem_region.RecvQ[vc->lpid];
    }
    else
    {
	vc->ch.fbox_out = NULL;
	vc->ch.fbox_in = NULL;
	vc->ch.recv_queue = NULL;

	mpi_errno = MPID_nem_net_module_vc_init (vc, business_card);
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    
    /* FIXME: ch3 assumes there is a field called sendq_head in the ch
       portion of the vc.  This is unused in nemesis and should be set
       to NULL */
    vc->ch.sendq_head = NULL;
    
 fn_exit:
    MPIDI_FUNC_EXIT (MPID_STATE_MPID_NEM_VC_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


int
MPID_nem_get_business_card (char *value, int length)
{
    return MPID_nem_net_module_get_business_card (&value, &length);    
}

int MPID_nem_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    return MPID_nem_net_module_connect_to_root (business_card, new_vc);
}
