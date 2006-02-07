#include "mpid_nem.h"

#define printf_dd(x...) /*printf (x) */

int MPID_nem_ckpt_logging_messages; /* are we in logging-message-mode? */
int MPID_nem_ckpt_sending_markers; /* are we in the process of sending markers? */
struct cli_message_log_total *MPID_nem_ckpt_message_log; /* are we replaying messages? */

#ifdef ENABLED_CHECKPOINTING
#include "cli.h"

static int* log_msg;
static int* sent_marker;
static unsigned short current_wave;
static int next_marker_dest;

static void checkpoint_shutdown();
static void restore_env (int rank);

extern unsigned short *recv_seqno;

static int _rank;

int
MPID_nem_ckpt_init (int ckpt_restart)
{
    int num_procs;
    int rank;
    int i;

    num_procs = MPID_nem_mem_region.num_procs;
    rank = _rank = MPID_nem_mem_region.rank;

    if (!ckpt_restart)
    {
	process_id_t *procids;

	procids = MALLOC (sizeof (*procids) * num_procs);
	if (!procids)
	    ERROR_RET (-1, "MALLOC_FAILED");

	for (i = 0; i < num_procs; ++i)
	    procids[i] = i;
	
	cli_init (num_procs, procids, rank, checkpoint_shutdown);
    }
	    
    log_msg = MALLOC (sizeof (*log_msg) * num_procs);
    if (!log_msg)
	ERROR_RET (-1, "MALLOC failed");
    sent_marker = MALLOC (sizeof (*sent_marker) * num_procs);
    if (!sent_marker)
	ERROR_RET (-1, "MALLOC failed");

    for (i = 0; i < num_procs; ++i)
    {
	log_msg[i] = 0;
	sent_marker[i] = 0;
    }

    MPID_nem_ckpt_logging_messages = 0;
    MPID_nem_ckpt_sending_markers = 0;

    current_wave = 0;
    
    return 0;
}

void
MPID_nem_ckpt_finalize()
{
    FREE (log_msg);
    FREE (sent_marker);
}

void
MPID_nem_ckpt_maybe_take_checkpoint()
{
    int ret;
    struct cli_marker marker;
    int num_procs;
    int rank;
    int i;
    
    num_procs = MPID_nem_mem_region.num_procs;
    rank = MPID_nem_mem_region.rank;
   
    ret = cli_check_for_checkpoint_start (&marker);

    switch (ret)
    {
    case CLI_CP_BEGIN:
	printf_dd ("%d: cli_check_for_checkpoint_start: CLI_CP_BEGIN wave = %d\n", rank, marker.checkpoint_wave_number);
	
	assert (MPID_nem_ckpt_logging_messages == 0);

	/* initialize the state */
	MPID_nem_ckpt_logging_messages = num_procs;
	MPID_nem_ckpt_sending_markers = num_procs;
	for (i = 0; i < num_procs; ++i)
	{
	    log_msg[i] = 1;
	    sent_marker[i] = 0;
	}
	
	/* we don't send messages to ourselves, so we pretend we sent and received a marker */
	--MPID_nem_ckpt_sending_markers;
	ret = cli_on_marker_receive (&marker, rank);
	assert (ret == CLI_CP_MARKED);
	--MPID_nem_ckpt_logging_messages;
	log_msg[rank] = 0;
	sent_marker[rank] = 1;

	next_marker_dest = 0;
	current_wave = marker.checkpoint_wave_number;
	MPID_nem_ckpt_send_markers ();
	break;
    case CLI_RESTART:
	{
	    int newrank;
	    int newsize;
	    struct cli_emitter_based_message_log *per_rank_log;
	    printf_dd ("%d: cli_check_for_checkpoint_start: CLI_CP_RESTART wave = %d\n", rank, marker.checkpoint_wave_number);

	    restore_env (rank);
	    printf_dd ("%d: before _MPID_nem_init\n", rank);
	    _MPID_nem_init (0, NULL, &newrank, &newsize, 1);
	    printf_dd ("%d: after _MPID_nem_init\n", rank);
	    assert (newrank == rank);
	    assert (newsize == num_procs);   
	    printf_dd ("%d: _MPID_nem_init done\n", rank);

	    per_rank_log = MALLOC (sizeof (struct cli_emitter_based_message_log) * num_procs);
	    if (!per_rank_log)
		FATAL_ERROR ("malloc error");
	    MPID_nem_ckpt_message_log = cli_get_network_state (per_rank_log);
	    printf_dd ("%d: got %s message log\n", rank, MPID_nem_ckpt_message_log ? "non-empty" : "empty");
	    FREE (per_rank_log); /* we don't need the log separated by rank */
	    break;
	}
    case CLI_NOTHING:
	break;
    default:
	FATAL_ERROR ("Error taking checkpoint");
	return;
    }
}

