/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_close_port(char *port_name)
{
    OpenPortNode *p, *pTrailer;

    MM_ENTER_FUNC(MM_CLOSE_PORT);

    pTrailer = p = MPID_Process.port_list;
    if (p == NULL)
    {
	MM_EXIT_FUNC(MM_CLOSE_PORT);
	return -1;
    }

    while (p)
    {
	if (strncmp(port_name, p->port_name, MPI_MAX_PORT_NAME) == 0)
	{
	    beasy_closesocket(p->bfd);
	    if (p == MPID_Process.port_list)
		MPID_Process.port_list = MPID_Process.port_list->next;
	    else
		pTrailer->next = p->next;
	    MPIU_Free(p);
	    MM_EXIT_FUNC(MM_CLOSE_PORT);
	    return 0;
	}
	if (pTrailer != p)
	    pTrailer = pTrailer->next;
	p = p->next;
    }

    MM_EXIT_FUNC(MM_CLOSE_PORT);
    return -1;
}

