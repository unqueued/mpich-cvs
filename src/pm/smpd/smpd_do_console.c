/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "smpd.h"

int smpd_do_console()
{
    int result;
    smpd_context_t *context;
    sock_set_t set;
    sock_t sock;
    SMPD_BOOL no_smpd = SMPD_FALSE;
    int saved_state = 0;

    smpd_enter_fn("smpd_do_console");

    /* make sure we have a passphrase to authenticate connections to the smpds */
    if (smpd_process.passphrase[0] == '\0')
	smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    if (smpd_process.passphrase[0] == '\0')
    {
	if (smpd_process.noprompt)
	{
	    printf("Error: No smpd passphrase specified through the registry or .smpd file, exiting.\n");
	    goto quit_job;
	}
	printf("Please specify an authentication passphrase for smpd: ");
	fflush(stdout);
	smpd_get_password(smpd_process.passphrase);
    }

    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	goto quit_job;
    }
    smpd_process.set = set;

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;

    /* turn off output if do_status is selected to supress error messages */
    if (smpd_process.do_status)
    {
	saved_state = smpd_process.dbg_state;
	smpd_process.dbg_state = 0;
    }

    /* start connecting the tree by posting a connect to the first host */
    result = sock_post_connect(set, NULL, smpd_process.console_host, smpd_process.port, &sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("Unable to connect to '%s:%d',\nsock error: %s\n",
	    smpd_process.console_host, smpd_process.port, get_sock_error_string(result));
	no_smpd = SMPD_TRUE;
	goto quit_job;
    }
    /* turn output back on */
    if (smpd_process.do_status)
	smpd_process.dbg_state = saved_state;

    result = smpd_create_context(SMPD_CONTEXT_LEFT_CHILD, set, sock, 1, &context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to create a context.\n");
	goto quit_job;
    }
    context->state = SMPD_MPIEXEC_CONNECTING_SMPD;
    smpd_process.left_context = context;
    result = sock_set_user_ptr(sock, context);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("Unable to set the smpd sock user pointer,\nsock error: %s\n",
	    get_sock_error_string(result));
	goto quit_job;
    }
    /* turn off output if do_status is selected to supress error messages */
    if (smpd_process.do_status)
	smpd_process.dbg_state = 0;
    result = smpd_enter_at_state(set, SMPD_MPIEXEC_CONNECTING_SMPD);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("state machine failed.\n");
	no_smpd = SMPD_TRUE;
	goto quit_job;
    }
    /* turn output back on */
    if (smpd_process.do_status)
	smpd_process.dbg_state = saved_state;

quit_job:

    if (smpd_process.do_status && (no_smpd || smpd_process.state_machine_ret_val != SMPD_SUCCESS))
    {
	printf("no smpd running on %s\n", smpd_process.console_host);
	smpd_process.dbg_state = saved_state;
    }
    /* finalize */
    smpd_dbg_printf("calling sock_finalize\n");
    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }

#ifdef HAVE_WINDOWS_H
    if (smpd_process.hCloseStdinThreadEvent)
	SetEvent(smpd_process.hCloseStdinThreadEvent);
    if (smpd_process.hStdinThread != NULL)
    {
	if (WaitForSingleObject(smpd_process.hStdinThread, 3000) != WAIT_OBJECT_0)
	{
	    TerminateThread(smpd_process.hStdinThread, 321);
	}
	CloseHandle(smpd_process.hStdinThread);
    }
    if (smpd_process.hCloseStdinThreadEvent)
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
#endif
    smpd_exit_fn("smpd_do_console");
    smpd_exit(0);
    return SMPD_SUCCESS;
}
