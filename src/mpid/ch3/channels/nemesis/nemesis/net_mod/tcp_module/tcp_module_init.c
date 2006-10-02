/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_module_impl.h"

static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_tcp_free_queue = 0;

MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

mpid_nem_tcp_internal_t MPID_nem_tcp_internal_vars = {0};

#undef FUNCNAME
#define FUNCNAME init_tcp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int init_tcp (MPIDI_PG_t *pg_p) 
{
    int           mpi_errno = MPI_SUCCESS;
    int           ret;
    int           pmi_errno;
    int           numprocs  = MPID_nem_mem_region.ext_procs;
    unsigned int  len       = sizeof(struct sockaddr_in);
    int           port      = 0 ;  
    int           grank;
    int           index;
    node_t       *nodes;
   
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    MPIU_CHKPMEM_DECL(1);
    
    mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* Allocate more than used, but fill only the external ones */
    MPIU_CHKPMEM_MALLOC (nodes, node_t *, sizeof (node_t) * MPID_nem_mem_region.num_procs, mpi_errno, "node struct");
    MPID_nem_tcp_internal_vars.nodes = nodes;
    MPID_nem_tcp_internal_vars.nb_procs = numprocs;

    /* All Masters create their sockets and put their keys w/PMI */
    for (index = 0 ; index < numprocs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if (grank > MPID_nem_mem_region.rank)
	{
	    struct sockaddr_in temp;
	    char               s[255];
	    int                len2 = 255;
            int                low_port, high_port;
            
	    nodes[grank].desc = socket(AF_INET, SOCK_STREAM, 0);

            /* find a port in the specified range */
            /*     if the environment var is not set, low_port and high_port are unchanged */
            low_port = high_port = 0;
            MPIU_GetEnvRange( "MPICH_PORT_RANGE", &low_port, &high_port );

            /* if MPICH_PORT_RANGE is not set, low_port and high_port are 0 so bind will use any available address */
            for (port = low_port; port <= high_port; ++port)
            {
                memset ((void *)&temp, 0, sizeof(temp));
                temp.sin_family      = AF_INET;
                temp.sin_addr.s_addr = htonl(INADDR_ANY);
                temp.sin_port        = htons(port);	
                

                ret = bind(nodes[grank].desc, (struct sockaddr *)&temp, len);
                if (ret == -1)
                {
                    /* check for real error */
                    MPIU_ERR_CHKANDJUMP3 (errno != EADDRINUSE && errno != EADDRNOTAVAIL, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));
                }
                else
                {
                    break;
                }
            }
            /* check if an available port was found */
            MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));
    
            
	    ret = getsockname(nodes[grank].desc, 
			      (struct sockaddr *)&(nodes[grank].sock_id), 
			      &len);	 
            MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", strerror (errno));

	  
	    ret = listen(nodes[grank].desc, SOMAXCONN);	      
            MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", strerror (errno), errno);
	  
	    /* Put the key (machine name, port #, src , dest) with PMI */
	    ret = gethostname(s, len2);
            MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost", "**sock_gethost %s %d", strerror (errno), errno);
#ifdef TRACE
	    fprintf(stderr,"[%i] ID :  %s_%d_%d_%d \n",MPID_nem_mem_region.rank,s,
		    ntohs(nodes[grank].sock_id.sin_port),grank,MPID_nem_mem_region.rank);
#endif
	    MPIU_Snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "%d:%s", ntohs(nodes[grank].sock_id.sin_port), s);
	    MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "TCPkey[%d:%d]", MPID_nem_mem_region.rank, grank);

	    /* Put my unique id */
	    pmi_errno = PMI_KVS_Put (kvs_name, key, val);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
	  
	    pmi_errno = PMI_KVS_Commit (kvs_name);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
	}       
	else if (grank < MPID_nem_mem_region.rank)
	{
	    struct sockaddr_in temp;
            int                low_port, high_port;

	    nodes[grank].desc   = socket(AF_INET, SOCK_STREAM, 0);

            /* find a port in the specified range */
            /*     if the environment var is not set, low_port and high_port are unchanged */
            low_port = high_port = 0;
            MPIU_GetEnvRange( "MPICH_PORT_RANGE", &low_port, &high_port );

            /* if MPICH_PORT_RANGE is not set, low_port and high_port are 0 so bind will use any available address */
            for (port = low_port; port <= high_port; ++port)
            {
                memset ((void *)&temp, 0, sizeof(temp));
                temp.sin_family      = AF_INET;
                temp.sin_addr.s_addr = htonl(INADDR_ANY);
                temp.sin_port        = htons(port);

                
                ret = bind (nodes[grank].desc, (struct sockaddr *)&temp, len);
                if (ret == -1)
                {
                    /* check for real error */
                    MPIU_ERR_CHKANDJUMP3 (errno != EADDRINUSE && errno != EADDRNOTAVAIL, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));
                }
                else
                {
                    break;
                }
                
            }
            /* check if an available port was found */
            MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));

	    ret = getsockname(nodes[grank].desc,
			      (struct sockaddr *)&(nodes[grank].sock_id),
			      &len);
            MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", strerror (errno));
	}
    }
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

