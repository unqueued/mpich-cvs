/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_create_set
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_create_set(struct MPIDU_Sock_set ** sock_setp)
{
    struct MPIDU_Sock_set * sock_set = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * Allocate and initialized a new sock set structure
     */
    sock_set = MPIU_Malloc(sizeof(struct MPIDU_Sock_set));
    if (sock_set == NULL)
    { 
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|setalloc", 0);
	goto fn_fail;
    }
    
    sock_set->id = MPIDU_Socki_set_next_id++;
    sock_set->poll_array_sz = 0;
    sock_set->poll_array_elems = 0;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;
#   if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    {
	sock_set->pollfds_active = NULL;
	sock_set->pollfds_updated = FALSE;
	sock_set->wakeup_posted = FALSE;
	sock_set->intr_fds[0] = -1;
	sock_set->intr_fds[1] = -1;
	sock_set->intr_sock = NULL;
    }
#   endif

#   if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    {
	struct MPIDU_Sock * sock = NULL;
	struct pollfd * pollfd;
	struct pollinfo * pollinfo;
	long flags;
	int rc;
	
	/*
	 * Acquire a pipe (the interrupter) to wake up a blocking poll should it become necessary.
	 *
	 * Make the read descriptor nonblocking.  The write descriptor is left as a blocking descriptor.  The write has to
	 * succeed or the system will lock up.  Should the blocking descriptor prove to be a problem, then (1) copy the above
	 * code, applying it to the write descriptor, and (2) update MPIDU_Socki_wakeup() so that it loops while write returns 0,
	 * performing a thread yield between iterations.
	 */
	rc = pipe(sock_set->intr_fds);
	if (rc != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|pipe", "**sock|poll|pipe %d %s", errno, MPIU_Strerror(errno));
	    goto fn_fail;
	}

	flags = fcntl(sock_set->intr_fds[0], F_GETFL, 0);
	if (flags == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d %s",
					     errno, MPIU_Strerror(errno));
	    goto fn_fail;
	}
    
	rc = fcntl(sock_set->intr_fds[0], F_SETFL, flags | O_NONBLOCK);
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d %s",
					     errno, MPIU_Strerror(errno));
	    goto fn_fail;
	}

	/*
	 * Allocate and initialize a sock structure for the interrupter pipe
	 */
	mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**sock|sockalloc", NULL);
	    goto fn_fail;
	}
    
	sock_set->intr_sock = sock;

	pollfd = MPIDU_Socki_sock_get_pollfd(sock);
	pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    
	pollinfo->fd = sock_set->intr_fds[0];
	pollinfo->user_ptr = NULL;
	pollinfo->type = MPIDU_SOCKI_TYPE_INTERRUPTER;
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;
	pollinfo->os_errno = 0;

	MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);
    }
#   endif

    *sock_setp = sock_set;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    return mpi_errno;

  fn_fail:
    if (sock_set != NULL)
    {
#       if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
	{
	    if (sock_set->intr_fds[0] != -1)
	    {
		close(sock_set->intr_fds[0]);
	    }
	    
	    if (sock_set->intr_fds[1] != -1)
	    {
		close(sock_set->intr_fds[1]);
	    }
	}
#	endif
	
	MPIU_Free(sock_set);
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_destroy_set
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_destroy_set(struct MPIDU_Sock_set * sock_set)
{
    int elem;
    struct MPIDU_Sock_event event;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_DESTROY_SET);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * FIXME: check for open socks and return an error if any are found
     */
    
    /*
     * FIXME: verify no other thread is blocked in poll().  wake it up and get it to exit.
     */
    
    /*
     * Close pipe for interrupting a blocking poll()
     */
#   if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    {
	close(sock_set->intr_fds[1]);
	close(sock_set->intr_fds[0]);
	MPIDU_Socki_sock_free(sock_set->intr_sock);

	sock_set->pollfds_updated = FALSE;
	sock_set->pollfds_active = NULL;
	sock_set->wakeup_posted = FALSE;
	sock_set->intr_fds[0] = -1;
	sock_set->intr_fds[1] = -1;
	sock_set->intr_sock = NULL;
    }
#   endif

    /*
     * Clear the event queue to eliminate memory leaks
     */
    while (MPIDU_Socki_event_dequeue(sock_set, &elem, &event) == MPI_SUCCESS);

    /*
     * Free structures used by the sock set
     */
    MPIU_Free(sock_set->pollinfos);
    MPIU_Free(sock_set->pollfds);

    /*
     * Reset the sock set fields
     */
    sock_set->id = ~0;
    sock_set->poll_array_sz = 0;
    sock_set->poll_array_elems = 0;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;

    /*
     * Free the structure
     */
    MPIU_Free(sock_set);
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
    return mpi_errno;
}
