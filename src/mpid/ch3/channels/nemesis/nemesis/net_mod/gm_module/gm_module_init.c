/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_module_impl.h"

#define MAX_GM_BOARDS 16
#define UNIQUE_ID_LEN 6
#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_UNIQUE_KEY "unique"
#define UNDEFINED_UNIQUE_ID_VAL "\0\0\0\0\0\0"

static unsigned char unique_id[UNIQUE_ID_LEN] = UNDEFINED_UNIQUE_ID_VAL;
static int port_id;

int MPID_nem_module_gm_num_send_tokens = 0;

struct gm_port *MPID_nem_module_gm_port = 0;

#define FREE_SEND_QUEUE_ELEMENTS MPID_NEM_NUM_CELLS
MPID_nem_gm_module_send_queue_head_t MPID_nem_gm_module_send_queue = {0};
MPID_nem_gm_module_send_queue_t *MPID_nem_gm_module_send_free_queue = 0;

static int my_pg_rank;

#undef FUNCNAME
#define FUNCNAME init_gm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
init_gm (int *boardId, int *portId, unsigned char unique_id[])
{
    int mpi_errno = MPI_SUCCESS;
    gm_status_t status;
    int max_gm_ports;
    
    status = gm_init();
    MPIU_ERR_CHKANDJUMP1 (status != GM_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gm_init", "**gm_init %d", status);
    
    max_gm_ports = gm_num_ports (NULL);
    
    for (*portId = 0; *portId < max_gm_ports; ++*portId)
    {
	/* skip reserved gm ports */
	if (*portId == 0 || *portId == 1 || *portId == 3)
	    continue;
	for (*boardId = 0; *boardId < MAX_GM_BOARDS; ++*boardId)
	{
	    status = gm_open (&MPID_nem_module_gm_port, *boardId, *portId, " ", GM_API_VERSION);
		
	    switch (status)
	    {
	    case GM_SUCCESS:
		/* successfuly allocated a port */
                status = gm_get_unique_board_id (MPID_nem_module_gm_port, (char *)unique_id);
                MPIU_ERR_CHKANDJUMP1 (status != GM_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gm_get_unique_board_id", "**gm_get_unique_board_id %d", status);
                goto fn_exit;
		break;
	    case GM_INCOMPATIBLE_LIB_AND_DRIVER:
                MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**gm_incompatible_lib");
		break;
	    default:
		break;
	    }
		
	}
    }
    
    /* if we got here, we tried all ports and couldn't find a free one */
    MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**gm_no_port");

 fn_exit:
    return mpi_errno;
 fn_fail:
    memset (unique_id, 0, UNIQUE_ID_LEN);
    goto fn_exit;
}


/*
   int  
   MPID_nem_gm_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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
#define FUNCNAME MPID_nem_gm_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
                         MPID_nem_queue_ptr_t proc_free_queue, 
                         MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
                         MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
                         MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
                         MPIDI_PG_t *pg_p, int pg_rank,
                         char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int board_id;
    int ret;
    gm_status_t status;
    int i;

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_gm_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

    my_pg_rank = pg_rank;
    
    mpi_errno = init_gm(&board_id, &port_id, unique_id);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_gm_module_get_business_card(pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPID_nem_module_gm_num_send_tokens = gm_num_send_tokens(MPID_nem_module_gm_port);

    mpi_errno = MPID_nem_gm_module_recv_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_gm_module_send_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPID_nem_gm_module_send_queue.head = NULL;
    MPID_nem_gm_module_send_queue.tail = NULL;

    MPID_nem_gm_module_send_free_queue = NULL;
    
    for (i = 0; i < FREE_SEND_QUEUE_ELEMENTS; ++i)
    {
	MPID_nem_gm_module_send_queue_t *e;
	
	e = MPIU_Malloc (sizeof (MPID_nem_gm_module_send_queue_t));
        if (e == NULL) MPIU_CHKMEM_SETERR (mpi_errno, sizeof (MPID_nem_gm_module_send_queue_t), "gm module send queue");
	e->next = MPID_nem_gm_module_send_free_queue;
	MPID_nem_gm_module_send_free_queue = e;
    }
    
    mpi_errno = MPID_nem_gm_module_lmt_init();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* we're not using the free_queue anymore FIXME: DARIUS take all references out of interface */
    *module_free_queue = NULL;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, port_id);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_UNIQUE_KEY, (char *)unique_id, UNIQUE_ID_LEN);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_get_port_unique_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_get_port_unique_from_bc (const char *business_card, unsigned *port_id, unsigned char *unique_id)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    int tmp_port_id;
    
    mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, &tmp_port_id);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }
    *port_id = (unsigned)tmp_port_id;

    mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_UNIQUE_KEY, (char *)unique_id, UNIQUE_ID_LEN, &len);
    if (mpi_errno != MPIU_STR_SUCCESS || len != UNIQUE_ID_LEN) {
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    /* In GM, once the VC is initialized there's nothing extra that we need to do to establish a connection */
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    int mpi_errno = MPI_SUCCESS;
    int ret;

    vc_ch->iStartContigMsg  = MPID_nem_gm_iStartContigMsg;
    vc_ch->iSendContig      = MPID_nem_gm_iSendContig;
    vc->sendNoncontig_fn    = MPID_nem_gm_SendNoncontig;
    VC_FIELD(vc, source_id) = my_pg_rank; /* FIXME: this is only valid for processes in COMM_WORLD */

    mpi_errno = MPID_nem_gm_module_get_port_unique_from_bc (business_card, &VC_FIELD(vc, gm_port_id), VC_FIELD(vc, gm_unique_id));
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    ret = gm_unique_id_to_node_id (MPID_nem_module_gm_port, (char *)VC_FIELD(vc, gm_unique_id), &VC_FIELD(vc, gm_node_id));
    /* --BEGIN ERROR HANDLING-- */
    if (ret != GM_SUCCESS)
    {
	mpi_errno = MPI_ERR_INTERN;
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   
   fn_exit:   
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_module_vc_terminate (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}