#ifdef TRACE
    fprintf(stderr,"[%i] ---- Creating sockets done \n",MPID_nem_mem_region.rank);	  	  
#endif

    /* Connect/accept sequence */
    for (index = 0 ; index < numprocs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if (grank > MPID_nem_mem_region.rank)
	{
	    /* I am a master */
#ifdef TRACE  
	    fprintf(stderr,"MASTER accepting sockets \n");
#endif
	    nodes[grank].desc = accept(nodes[grank].desc,
                                       (struct sockaddr *)&(nodes[grank].sock_id),
                                       &len);
            MPIU_ERR_CHKANDJUMP2 (nodes[grank].desc == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|accept", "**sock|poll|accept %d %s", errno, strerror (errno));
            {
                struct sockaddr_in sid;
                size_t sidlen = sizeof(sid);
                getsockname(nodes[grank].desc, (struct sockaddr *)&sid, &sidlen);
            }
            

#ifdef TRACE  
	    fprintf(stderr,"[%i] ====> ACCEPT DONE for GRANK %i\n",MPID_nem_mem_region.rank,grank);    
#endif
	}
	else if (grank < MPID_nem_mem_region.rank)
	{	      
	    /* I am the slave */
	    struct sockaddr_in  master;
	    struct hostent     *hp = NULL;
	    char                s[255];
	    int                 port_num;  
		  
	    memset(val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	    MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN,"TCPkey[%d:%d]", grank, MPID_nem_mem_region.rank);
	      
	    pmi_errno = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
	  
	    ret = sscanf (val, "%d:%s", &port_num, s);
            MPIU_ERR_CHKANDJUMP1 (ret != 2, mpi_errno, MPI_ERR_OTHER, "**business_card", "**business_card %s", val);

	    hp = gethostbyname(s);	  
            MPIU_ERR_CHKANDJUMP1 (hp == NULL, mpi_errno, MPI_ERR_OTHER, "**gethostbyname", "**gethostbyname %d", h_errno);
	    master.sin_family      = AF_INET;
	    master.sin_port        = htons(port_num);
	    MPID_NEM_MEMCPY(&(master.sin_addr.s_addr), hp->h_addr, hp->h_length);
	    
	    ret = connect(nodes[grank].desc,(struct sockaddr *)&master, sizeof(master));
            MPIU_ERR_CHKANDJUMP4 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_connect", "**sock_connect %s %d %s %d", s, port_num, strerror (errno), errno);
            {
                struct sockaddr_in sid;
                size_t sidlen = sizeof(sid);
                getsockname(nodes[grank].desc, (struct sockaddr *)&sid, &sidlen);
            }
#ifdef TRACE
	    fprintf(stderr,"====> CONNECT DONE : %i\n", ret);	      	 
#endif
	}
    }

    for(index = 0 ; index < numprocs ; index++)
    {
	int       option = 1;
        int       option2;
        socklen_t size;
       
	grank     = MPID_nem_mem_region.ext_ranks[index];
	if(grank != MPID_nem_mem_region.rank)
	{
	    nodes[grank].internal_recv_queue.head = NULL;
	    nodes[grank].internal_recv_queue.tail = NULL;
	    nodes[grank].internal_free_queue.head = NULL;
	    nodes[grank].internal_free_queue.tail = NULL;
	  
	    nodes[grank].left2write     = 0;
	    nodes[grank].left2read_head = 0;
	    nodes[grank].left2read      = 0;
	    nodes[grank].toread         = 0;

#ifdef TRACE
	    fprintf(stderr,"[%i] ----- DESC %i is %i ------ \n",
		    MPID_nem_mem_region.rank,grank,
		    nodes[grank].desc);
#endif

	    FD_SET(nodes[grank].desc, &MPID_nem_tcp_internal_vars.set);
	    if(nodes[grank].desc > MPID_nem_tcp_internal_vars.max_fd)
		MPID_nem_tcp_internal_vars.max_fd = nodes[grank].desc ;
	  
	    ret = fcntl(nodes[grank].desc, F_SETFL, O_NONBLOCK);
            MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fcntl", "**fcntl %s", strerror (errno));
	    ret = setsockopt( nodes[grank].desc, 
                              IPPROTO_TCP,  
                              TCP_NODELAY,  
                              &option, 
                              sizeof(int));
            MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**setsockopt", "**setsockopt %s", strerror (errno));
 	    getsockopt(nodes[grank].desc,IPPROTO_TCP,TCP_NODELAY,&option2,&size);
	   
	    option = 2 * MPID_NEM_CELL_PAYLOAD_LEN ;
	    ret = setsockopt( nodes[grank].desc, 
                              SOL_SOCKET,  
                              SO_RCVBUF,  
                              &option, 
                              sizeof(int));
            MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**setsockopt", "**setsockopt %s", strerror (errno));
	    getsockopt(nodes[grank].desc,SOL_SOCKET,SO_RCVBUF,&option2,&size);
	   
	    ret = setsockopt( nodes[grank].desc, 
                              SOL_SOCKET,  
                              SO_SNDBUF,  
                              &option, 
                              sizeof(int));  	  
            MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**setsockopt", "**setsockopt %s", strerror (errno));
	    getsockopt(nodes[grank].desc,SOL_SOCKET,SO_SNDBUF,&option2,&size);
	    
	    setsockopt( nodes[grank].desc, IPPROTO_TCP,TCP_MAXSEG,&option,sizeof(int));
	    getsockopt(nodes[grank].desc,IPPROTO_TCP,TCP_MAXSEG,&option2,&size);
	}
    }
    (MPID_nem_tcp_internal_vars.max_fd)++;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


/*
   int  
   MPID_nem_tcp_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
                            MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
                            MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
       ckpt_restart -- true if this is a restart from a checkpoint.  In a restart, the network needs to be brought up again, but
                       we want to keep things like sequence numbers.
   OUT
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_init (MPID_nem_queue_ptr_t  proc_recv_queue, 
			  MPID_nem_queue_ptr_t  proc_free_queue, 
			  MPID_nem_cell_ptr_t    proc_elements,   
			  int num_proc_elements,
			  MPID_nem_cell_ptr_t    module_elements, 
			  int num_module_elements, 
			  MPID_nem_queue_ptr_t *module_free_queue,
			  int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank,
			  char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int index;
    MPIU_CHKPMEM_DECL(1);
 
    MPID_nem_tcp_internal_vars.n_pending_send  = 0;
    MPID_nem_tcp_internal_vars.n_pending_recv  = 0;
    MPID_nem_tcp_internal_vars.outstanding     = 0;
    MPID_nem_tcp_internal_vars.poll_freq       = TCP_POLL_FREQ_NO;
    MPID_nem_tcp_internal_vars.old_poll_freq   = TCP_POLL_FREQ_NO;

    MPIU_CHKPMEM_MALLOC (MPID_nem_tcp_internal_vars.n_pending_sends, int *, MPID_nem_mem_region.num_procs * sizeof (int), mpi_errno, "pending sends");
    for(index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
    {
	MPID_nem_tcp_internal_vars.n_pending_sends[index] = 0;
    }

    if( MPID_nem_mem_region.ext_procs > 0)
    {
	mpi_errno = init_tcp (pg_p);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
       
	if(MPID_nem_mem_region.num_local == 0)
	    MPID_nem_tcp_internal_vars.poll_freq = TCP_POLL_FREQ_ALONE ;
	else
	    MPID_nem_tcp_internal_vars.poll_freq = TCP_POLL_FREQ_MULTI ;
	MPID_nem_tcp_internal_vars.old_poll_freq = MPID_nem_tcp_internal_vars.poll_freq;	
    }

    MPID_nem_process_recv_queue = proc_recv_queue;
    MPID_nem_process_free_queue = proc_free_queue;

    MPID_nem_module_tcp_free_queue = &_free_queue;

    MPID_nem_queue_init (MPID_nem_module_tcp_free_queue);

    for (index = 0; index < num_module_elements; ++index)
    {
	MPID_nem_queue_enqueue (MPID_nem_module_tcp_free_queue, &module_elements[index]);
    }

    *module_free_queue = MPID_nem_module_tcp_free_queue;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    return MPI_SUCCESS;
}