void
MPID_nem_ckpt_got_marker (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int ret;
    int source;
    struct cli_marker marker;
    int num_procs;
    int rank;
    int i;
    
    num_procs = MPID_nem_mem_region.num_procs;
    rank = MPID_nem_mem_region.rank;

    marker.checkpoint_wave_number = (*cell)->pkt.ckpt.wave;
    source = (*cell)->pkt.ckpt.source;

    if (!*in_fbox)
    {
	MPID_nem_mpich2_release_cell (*cell);
    }
    else
    {
	MPID_nem_mpich2_release_fbox (*cell);
    }
    
    *cell = NULL;
    *in_fbox = 0;

    ret = cli_on_marker_receive (&marker, source);

    switch (ret)
    {
    case CLI_CP_BEGIN:
	printf_dd ("%d: cli_on_marker_recv: CLI_CP_BEGIN wave = %d\n", rank, marker.checkpoint_wave_number);
	assert (MPID_nem_ckpt_logging_messages == 0);

	/* initialize the state */
	MPID_nem_ckpt_logging_messages = num_procs;
	MPID_nem_ckpt_sending_markers = num_procs;
	for (i = 0; i < num_procs; ++i)
	{
	    log_msg[i] = 1;
	    sent_marker[i] = 0;
	}
	
	/* we received one from the source of this message */
	--MPID_nem_ckpt_logging_messages;
	log_msg[source] = 0;
	
	/* we don't send messages to ourselves, so we pretend we sent and received a marker */
	--MPID_nem_ckpt_sending_markers;
	ret = cli_on_marker_receive (&marker, rank);
	assert (ret == CLI_CP_MARKED);
	--MPID_nem_ckpt_logging_messages;
	log_msg[rank] = 0;
	sent_marker[rank] = 1;

	next_marker_dest = 0;
	current_wave = marker.checkpoint_wave_number;
	MPID_nem_ckpt_send_markers();
	break;
    case CLI_CP_MARKED:
	printf_dd ("%d: cli_on_marker_recv: CLI_CP_MARKED wave = %d\n", rank, marker.checkpoint_wave_number);
	assert (MPID_nem_ckpt_logging_messages);
	assert (log_msg[source]);
	    
	--MPID_nem_ckpt_logging_messages;
	log_msg[source] = 0;
	break;
    case CLI_RESTART:
	{
	    int newrank;
	    int newsize;
	    struct cli_emitter_based_message_log *per_rank_log;

	    --recv_seqno[source]; /* Will thie really work????? */
	    
	    printf_dd ("%d: cli_on_marker_recv: CLI_CP_RESTART wave = %d\n", rank, marker.checkpoint_wave_number);
	    restore_env (rank);
	    printf_dd ("%d: before _MPID_nem_init\n", rank);
	    _MPID_nem_init (0, NULL, &newrank, &newsize, 1);
	    printf_dd ("%d: after _MPID_nem_init\n", rank);
	    assert (newrank == rank);
	    assert (newsize == num_procs);
	    printf_dd ("%d: _MPID_nem_init done\n", rank);

	    per_rank_log = MALLOC (sizeof (struct cli_emitter_based_message_log) * num_procs);
	    if (!per_rank_log)
		FATAL_ERROR ("malloc error");
	    MPID_nem_ckpt_message_log = cli_get_network_state (per_rank_log);
	    printf_dd ("%d: got %s message log\n", rank, MPID_nem_ckpt_message_log ? "non-empty" : "empty");
	    FREE (per_rank_log); /* we don't need the log separated by rank */
	    break;
	}
    default:
	FATAL_ERROR ("Error taking checkpoint");
	return;
    }
}

void
MPID_nem_ckpt_log_message (MPID_nem_cell_ptr_t cell)
{
    int source;
    
    source = cell->pkt.mpich2.source;
    if (log_msg[source])
    {
	cli_log_message (cell, cell->pkt.header.datalen + MPID_NEM__MPICH2_HEADER_LEN + MPID_NEM_OFFSETOF (MPID_nem_cell_t, pkt.mpich2.payload), source);
	printf_dd ("%d: logging message from %d size (%d + %d + %d = %d)\n", _rank, source,
		   cell->pkt.header.datalen, MPID_NEM__MPICH2_HEADER_LEN, MPID_NEM_OFFSETOF (MPID_nem_cell_t, pkt.mpich2.payload),
		   cell->pkt.header.datalen + MPID_NEM__MPICH2_HEADER_LEN + MPID_NEM_OFFSETOF (MPID_nem_cell_t, pkt.mpich2.payload));
    }

}

void
MPID_nem_ckpt_send_markers()
{
    /* keep sending markers until we're done or out of cells */
    int ret;
    int num_procs = MPID_nem_mem_region.num_procs;
    MPIDI_VC_t vc;
    
    assert (MPID_nem_ckpt_sending_markers);
    assert (next_marker_dest < num_procs);

    while (next_marker_dest < num_procs)
    {
	if (sent_marker[next_marker_dest])
	{
	    ++next_marker_dest;
	    continue;
	}
	
	MPIDI_PG_Get_vc (MPIDI_Process.my_pg, next_marker_dest, &vc);
	ret = MPID_nem_mpich2_send_ckpt_marker (current_wave, vc);
	if (ret == MPID_NEM_MPICH2_AGAIN)
	    break;
	if (ret == MPID_NEM_MPICH2_FAILURE)
	    FATAL_ERROR ("checkpoint send failed");

	sent_marker[next_marker_dest] = 1;
	++next_marker_dest;
	--MPID_nem_ckpt_sending_markers;
    }
}

int
MPID_nem_ckpt_replay_message (MPID_nem_cell_ptr_t *cell)
{
    assert (MPID_nem_ckpt_message_log);

    *cell = (MPID_nem_cell_ptr_t)MPID_nem_ckpt_message_log->ptr;
    (*cell)->pkt.header.type = MPID_NEM_PKT_CKPT_REPLAY;
    printf_dd ("%d: replaying message source = %d (%d) seno=%d\n", _rank, MPID_nem_ckpt_message_log->from, (*cell)->pkt.header.source,
	       (*cell)->pkt.header.seqno);
    
    MPID_nem_ckpt_message_log = MPID_nem_ckpt_message_log->next;

    return 0;
}

void
MPID_nem_ckpt_free_msg_log()
{
    assert (!MPID_nem_ckpt_message_log);
    cli_message_log_free();
}

static void
checkpoint_shutdown()
{
    int ret;
    
    ret = MPID_nem_ckpt_shutdown ();
    if (ret)
	FATAL_ERROR ("checkpoint shutdown failed");
}

#define MAX_STR_LEN 256

static void
restore_env (int rank)
{
    FILE *fd;
    char env_filename[MAX_STR_LEN];
    char var[MAX_STR_LEN], val[MAX_STR_LEN];
    int ret;

    printf_dd ("%d: restore_env\n", rank);
    
    snprintf (env_filename, MAX_STR_LEN, "/tmp/cli-restart-env:%d", rank);
    printf_dd ("1 %s\n", env_filename);
    fd = fopen (env_filename, "r");
    if (!fd)
	FATAL_PERROR ("fopen(%s) failed", env_filename);
    printf_dd ("2 %p\n", fd);
    
    {
	int i;

	for (i = 0; i < sizeof(FILE); ++i)
	{
	    if (!(i%8))
		printf (" ");
	    if (!(i%64))
		printf ("\n");
	    printf ("%02x", ((unsigned char *)fd)[i]);
	}
	printf ("\n--\n");

	fgets (var, MAX_STR_LEN, fd);
	printf_dd ("var = %s\n", var);
    }
    
    ret = fscanf (fd, "%[^=]=%[^\n]\n", var, val);
    printf_dd ("2\n");
    while (ret != EOF)
    {
	printf_dd ("3\n");
	if (ret == 2)
	{
	    ret = setenv (var, val, 1);
	    printf_dd ("4\n");
	    if (ret)
		FATAL_ERROR ("putenv failed");
	}
	ret = fscanf (fd, "%[^=]=%[^\n]\n", var, val);
	printf_dd ("5\n");
    }
    printf_dd ("6\n");
}
#endif
